#include "ai/PresetRecommendationEngine.h"
#include <algorithm>
#include <random>
#include <numeric>
#include <cmath>
#include <sstream>

namespace AIMusicHardware {

// UserProfile implementation

float UserProfile::calculatePreferenceScore(const AudioFeatureVector& features, 
                                           const std::string& category,
                                           const std::string& author) const {
    float score = 0.0f;
    float totalWeight = 0.0f;
    
    // Feature preferences (weighted by learned preferences)
    auto featureVector = features.toDenseVector();
    for (size_t i = 0; i < featureVector.size() && i < 20; ++i) { // Limit to first 20 features
        std::string featureKey = "feature_" + std::to_string(i);
        auto it = featurePreferences.find(featureKey);
        if (it != featurePreferences.end()) {
            score += featureVector[i] * it->second;
            totalWeight += std::abs(it->second);
        }
    }
    
    // Category preference
    auto catIt = categoryPreferences.find(category);
    if (catIt != categoryPreferences.end()) {
        score += catIt->second * 2.0f; // Category preferences are more important
        totalWeight += 2.0f;
    }
    
    // Author preference
    auto authIt = authorPreferences.find(author);
    if (authIt != authorPreferences.end()) {
        score += authIt->second * 1.5f;
        totalWeight += 1.5f;
    }
    
    return totalWeight > 0.0f ? std::clamp(score / totalWeight, 0.0f, 1.0f) : 0.5f;
}

// PresetRecommendationEngine implementation

PresetRecommendationEngine::PresetRecommendationEngine(std::shared_ptr<PresetMLAnalyzer> analyzer)
    : analyzer_(analyzer) {
    if (!analyzer_) {
        throw std::invalid_argument("PresetMLAnalyzer cannot be null");
    }
    
    initializeDefaults();
    userProfile_.lastUpdated = std::chrono::system_clock::now();
}

PresetRecommendationEngine::~PresetRecommendationEngine() = default;

std::vector<PresetRecommendation> PresetRecommendationEngine::getRecommendations(
    const RecommendationContext& context) {
    
    // If we don't have enough interaction data, fall back to similarity-based recommendations
    if (interactionHistory_.size() < minimumInteractions_) {
        if (!context.currentPreset.empty()) {
            return getSimilarPresets(context.currentPreset, context.maxRecommendations, context.diversityWeight);
        } else {
            return getTrendingRecommendations(168, context.maxRecommendations);
        }
    }
    
    // Generate recommendations using multiple algorithms
    std::vector<PresetRecommendation> allRecommendations;
    
    // Content-based recommendations (weighted by algorithm_weights)
    float contentWeight = algorithmWeights_.at("content_based");
    if (contentWeight > 0.0f) {
        auto contentRecs = contentBasedRecommendations(context);
        for (auto& rec : contentRecs) {
            rec.relevanceScore *= contentWeight;
            rec.explanation.algorithm = "content_based";
        }
        allRecommendations.insert(allRecommendations.end(), contentRecs.begin(), contentRecs.end());
    }
    
    // Collaborative filtering recommendations
    float collaborativeWeight = algorithmWeights_.at("collaborative");
    if (collaborativeWeight > 0.0f && interactionHistory_.size() >= minimumInteractions_) {
        auto collaborativeRecs = collaborativeFilteringRecommendations(context);
        for (auto& rec : collaborativeRecs) {
            rec.relevanceScore *= collaborativeWeight;
            rec.explanation.algorithm = "collaborative";
        }
        allRecommendations.insert(allRecommendations.end(), collaborativeRecs.begin(), collaborativeRecs.end());
    }
    
    // Workflow-based recommendations
    float workflowWeight = algorithmWeights_.at("workflow");
    if (workflowWeight > 0.0f && !context.recentPresets.empty()) {
        auto workflowRecs = getWorkflowRecommendations(context.recentPresets, context.maxRecommendations / 2);
        for (auto& rec : workflowRecs) {
            rec.relevanceScore *= workflowWeight;
            rec.explanation.algorithm = "workflow";
        }
        allRecommendations.insert(allRecommendations.end(), workflowRecs.begin(), workflowRecs.end());
    }
    
    // Add some discovery recommendations for serendipity
    float discoveryWeight = algorithmWeights_.at("discovery");
    if (discoveryWeight > 0.0f) {
        auto discoveryRecs = getDiscoveryRecommendations(userProfile_.explorationFactor, context.maxRecommendations / 4);
        for (auto& rec : discoveryRecs) {
            rec.relevanceScore *= discoveryWeight;
            rec.explanation.algorithm = "discovery";
        }
        allRecommendations.insert(allRecommendations.end(), discoveryRecs.begin(), discoveryRecs.end());
    }
    
    // Remove duplicates and merge scores for same presets
    std::unordered_map<std::string, PresetRecommendation> mergedRecs;
    for (const auto& rec : allRecommendations) {
        auto it = mergedRecs.find(rec.presetPath);
        if (it != mergedRecs.end()) {
            // Merge scores and explanations
            it->second.relevanceScore = std::max(it->second.relevanceScore, rec.relevanceScore);
            it->second.confidenceScore = (it->second.confidenceScore + rec.confidenceScore) / 2.0f;
            if (rec.relevanceScore > it->second.relevanceScore * 0.8f) {
                // Use explanation from higher-scoring algorithm
                it->second.explanation = rec.explanation;
            }
        } else {
            mergedRecs[rec.presetPath] = rec;
        }
    }
    
    // Convert back to vector and sort
    std::vector<PresetRecommendation> finalRecommendations;
    for (const auto& [path, rec] : mergedRecs) {
        finalRecommendations.push_back(rec);
    }
    
    // Sort by relevance score
    std::sort(finalRecommendations.begin(), finalRecommendations.end());
    
    // Apply diversification
    if (context.diversityWeight > 0.0f) {
        finalRecommendations = applyDiversification(finalRecommendations, context.diversityWeight);
    }
    
    // Limit to requested number of recommendations
    if (static_cast<int>(finalRecommendations.size()) > context.maxRecommendations) {
        finalRecommendations.resize(context.maxRecommendations);
    }
    
    // Update statistics
    updateStatistics(finalRecommendations, "hybrid");
    
    return finalRecommendations;
}

std::vector<PresetRecommendation> PresetRecommendationEngine::getSimilarPresets(
    const std::string& referencePreset,
    int maxResults,
    float diversityWeight) {
    
    std::vector<PresetRecommendation> recommendations;
    
    // Get cached similarities if available
    {
        std::lock_guard<std::mutex> lock(precomputeMutex_);
        auto it = similarityCache_.find(referencePreset);
        if (it != similarityCache_.end()) {
            for (const auto& [presetPath, similarity] : it->second) {
                if (static_cast<int>(recommendations.size()) >= maxResults) break;
                
                PresetRecommendation rec;
                rec.presetPath = presetPath;
                rec.relevanceScore = similarity;
                rec.confidenceScore = similarity;
                rec.noveltyScore = calculateNoveltyScore(presetPath, interactionHistory_);
                rec.recommendationType = "similar";
                rec.sourcePresets = {referencePreset};
                rec.explanation.primary = "Similar audio characteristics to " + referencePreset;
                rec.explanation.algorithm = "content_similarity";
                
                recommendations.push_back(rec);
            }
        }
    }
    
    // If no cached similarities, compute on-demand (simplified for demo)
    if (recommendations.empty()) {
        // This would normally use the ML analyzer to find similar presets
        // For now, create some mock recommendations
        for (int i = 0; i < maxResults && i < 5; ++i) {
            PresetRecommendation rec;
            rec.presetPath = "similar_preset_" + std::to_string(i) + ".preset";
            rec.relevanceScore = 0.8f - (i * 0.1f); // Decreasing similarity
            rec.confidenceScore = rec.relevanceScore;
            rec.noveltyScore = 0.5f;
            rec.recommendationType = "similar";
            rec.sourcePresets = {referencePreset};
            rec.explanation.primary = "Similar timbral characteristics";
            rec.explanation.algorithm = "content_similarity";
            
            recommendations.push_back(rec);
        }
    }
    
    // Apply diversification if requested
    if (diversityWeight > 0.0f) {
        recommendations = applyDiversification(recommendations, diversityWeight);
    }
    
    updateStatistics(recommendations, "similarity");
    return recommendations;
}

std::vector<PresetRecommendation> PresetRecommendationEngine::getDiscoveryRecommendations(
    float explorationLevel,
    int maxResults) {
    
    std::vector<PresetRecommendation> recommendations;
    
    // Generate discovery recommendations based on exploration level
    // Higher exploration level = more novel/different presets
    
    for (int i = 0; i < maxResults; ++i) {
        PresetRecommendation rec;
        rec.presetPath = "discovery_preset_" + std::to_string(i) + ".preset";
        rec.relevanceScore = 0.6f + (explorationLevel * 0.3f); // Base relevance with exploration boost
        rec.confidenceScore = 0.7f - (explorationLevel * 0.2f); // Lower confidence for more experimental
        rec.noveltyScore = 0.3f + (explorationLevel * 0.7f); // Higher novelty with more exploration
        rec.recommendationType = "discovery";
        rec.explanation.primary = explorationLevel > 0.7f ? 
            "Experimental preset you haven't tried" : 
            "Popular preset outside your usual preferences";
        rec.explanation.algorithm = "discovery";
        
        recommendations.push_back(rec);
    }
    
    updateStatistics(recommendations, "discovery");
    return recommendations;
}

std::vector<PresetRecommendation> PresetRecommendationEngine::getWorkflowRecommendations(
    const std::vector<std::string>& currentWorkflow,
    int maxResults) {
    
    std::vector<PresetRecommendation> recommendations;
    
    // Analyze current workflow and suggest next presets based on learned patterns
    if (currentWorkflow.empty()) return recommendations;
    
    // For demo purposes, generate workflow-based recommendations
    for (int i = 0; i < maxResults && i < 3; ++i) {
        PresetRecommendation rec;
        rec.presetPath = "workflow_preset_" + std::to_string(i) + ".preset";
        rec.relevanceScore = 0.75f - (i * 0.1f);
        rec.confidenceScore = 0.8f;
        rec.noveltyScore = 0.4f;
        rec.recommendationType = "workflow";
        rec.sourcePresets = currentWorkflow;
        rec.explanation.primary = "Commonly used after presets in your current workflow";
        rec.explanation.algorithm = "workflow";
        
        recommendations.push_back(rec);
    }
    
    updateStatistics(recommendations, "workflow");
    return recommendations;
}

std::vector<PresetRecommendation> PresetRecommendationEngine::getTrendingRecommendations(
    int timeWindow,
    int maxResults) {
    
    std::vector<PresetRecommendation> recommendations;
    
    // Get trending presets based on recent popularity
    auto cutoffTime = std::chrono::system_clock::now() - std::chrono::hours(timeWindow);
    
    // Count interactions within time window
    std::unordered_map<std::string, int> recentInteractions;
    for (const auto& interaction : interactionHistory_) {
        if (interaction.timestamp >= cutoffTime) {
            recentInteractions[interaction.presetPath]++;
        }
    }
    
    // Convert to recommendations
    std::vector<std::pair<std::string, int>> sortedTrending;
    for (const auto& [preset, count] : recentInteractions) {
        sortedTrending.emplace_back(preset, count);
    }
    
    std::sort(sortedTrending.begin(), sortedTrending.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (int i = 0; i < maxResults && i < static_cast<int>(sortedTrending.size()); ++i) {
        const auto& [presetPath, interactionCount] = sortedTrending[i];
        
        PresetRecommendation rec;
        rec.presetPath = presetPath;
        rec.relevanceScore = std::min(1.0f, interactionCount / 10.0f); // Normalize interaction count
        rec.confidenceScore = 0.9f; // High confidence in trending data
        rec.noveltyScore = 0.6f; // Moderate novelty
        rec.recommendationType = "trending";
        rec.explanation.primary = "Popular preset with " + std::to_string(interactionCount) + " recent uses";
        rec.explanation.algorithm = "trending";
        
        recommendations.push_back(rec);
    }
    
    // If no trending data, create some mock trending recommendations
    if (recommendations.empty()) {
        for (int i = 0; i < maxResults && i < 5; ++i) {
            PresetRecommendation rec;
            rec.presetPath = "trending_preset_" + std::to_string(i) + ".preset";
            rec.relevanceScore = 0.8f - (i * 0.1f);
            rec.confidenceScore = 0.9f;
            rec.noveltyScore = 0.6f;
            rec.recommendationType = "trending";
            rec.explanation.primary = "Popular preset in the community";
            rec.explanation.algorithm = "trending";
            
            recommendations.push_back(rec);
        }
    }
    
    updateStatistics(recommendations, "trending");
    return recommendations;
}

void PresetRecommendationEngine::recordInteraction(const UserInteraction& interaction) {
    interactionHistory_.push_back(interaction);
    
    // Update popularity
    presetPopularity_[interaction.presetPath] += calculateInteractionWeight(interaction);
    
    // Trigger profile update if we have enough new interactions
    if (interactionHistory_.size() % 20 == 0) { // Update every 20 interactions
        updateUserProfile();
    }
}

void PresetRecommendationEngine::recordInteractions(const std::vector<UserInteraction>& interactions) {
    for (const auto& interaction : interactions) {
        recordInteraction(interaction);
    }
}

void PresetRecommendationEngine::provideFeedback(const std::string& recommendedPreset, 
                                                bool wasUseful, 
                                                int rating) {
    // Record feedback for algorithm improvement
    float feedbackScore = wasUseful ? 1.0f : 0.0f;
    if (rating > 0) {
        feedbackScore = rating / 5.0f; // Normalize to 0-1 scale
    }
    
    feedbackHistory_[recommendedPreset].push_back(feedbackScore);
    
    // Update statistics
    stats_.totalRecommendations++;
    if (wasUseful) {
        stats_.successfulRecommendations++;
    }
    
    stats_.averageUserRating = (stats_.averageUserRating + feedbackScore) / 2.0f;
}

void PresetRecommendationEngine::updateUserProfile() {
    auto now = std::chrono::system_clock::now();
    
    // Learn from recent interactions (last 30 days)
    auto cutoff = now - std::chrono::hours(24 * 30);
    std::vector<UserInteraction> recentInteractions;
    std::copy_if(interactionHistory_.begin(), interactionHistory_.end(),
                 std::back_inserter(recentInteractions),
                 [cutoff](const UserInteraction& interaction) {
                     return interaction.timestamp >= cutoff;
                 });
    
    if (recentInteractions.size() >= 5) { // Need minimum interactions for reliable learning
        learnFeaturePreferences();
        learnCategoryPreferences();
        learnTemporalPatterns();
        learnWorkflowPatterns();
    }
    
    userProfile_.lastUpdated = now;
}

// Private implementation methods

std::vector<PresetRecommendation> PresetRecommendationEngine::contentBasedRecommendations(
    const RecommendationContext& context) {
    
    std::vector<PresetRecommendation> recommendations;
    
    // Content-based filtering using audio features and user profile
    if (!context.currentPreset.empty()) {
        // Get similar presets to current preset
        auto similarPresets = getSimilarPresets(context.currentPreset, context.maxRecommendations * 2, 0.1f);
        
        // Score each preset based on user profile
        for (auto& rec : similarPresets) {
            // This would normally extract features and calculate profile match
            rec.userProfileMatch = 0.7f; // Mock profile match score
            rec.relevanceScore *= (0.5f + rec.userProfileMatch * 0.5f); // Boost based on profile match
            recommendations.push_back(rec);
        }
    }
    
    return recommendations;
}

std::vector<PresetRecommendation> PresetRecommendationEngine::collaborativeFilteringRecommendations(
    const RecommendationContext& context) {
    
    std::vector<PresetRecommendation> recommendations;
    
    // Collaborative filtering based on users with similar preferences
    // This is a simplified implementation - real collaborative filtering would
    // require multiple users' interaction data
    
    // For demo, generate recommendations based on interaction patterns
    std::unordered_map<std::string, float> presetScores;
    
    for (const auto& interaction : interactionHistory_) {
        if (interaction.type == UserInteraction::Type::Favorite ||
            interaction.type == UserInteraction::Type::Load) {
            presetScores[interaction.presetPath] += calculateInteractionWeight(interaction);
        }
    }
    
    // Sort by score and create recommendations
    std::vector<std::pair<std::string, float>> sortedPresets;
    for (const auto& [preset, score] : presetScores) {
        sortedPresets.emplace_back(preset, score);
    }
    
    std::sort(sortedPresets.begin(), sortedPresets.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (int i = 0; i < context.maxRecommendations && i < static_cast<int>(sortedPresets.size()); ++i) {
        const auto& [presetPath, score] = sortedPresets[i];
        
        PresetRecommendation rec;
        rec.presetPath = presetPath;
        rec.relevanceScore = std::min(1.0f, score / 5.0f); // Normalize score
        rec.confidenceScore = 0.8f;
        rec.noveltyScore = calculateNoveltyScore(presetPath, interactionHistory_);
        rec.recommendationType = "collaborative";
        rec.explanation.primary = "Based on your listening patterns";
        
        recommendations.push_back(rec);
    }
    
    return recommendations;
}

void PresetRecommendationEngine::learnFeaturePreferences() {
    // Analyze user interactions to learn audio feature preferences
    std::unordered_map<std::string, std::vector<float>> featureValues;
    std::unordered_map<std::string, std::vector<float>> featureWeights;
    
    for (const auto& interaction : interactionHistory_) {
        float weight = calculateInteractionWeight(interaction);
        
        // This would normally extract features from the preset
        // For demo, create mock feature preferences
        for (int i = 0; i < 20; ++i) {
            std::string featureKey = "feature_" + std::to_string(i);
            featureValues[featureKey].push_back(0.5f); // Mock feature value
            featureWeights[featureKey].push_back(weight);
        }
    }
    
    // Calculate weighted average preferences
    for (const auto& [featureKey, values] : featureValues) {
        const auto& weights = featureWeights[featureKey];
        
        float weightedSum = 0.0f;
        float totalWeight = 0.0f;
        
        for (size_t i = 0; i < values.size(); ++i) {
            weightedSum += values[i] * weights[i];
            totalWeight += weights[i];
        }
        
        if (totalWeight > 0.0f) {
            userProfile_.featurePreferences[featureKey] = weightedSum / totalWeight;
        }
    }
}

void PresetRecommendationEngine::learnCategoryPreferences() {
    std::unordered_map<std::string, float> categoryScores;
    std::unordered_map<std::string, float> categoryWeights;
    
    for (const auto& interaction : interactionHistory_) {
        float weight = calculateInteractionWeight(interaction);
        
        // This would normally get the category from preset metadata
        std::string category = "Unknown"; // Mock category
        
        categoryScores[category] += weight;
        categoryWeights[category] += 1.0f;
    }
    
    // Normalize category preferences
    for (const auto& [category, score] : categoryScores) {
        float weight = categoryWeights[category];
        userProfile_.categoryPreferences[category] = score / weight;
    }
}

void PresetRecommendationEngine::learnTemporalPatterns() {
    std::unordered_map<std::string, float> timeContextScores;
    
    for (const auto& interaction : interactionHistory_) {
        std::string timeContext = getTimeContext(interaction.timestamp);
        float weight = calculateInteractionWeight(interaction);
        
        timeContextScores[timeContext] += weight;
    }
    
    // Normalize temporal preferences
    float totalScore = std::accumulate(timeContextScores.begin(), timeContextScores.end(), 0.0f,
        [](float sum, const auto& pair) { return sum + pair.second; });
    
    if (totalScore > 0.0f) {
        for (const auto& [timeContext, score] : timeContextScores) {
            userProfile_.timeContextPreferences[timeContext] = score / totalScore;
        }
    }
}

void PresetRecommendationEngine::learnWorkflowPatterns() {
    // Analyze sequences of preset usage to learn workflow patterns
    std::vector<std::vector<std::string>> sessions;
    
    // Group interactions by session ID
    std::unordered_map<int, std::vector<UserInteraction>> sessionGroups;
    for (const auto& interaction : interactionHistory_) {
        sessionGroups[interaction.sessionId].push_back(interaction);
    }
    
    // Extract preset sequences from each session
    for (const auto& [sessionId, sessionInteractions] : sessionGroups) {
        std::vector<std::string> presetSequence;
        for (const auto& interaction : sessionInteractions) {
            if (interaction.type == UserInteraction::Type::Load ||
                interaction.type == UserInteraction::Type::Select) {
                presetSequence.push_back(interaction.presetPath);
            }
        }
        
        if (presetSequence.size() >= 2) {
            sessions.push_back(presetSequence);
        }
    }
    
    // Find common workflow patterns (simplified)
    userProfile_.commonWorkflows = sessions; // Store all sessions for now
    
    // In a real implementation, this would use sequence mining algorithms
    // to find common patterns across sessions
}

std::vector<PresetRecommendation> PresetRecommendationEngine::applyDiversification(
    std::vector<PresetRecommendation> recommendations,
    float diversityWeight) {
    
    if (recommendations.empty() || diversityWeight <= 0.0f) {
        return recommendations;
    }
    
    std::vector<PresetRecommendation> diversified;
    std::vector<bool> selected(recommendations.size(), false);
    
    // Always include the top recommendation
    if (!recommendations.empty()) {
        diversified.push_back(recommendations[0]);
        selected[0] = true;
    }
    
    // Select remaining recommendations balancing relevance and diversity
    while (diversified.size() < recommendations.size()) {
        float bestScore = -1.0f;
        int bestIndex = -1;
        
        for (size_t i = 0; i < recommendations.size(); ++i) {
            if (selected[i]) continue;
            
            float relevanceScore = recommendations[i].relevanceScore;
            
            // Calculate diversity score (average distance from already selected items)
            float diversityScore = 0.0f;
            for (const auto& selectedRec : diversified) {
                // This would normally calculate feature-based distance
                // For demo, use simple string distance
                diversityScore += (recommendations[i].presetPath != selectedRec.presetPath) ? 1.0f : 0.0f;
            }
            diversityScore /= diversified.size();
            
            // Combine relevance and diversity
            float combinedScore = (1.0f - diversityWeight) * relevanceScore + diversityWeight * diversityScore;
            
            if (combinedScore > bestScore) {
                bestScore = combinedScore;
                bestIndex = i;
            }
        }
        
        if (bestIndex != -1) {
            diversified.push_back(recommendations[bestIndex]);
            selected[bestIndex] = true;
        } else {
            break; // No more recommendations to add
        }
    }
    
    return diversified;
}

float PresetRecommendationEngine::calculateNoveltyScore(const std::string& presetPath, 
                                                       const std::vector<UserInteraction>& userInteractions) {
    // Calculate how novel/unfamiliar this preset is to the user
    int interactionCount = 0;
    auto recentCutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * 7); // Last week
    
    for (const auto& interaction : userInteractions) {
        if (interaction.presetPath == presetPath && interaction.timestamp >= recentCutoff) {
            interactionCount++;
        }
    }
    
    // Higher novelty for presets with fewer recent interactions
    return std::max(0.0f, 1.0f - (interactionCount / 10.0f));
}

PresetRecommendation::Explanation PresetRecommendationEngine::generateExplanation(
    const PresetRecommendation& recommendation,
    const std::string& algorithm,
    const RecommendationContext& context) {
    
    PresetRecommendation::Explanation explanation;
    explanation.algorithm = algorithm;
    
    if (algorithm == "content_based") {
        explanation.primary = "Similar audio characteristics to your preferences";
        explanation.factors.push_back("Timbral similarity");
        explanation.factors.push_back("Harmonic content match");
    } else if (algorithm == "collaborative") {
        explanation.primary = "Popular among users with similar tastes";
        explanation.factors.push_back("User behavior patterns");
        explanation.factors.push_back("Community preferences");
    } else if (algorithm == "workflow") {
        explanation.primary = "Commonly used in similar musical contexts";
        explanation.factors.push_back("Session workflow patterns");
        explanation.factors.push_back("Sequential usage data");
    } else if (algorithm == "discovery") {
        explanation.primary = "New preset outside your usual preferences";
        explanation.factors.push_back("Exploration recommendation");
        explanation.factors.push_back("Serendipitous discovery");
    } else {
        explanation.primary = "Recommended based on multiple factors";
    }
    
    return explanation;
}

// Utility methods

std::string PresetRecommendationEngine::getTimeContext(const std::chrono::system_clock::time_point& timestamp) {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto tm = *std::localtime(&time_t);
    
    int hour = tm.tm_hour;
    if (hour >= 6 && hour < 12) return "morning";
    if (hour >= 12 && hour < 17) return "afternoon";
    if (hour >= 17 && hour < 22) return "evening";
    return "night";
}

float PresetRecommendationEngine::calculateInteractionWeight(const UserInteraction& interaction) {
    float baseWeight = interaction.value;
    
    // Weight by interaction type
    switch (interaction.type) {
        case UserInteraction::Type::Favorite: baseWeight *= 3.0f; break;
        case UserInteraction::Type::Rate: baseWeight *= 2.5f; break;
        case UserInteraction::Type::Load: baseWeight *= 2.0f; break;
        case UserInteraction::Type::Select: baseWeight *= 1.5f; break;
        case UserInteraction::Type::View: baseWeight *= 1.0f; break;
        case UserInteraction::Type::Skip: baseWeight *= 0.2f; break;
        default: break;
    }
    
    // Apply recency decay (interactions lose weight over time)
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - interaction.timestamp).count();
    float recencyWeight = std::exp(-age / (24.0f * 30.0f)); // Decay over 30 days
    
    return baseWeight * recencyWeight;
}

void PresetRecommendationEngine::initializeDefaults() {
    // Default algorithm weights
    algorithmWeights_["content_based"] = 0.4f;
    algorithmWeights_["collaborative"] = 0.3f;
    algorithmWeights_["workflow"] = 0.2f;
    algorithmWeights_["discovery"] = 0.1f;
    
    // Default recommendation parameters
    recommendationParams_["diversity_threshold"] = 0.3f;
    recommendationParams_["novelty_weight"] = 0.2f;
    recommendationParams_["recency_decay"] = 0.95f;
    recommendationParams_["min_confidence"] = 0.4f;
    
    // Initialize user profile defaults
    userProfile_.diversityPreference = 0.5f;
    userProfile_.explorationFactor = 0.3f;
}

void PresetRecommendationEngine::updateStatistics(
    const std::vector<PresetRecommendation>& recommendations,
    const std::string& algorithm) {
    
    stats_.algorithmUsage[algorithm]++;
    
    if (!recommendations.empty()) {
        float totalRelevance = 0.0f;
        for (const auto& rec : recommendations) {
            totalRelevance += rec.relevanceScore;
        }
        stats_.averageRelevanceScore = (stats_.averageRelevanceScore + 
                                      totalRelevance / recommendations.size()) / 2.0f;
    }
    
    stats_.lastUpdated = std::chrono::system_clock::now();
}

// Public interface methods

void PresetRecommendationEngine::setAlgorithmWeights(const std::unordered_map<std::string, float>& weights) {
    for (const auto& [algorithm, weight] : weights) {
        algorithmWeights_[algorithm] = weight;
    }
}

void PresetRecommendationEngine::setRecommendationParameters(const std::unordered_map<std::string, float>& params) {
    for (const auto& [param, value] : params) {
        recommendationParams_[param] = value;
    }
}

void PresetRecommendationEngine::setMinimumInteractions(int minInteractions) {
    minimumInteractions_ = std::max(1, minInteractions);
}

PresetRecommendationEngine::RecommendationStats PresetRecommendationEngine::getStatistics() const {
    return stats_;
}

UserProfile PresetRecommendationEngine::getUserProfile() const {
    return userProfile_;
}

nlohmann::json PresetRecommendationEngine::exportUserData() const {
    nlohmann::json data;
    
    // Export user profile
    data["profile"]["diversityPreference"] = userProfile_.diversityPreference;
    data["profile"]["explorationFactor"] = userProfile_.explorationFactor;
    
    // Export interaction history (limited to recent interactions for privacy)
    auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * 90); // 90 days
    for (const auto& interaction : interactionHistory_) {
        if (interaction.timestamp >= cutoff) {
            nlohmann::json interactionJson;
            interactionJson["presetPath"] = interaction.presetPath;
            interactionJson["type"] = static_cast<int>(interaction.type);
            interactionJson["value"] = interaction.value;
            interactionJson["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                interaction.timestamp.time_since_epoch()).count();
            data["interactions"].push_back(interactionJson);
        }
    }
    
    return data;
}

bool PresetRecommendationEngine::importUserData(const nlohmann::json& userData) {
    try {
        // Import user profile
        if (userData.contains("profile")) {
            const auto& profile = userData["profile"];
            if (profile.contains("diversityPreference")) {
                userProfile_.diversityPreference = profile["diversityPreference"];
            }
            if (profile.contains("explorationFactor")) {
                userProfile_.explorationFactor = profile["explorationFactor"];
            }
        }
        
        // Import interactions
        if (userData.contains("interactions")) {
            interactionHistory_.clear();
            for (const auto& interactionJson : userData["interactions"]) {
                UserInteraction interaction(
                    interactionJson["presetPath"],
                    static_cast<UserInteraction::Type>(interactionJson["type"]),
                    interactionJson["value"]
                );
                interaction.timestamp = std::chrono::system_clock::time_point(
                    std::chrono::seconds(interactionJson["timestamp"]));
                interactionHistory_.push_back(interaction);
            }
        }
        
        // Update profile based on imported data
        updateUserProfile();
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// Placeholder implementations for precomputation methods
void PresetRecommendationEngine::precomputeSimilarities(
    const std::vector<PresetInfo>& presets,
    std::function<void(int, int)> progressCallback) {
    
    // This would precompute similarity matrices for all presets
    // For demo purposes, just report progress
    if (progressCallback) {
        for (size_t i = 0; i < presets.size(); ++i) {
            progressCallback(static_cast<int>(i), static_cast<int>(presets.size()));
        }
    }
}

void PresetRecommendationEngine::updatePrecomputedData(const std::vector<PresetInfo>& newPresets) {
    // This would update precomputed similarities with new presets
}

void PresetRecommendationEngine::clearPrecomputedData() {
    std::lock_guard<std::mutex> lock(precomputeMutex_);
    similarityCache_.clear();
    featureCache_.clear();
}

} // namespace AIMusicHardware