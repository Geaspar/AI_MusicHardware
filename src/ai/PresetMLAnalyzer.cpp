#include "ai/PresetMLAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <random>

namespace AIMusicHardware {

// AudioFeatureVector implementation

float AudioFeatureVector::calculateDistance(const AudioFeatureVector& other, 
                                           const std::unordered_map<std::string, float>& weights) const {
    float distance = 0.0f;
    float totalWeight = 0.0f;
    
    // Default weights if none provided
    auto getWeight = [&weights](const std::string& category, float defaultWeight) {
        auto it = weights.find(category);
        return it != weights.end() ? it->second : defaultWeight;
    };
    
    // Spectral features distance
    float spectralWeight = getWeight("spectral", 0.3f);
    float spectralDist = 0.0f;
    
    // Chroma vector distance
    for (size_t i = 0; i < chromaVector.size(); ++i) {
        float diff = chromaVector[i] - other.chromaVector[i];
        spectralDist += diff * diff;
    }
    
    // MFCC distance
    for (size_t i = 0; i < mfccVector.size(); ++i) {
        float diff = mfccVector[i] - other.mfccVector[i];
        spectralDist += diff * diff;
    }
    
    // Spectral moments distance
    for (size_t i = 0; i < spectralMoments.size(); ++i) {
        float diff = spectralMoments[i] - other.spectralMoments[i];
        spectralDist += diff * diff;
    }
    
    distance += spectralWeight * std::sqrt(spectralDist);
    totalWeight += spectralWeight;
    
    // Temporal features distance
    float temporalWeight = getWeight("temporal", 0.2f);
    float temporalDist = 0.0f;
    
    temporalDist += (tempo - other.tempo) * (tempo - other.tempo);
    temporalDist += (rhythmComplexity - other.rhythmComplexity) * (rhythmComplexity - other.rhythmComplexity);
    temporalDist += (attackTime - other.attackTime) * (attackTime - other.attackTime);
    temporalDist += (releaseTime - other.releaseTime) * (releaseTime - other.releaseTime);
    
    distance += temporalWeight * std::sqrt(temporalDist);
    totalWeight += temporalWeight;
    
    // Timbral features distance
    float timbralWeight = getWeight("timbral", 0.25f);
    float timbralDist = 0.0f;
    
    timbralDist += (brightness - other.brightness) * (brightness - other.brightness);
    timbralDist += (warmth - other.warmth) * (warmth - other.warmth);
    timbralDist += (roughness - other.roughness) * (roughness - other.roughness);
    timbralDist += (sharpness - other.sharpness) * (sharpness - other.sharpness);
    
    distance += timbralWeight * std::sqrt(timbralDist);
    totalWeight += timbralWeight;
    
    // Energy distribution distance
    float energyWeight = getWeight("energy", 0.15f);
    float energyDist = 0.0f;
    
    for (size_t i = 0; i < energyBands.size(); ++i) {
        float diff = energyBands[i] - other.energyBands[i];
        energyDist += diff * diff;
    }
    energyDist += (totalEnergy - other.totalEnergy) * (totalEnergy - other.totalEnergy);
    energyDist += (dynamicRange - other.dynamicRange) * (dynamicRange - other.dynamicRange);
    
    distance += energyWeight * std::sqrt(energyDist);
    totalWeight += energyWeight;
    
    // Synthesis features distance
    float synthesisWeight = getWeight("synthesis", 0.1f);
    float synthesisDist = 0.0f;
    
    synthesisDist += (oscillatorComplexity - other.oscillatorComplexity) * (oscillatorComplexity - other.oscillatorComplexity);
    synthesisDist += (filterResonance - other.filterResonance) * (filterResonance - other.filterResonance);
    synthesisDist += (effectsComplexity - other.effectsComplexity) * (effectsComplexity - other.effectsComplexity);
    
    distance += synthesisWeight * std::sqrt(synthesisDist);
    totalWeight += synthesisWeight;
    
    return totalWeight > 0.0f ? distance / totalWeight : 0.0f;
}

void AudioFeatureVector::normalize() {
    // Normalize each feature category to [0, 1] range
    
    // Normalize chroma (should already be normalized, but ensure it)
    float chromaSum = 0.0f;
    for (float& val : chromaVector) {
        chromaSum += val;
    }
    if (chromaSum > 0.0f) {
        for (float& val : chromaVector) {
            val /= chromaSum;
        }
    }
    
    // Normalize MFCC (clip to reasonable range)
    for (float& val : mfccVector) {
        val = std::clamp(val / 100.0f, -1.0f, 1.0f);
    }
    
    // Normalize spectral moments
    for (float& val : spectralMoments) {
        val = std::clamp(val, 0.0f, 1.0f);
    }
    
    // Normalize temporal features
    tempo = std::clamp(tempo / 200.0f, 0.0f, 1.0f);
    rhythmComplexity = std::clamp(rhythmComplexity, 0.0f, 1.0f);
    attackTime = std::clamp(attackTime, 0.0f, 1.0f);
    releaseTime = std::clamp(releaseTime, 0.0f, 1.0f);
    
    // Normalize timbral features
    brightness = std::clamp(brightness, 0.0f, 1.0f);
    warmth = std::clamp(warmth, 0.0f, 1.0f);
    roughness = std::clamp(roughness, 0.0f, 1.0f);
    sharpness = std::clamp(sharpness, 0.0f, 1.0f);
    
    // Normalize energy bands
    float maxEnergy = *std::max_element(energyBands.begin(), energyBands.end());
    if (maxEnergy > 0.0f) {
        for (float& val : energyBands) {
            val /= maxEnergy;
        }
    }
    
    totalEnergy = std::clamp(totalEnergy / 100.0f, 0.0f, 1.0f);
    dynamicRange = std::clamp(dynamicRange / 60.0f, 0.0f, 1.0f); // 60dB max range
}

std::vector<float> AudioFeatureVector::toDenseVector() const {
    std::vector<float> dense;
    dense.reserve(100); // Approximate size
    
    // Add all features to dense vector
    dense.insert(dense.end(), chromaVector.begin(), chromaVector.end());
    dense.insert(dense.end(), mfccVector.begin(), mfccVector.end());
    dense.insert(dense.end(), spectralMoments.begin(), spectralMoments.end());
    
    dense.push_back(tempo);
    dense.push_back(rhythmComplexity);
    dense.push_back(attackTime);
    dense.push_back(releaseTime);
    
    dense.push_back(harmonicity);
    dense.push_back(fundamentalFrequency);
    dense.push_back(inharmonicity);
    
    dense.push_back(brightness);
    dense.push_back(warmth);
    dense.push_back(roughness);
    dense.push_back(sharpness);
    
    dense.insert(dense.end(), energyBands.begin(), energyBands.end());
    dense.push_back(totalEnergy);
    dense.push_back(dynamicRange);
    
    dense.push_back(lfoDepth);
    dense.push_back(filterMovement);
    dense.push_back(amplitudeModulation);
    dense.push_back(frequencyModulation);
    
    dense.push_back(oscillatorComplexity);
    dense.push_back(filterResonance);
    dense.push_back(effectsComplexity);
    dense.push_back(voiceCount);
    
    return dense;
}

void AudioFeatureVector::fromDenseVector(const std::vector<float>& denseVector) {
    if (denseVector.size() < 60) return; // Minimum expected size
    
    size_t idx = 0;
    
    // Extract features in same order as toDenseVector
    std::copy_n(denseVector.begin() + idx, chromaVector.size(), chromaVector.begin());
    idx += chromaVector.size();
    
    std::copy_n(denseVector.begin() + idx, mfccVector.size(), mfccVector.begin());
    idx += mfccVector.size();
    
    std::copy_n(denseVector.begin() + idx, spectralMoments.size(), spectralMoments.begin());
    idx += spectralMoments.size();
    
    if (idx < denseVector.size()) tempo = denseVector[idx++];
    if (idx < denseVector.size()) rhythmComplexity = denseVector[idx++];
    if (idx < denseVector.size()) attackTime = denseVector[idx++];
    if (idx < denseVector.size()) releaseTime = denseVector[idx++];
    
    if (idx < denseVector.size()) harmonicity = denseVector[idx++];
    if (idx < denseVector.size()) fundamentalFrequency = denseVector[idx++];
    if (idx < denseVector.size()) inharmonicity = denseVector[idx++];
    
    if (idx < denseVector.size()) brightness = denseVector[idx++];
    if (idx < denseVector.size()) warmth = denseVector[idx++];
    if (idx < denseVector.size()) roughness = denseVector[idx++];
    if (idx < denseVector.size()) sharpness = denseVector[idx++];
    
    if (idx + energyBands.size() <= denseVector.size()) {
        std::copy_n(denseVector.begin() + idx, energyBands.size(), energyBands.begin());
        idx += energyBands.size();
    }
    
    if (idx < denseVector.size()) totalEnergy = denseVector[idx++];
    if (idx < denseVector.size()) dynamicRange = denseVector[idx++];
    
    if (idx < denseVector.size()) lfoDepth = denseVector[idx++];
    if (idx < denseVector.size()) filterMovement = denseVector[idx++];
    if (idx < denseVector.size()) amplitudeModulation = denseVector[idx++];
    if (idx < denseVector.size()) frequencyModulation = denseVector[idx++];
    
    if (idx < denseVector.size()) oscillatorComplexity = denseVector[idx++];
    if (idx < denseVector.size()) filterResonance = denseVector[idx++];
    if (idx < denseVector.size()) effectsComplexity = denseVector[idx++];
    if (idx < denseVector.size()) voiceCount = denseVector[idx++];
}

// PresetMLAnalyzer implementation

PresetMLAnalyzer::PresetMLAnalyzer() {
    initializeDefaultCategories();
    initializeDefaultWeights();
    initializeDefaultParameters();
}

PresetMLAnalyzer::~PresetMLAnalyzer() = default;

AudioFeatureVector PresetMLAnalyzer::extractFeatures(const PresetInfo& preset) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Check cache first
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto it = featureCache_.find(preset.filePath);
        if (it != featureCache_.end()) {
            stats_.cacheHits++;
            return it->second;
        }
        stats_.cacheMisses++;
    }
    
    AudioFeatureVector features;
    
    // Extract different feature categories based on configuration
    if (useSpectralFeatures_) {
        auto spectral = extractSpectralFeatures(preset);
        // Merge spectral features
        features.chromaVector = spectral.chromaVector;
        features.mfccVector = spectral.mfccVector;
        features.spectralMoments = spectral.spectralMoments;
    }
    
    if (useTemporalFeatures_) {
        auto temporal = extractTemporalFeatures(preset);
        features.tempo = temporal.tempo;
        features.rhythmComplexity = temporal.rhythmComplexity;
        features.attackTime = temporal.attackTime;
        features.releaseTime = temporal.releaseTime;
    }
    
    if (useHarmonicFeatures_) {
        auto harmonic = extractHarmonicFeatures(preset);
        features.harmonicity = harmonic.harmonicity;
        features.fundamentalFrequency = harmonic.fundamentalFrequency;
        features.inharmonicity = harmonic.inharmonicity;
    }
    
    if (useSynthesisFeatures_) {
        auto synthesis = extractSynthesisFeatures(preset);
        features.brightness = synthesis.brightness;
        features.warmth = synthesis.warmth;
        features.roughness = synthesis.roughness;
        features.sharpness = synthesis.sharpness;
        features.energyBands = synthesis.energyBands;
        features.totalEnergy = synthesis.totalEnergy;
        features.dynamicRange = synthesis.dynamicRange;
        features.lfoDepth = synthesis.lfoDepth;
        features.filterMovement = synthesis.filterMovement;
        features.amplitudeModulation = synthesis.amplitudeModulation;
        features.frequencyModulation = synthesis.frequencyModulation;
        features.oscillatorComplexity = synthesis.oscillatorComplexity;
        features.filterResonance = synthesis.filterResonance;
        features.effectsComplexity = synthesis.effectsComplexity;
        features.voiceCount = synthesis.voiceCount;
    }
    
    // Normalize features
    features.normalize();
    
    // Cache the result
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        featureCache_[preset.filePath] = features;
    }
    
    // Update statistics
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats_.totalAnalyzed++;
    stats_.averageAnalysisTime = (stats_.averageAnalysisTime * (stats_.totalAnalyzed - 1) + 
                                 duration.count()) / stats_.totalAnalyzed;
    
    return features;
}

std::vector<PresetSimilarity> PresetMLAnalyzer::findSimilarPresets(
    const PresetInfo& reference,
    const std::vector<PresetInfo>& candidates,
    int maxResults,
    float minSimilarity) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    AudioFeatureVector refFeatures = extractFeatures(reference);
    std::vector<PresetSimilarity> similarities;
    similarities.reserve(candidates.size());
    
    for (const auto& candidate : candidates) {
        if (candidate.filePath == reference.filePath) continue; // Skip self
        
        AudioFeatureVector candFeatures = extractFeatures(candidate);
        float distance = refFeatures.calculateDistance(candFeatures, similarityWeights_);
        float similarity = 1.0f / (1.0f + distance); // Convert distance to similarity
        
        if (similarity >= minSimilarity) {
            PresetSimilarity sim;
            sim.presetPath = candidate.filePath;
            sim.similarityScore = similarity;
            sim.confidenceScore = std::min(1.0f, similarity * 1.2f); // Boost confidence slightly
            sim.features = candFeatures;
            
            // Generate similarity reason
            if (similarity > 0.8f) {
                sim.similarityReason = "Very similar timbral characteristics";
            } else if (similarity > 0.6f) {
                sim.similarityReason = "Similar harmonic content and brightness";
            } else if (similarity > 0.4f) {
                sim.similarityReason = "Similar synthesis approach";
            } else {
                sim.similarityReason = "Some shared characteristics";
            }
            
            similarities.push_back(sim);
        }
    }
    
    // Sort by similarity score (descending)
    std::sort(similarities.begin(), similarities.end());
    
    // Limit results
    if (static_cast<int>(similarities.size()) > maxResults) {
        similarities.resize(maxResults);
    }
    
    // Update statistics
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats_.averageSimilarityTime = (stats_.averageSimilarityTime + duration.count()) / 2.0f;
    
    return similarities;
}

PresetCategorization PresetMLAnalyzer::suggestCategory(const PresetInfo& preset) {
    AudioFeatureVector features = extractFeatures(preset);
    
    PresetCategorization result;
    result.confidence = 0.0f;
    
    // Simple rule-based categorization based on audio characteristics
    float bassiness = features.energyBands[0] + features.energyBands[1]; // Low frequency content
    float brightness = features.brightness;
    float complexity = features.oscillatorComplexity + features.effectsComplexity;
    float rhythmicity = features.rhythmComplexity;
    
    // Category scoring
    std::vector<std::pair<std::string, float>> categoryScores;
    
    // Bass category
    float bassScore = bassiness * 0.4f + (1.0f - brightness) * 0.3f + features.warmth * 0.3f;
    categoryScores.emplace_back("Bass", bassScore);
    
    // Lead category  
    float leadScore = brightness * 0.4f + complexity * 0.3f + features.sharpness * 0.3f;
    categoryScores.emplace_back("Lead", leadScore);
    
    // Pad category
    float padScore = features.warmth * 0.4f + (1.0f - rhythmicity) * 0.3f + 
                    features.attackTime * 0.3f;
    categoryScores.emplace_back("Pad", padScore);
    
    // Pluck category
    float pluckScore = (1.0f - features.attackTime) * 0.4f + brightness * 0.3f + 
                      rhythmicity * 0.3f;
    categoryScores.emplace_back("Pluck", pluckScore);
    
    // Keys category
    float keysScore = features.harmonicity * 0.4f + (brightness + features.warmth) * 0.3f + 
                     (1.0f - complexity) * 0.3f;
    categoryScores.emplace_back("Keys", keysScore);
    
    // Sort by score
    std::sort(categoryScores.begin(), categoryScores.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Set result
    if (!categoryScores.empty()) {
        result.suggestedCategory = categoryScores[0].first;
        result.confidence = categoryScores[0].second;
        
        // Add alternatives
        for (size_t i = 1; i < std::min(categoryScores.size(), size_t(3)); ++i) {
            result.alternativeCategories.push_back(categoryScores[i]);
        }
        
        // Generate reasoning
        result.reasoning = "Based on timbral analysis: " + result.suggestedCategory + 
                          " characteristics detected with " + 
                          std::to_string(int(result.confidence * 100)) + "% confidence";
    }
    
    return result;
}

std::vector<std::pair<std::string, float>> PresetMLAnalyzer::suggestTags(const PresetInfo& preset) {
    AudioFeatureVector features = extractFeatures(preset);
    std::vector<std::pair<std::string, float>> tags;
    
    // Combine different tag categories
    auto genreTags = analyzeGenreTags(features);
    auto moodTags = analyzeMoodTags(features);
    auto instrumentTags = analyzeInstrumentTags(features);
    auto technicalTags = analyzeTechnicalTags(features);
    
    tags.insert(tags.end(), genreTags.begin(), genreTags.end());
    tags.insert(tags.end(), moodTags.begin(), moodTags.end());
    tags.insert(tags.end(), instrumentTags.begin(), instrumentTags.end());
    tags.insert(tags.end(), technicalTags.begin(), technicalTags.end());
    
    // Sort by relevance and limit
    std::sort(tags.begin(), tags.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (tags.size() > 10) {
        tags.resize(10);
    }
    
    return tags;
}

std::vector<PresetSimilarity> PresetMLAnalyzer::detectDuplicates(
    const PresetInfo& preset,
    const std::vector<PresetInfo>& existingPresets,
    float threshold) {
    
    std::vector<PresetSimilarity> duplicates;
    AudioFeatureVector refFeatures = extractFeatures(preset);
    
    for (const auto& existing : existingPresets) {
        if (existing.filePath == preset.filePath) continue;
        
        AudioFeatureVector existingFeatures = extractFeatures(existing);
        float distance = refFeatures.calculateDistance(existingFeatures, similarityWeights_);
        float similarity = 1.0f / (1.0f + distance);
        
        if (similarity >= threshold) {
            PresetSimilarity dup;
            dup.presetPath = existing.filePath;
            dup.similarityScore = similarity;
            dup.confidenceScore = similarity;
            dup.features = existingFeatures;
            dup.similarityReason = similarity > 0.95f ? "Likely duplicate" : "Highly similar";
            
            duplicates.push_back(dup);
        }
    }
    
    std::sort(duplicates.begin(), duplicates.end());
    return duplicates;
}

// Private implementation methods

AudioFeatureVector PresetMLAnalyzer::extractSpectralFeatures(const PresetInfo& preset) {
    AudioFeatureVector features;
    
    // Simulate spectral analysis based on oscillator and filter parameters
    if (preset.parameterData.contains("osc1_waveform")) {
        int waveform = preset.parameterData["osc1_waveform"].get<int>();
        
        // Generate chroma vector based on waveform
        for (size_t i = 0; i < features.chromaVector.size(); ++i) {
            // Simulate harmonic content based on waveform type
            switch (waveform) {
                case 0: // Saw
                    features.chromaVector[i] = 1.0f / (i + 1); // Rich harmonics
                    break;
                case 1: // Square
                    features.chromaVector[i] = (i % 2 == 0) ? 1.0f / (i + 1) : 0.0f; // Odd harmonics
                    break;
                case 2: // Triangle
                    features.chromaVector[i] = 1.0f / ((i + 1) * (i + 1)); // Softer harmonics
                    break;
                default: // Sine
                    features.chromaVector[i] = (i == 0) ? 1.0f : 0.0f; // Fundamental only
                    break;
            }
        }
    }
    
    // Generate MFCC-like features based on filter parameters
    if (preset.parameterData.contains("filter_cutoff")) {
        float cutoff = preset.parameterData["filter_cutoff"].get<float>();
        for (size_t i = 0; i < features.mfccVector.size(); ++i) {
            features.mfccVector[i] = std::cos(static_cast<float>(i) * cutoff * 0.01f);
        }
    }
    
    // Generate spectral moments
    features.spectralMoments[0] = preset.audioCharacteristics.brightness; // Centroid
    features.spectralMoments[1] = 0.5f; // Spread
    features.spectralMoments[2] = 0.0f; // Skewness
    features.spectralMoments[3] = 0.0f; // Kurtosis
    
    return features;
}

AudioFeatureVector PresetMLAnalyzer::extractTemporalFeatures(const PresetInfo& preset) {
    AudioFeatureVector features;
    
    // Extract envelope-based temporal features
    if (preset.parameterData.contains("env_attack")) {
        features.attackTime = preset.parameterData["env_attack"].get<float>();
    }
    
    if (preset.parameterData.contains("env_release")) {
        features.releaseTime = preset.parameterData["env_release"].get<float>();
    }
    
    // Estimate tempo and rhythm complexity from LFO and sequencer data
    if (preset.parameterData.contains("lfo_rate")) {
        float lfoRate = preset.parameterData["lfo_rate"].get<float>();
        features.tempo = lfoRate * 120.0f; // Convert to BPM estimate
    }
    
    features.rhythmComplexity = preset.audioCharacteristics.hasSequencer ? 0.8f : 0.2f;
    
    return features;
}

AudioFeatureVector PresetMLAnalyzer::extractHarmonicFeatures(const PresetInfo& preset) {
    AudioFeatureVector features;
    
    // Estimate harmonic content from oscillator setup
    float harmonic_content = 0.0f;
    if (preset.parameterData.contains("osc1_waveform")) {
        int waveform = preset.parameterData["osc1_waveform"];
        switch (waveform) {
            case 0: harmonic_content = 0.9f; break; // Saw - very harmonic
            case 1: harmonic_content = 0.7f; break; // Square - moderately harmonic
            case 2: harmonic_content = 0.5f; break; // Triangle - some harmonics
            default: harmonic_content = 0.1f; break; // Sine - minimal harmonics
        }
    }
    
    features.harmonicity = harmonic_content;
    features.fundamentalFrequency = 440.0f; // Default A4
    features.inharmonicity = 1.0f - harmonic_content;
    
    return features;
}

AudioFeatureVector PresetMLAnalyzer::extractSynthesisFeatures(const PresetInfo& preset) {
    AudioFeatureVector features;
    
    // Use existing audio characteristics
    features.brightness = preset.audioCharacteristics.brightness;
    features.warmth = preset.audioCharacteristics.warmth;
    features.roughness = 0.5f; // Default
    features.sharpness = preset.audioCharacteristics.brightness;
    
    // Analyze synthesis complexity
    features.oscillatorComplexity = analyzeOscillatorComplexity(preset.parameterData);
    features.filterResonance = analyzeFilterCharacteristics(preset.parameterData);
    features.effectsComplexity = analyzeEffectsProcessing(preset.parameterData);
    
    // Simulate energy distribution
    float bassWeight = preset.audioCharacteristics.bassContent;
    float midWeight = preset.audioCharacteristics.midContent;
    float trebleWeight = preset.audioCharacteristics.trebleContent;
    
    for (size_t i = 0; i < features.energyBands.size(); ++i) {
        float bandPos = static_cast<float>(i) / features.energyBands.size();
        if (bandPos < 0.3f) {
            features.energyBands[i] = bassWeight;
        } else if (bandPos < 0.7f) {
            features.energyBands[i] = midWeight;
        } else {
            features.energyBands[i] = trebleWeight;
        }
    }
    
    features.totalEnergy = bassWeight + midWeight + trebleWeight;
    features.dynamicRange = preset.audioCharacteristics.complexity * 60.0f; // Convert to dB
    
    // Modulation analysis
    features.lfoDepth = analyzeModulationDepth(preset.parameterData);
    features.filterMovement = features.lfoDepth * 0.5f;
    features.amplitudeModulation = features.lfoDepth * 0.3f;
    features.frequencyModulation = features.lfoDepth * 0.4f;
    
    features.voiceCount = 1.0f; // Default monophonic
    
    return features;
}

// Analysis helper methods

float PresetMLAnalyzer::analyzeOscillatorComplexity(const nlohmann::json& parameters) {
    float complexity = 0.0f;
    int oscCount = 0;
    
    // Count active oscillators and their complexity
    for (int i = 1; i <= 3; ++i) {
        std::string oscKey = "osc" + std::to_string(i) + "_enabled";
        if (parameters.contains(oscKey) && parameters[oscKey].get<bool>()) {
            oscCount++;
            
            // Add complexity based on waveform type
            std::string waveKey = "osc" + std::to_string(i) + "_waveform";
            if (parameters.contains(waveKey)) {
                int waveform = parameters[waveKey].get<int>();
                complexity += (waveform == 0) ? 0.8f : // Saw
                              (waveform == 1) ? 0.6f : // Square
                              (waveform == 2) ? 0.4f : // Triangle
                              0.2f; // Sine
            }
        }
    }
    
    return std::min(1.0f, complexity + oscCount * 0.2f);
}

float PresetMLAnalyzer::analyzeFilterCharacteristics(const nlohmann::json& parameters) {
    float resonance = 0.0f;
    
    if (parameters.contains("filter_resonance")) {
        resonance = parameters["filter_resonance"].get<float>();
    }
    
    return std::clamp(resonance, 0.0f, 1.0f);
}

float PresetMLAnalyzer::analyzeEnvelopeComplexity(const nlohmann::json& parameters) {
    float complexity = 0.0f;
    
    // Sum envelope parameters for complexity measure
    std::vector<std::string> envParams = {
        "env_attack", "env_decay", "env_sustain", "env_release"
    };
    
    for (const auto& param : envParams) {
        if (parameters.contains(param)) {
            complexity += parameters[param].get<float>();
        }
    }
    
    return std::clamp(complexity / 4.0f, 0.0f, 1.0f);
}

float PresetMLAnalyzer::analyzeModulationDepth(const nlohmann::json& parameters) {
    float depth = 0.0f;
    
    if (parameters.contains("lfo_depth")) {
        depth = parameters["lfo_depth"].get<float>();
    }
    
    return std::clamp(depth, 0.0f, 1.0f);
}

float PresetMLAnalyzer::analyzeEffectsProcessing(const nlohmann::json& parameters) {
    float complexity = 0.0f;
    int effectCount = 0;
    
    // Check for various effects
    std::vector<std::string> effects = {
        "reverb_enabled", "delay_enabled", "chorus_enabled", 
        "distortion_enabled", "filter_enabled"
    };
    
    for (const auto& effect : effects) {
        if (parameters.contains(effect) && parameters[effect].get<bool>()) {
            effectCount++;
            complexity += 0.2f;
        }
    }
    
    return std::min(1.0f, complexity);
}

// Tag analysis methods

std::vector<std::pair<std::string, float>> PresetMLAnalyzer::analyzeGenreTags(const AudioFeatureVector& features) {
    std::vector<std::pair<std::string, float>> tags;
    
    float bassiness = features.energyBands[0] + features.energyBands[1];
    float brightness = features.brightness;
    float complexity = features.oscillatorComplexity;
    
    if (bassiness > 0.7f) tags.emplace_back("Electronic", bassiness);
    if (brightness > 0.8f) tags.emplace_back("Trance", brightness);
    if (complexity > 0.6f) tags.emplace_back("Experimental", complexity);
    if (features.warmth > 0.7f) tags.emplace_back("Ambient", features.warmth);
    
    return tags;
}

std::vector<std::pair<std::string, float>> PresetMLAnalyzer::analyzeMoodTags(const AudioFeatureVector& features) {
    std::vector<std::pair<std::string, float>> tags;
    
    if (features.brightness > 0.7f) tags.emplace_back("Bright", features.brightness);
    if (features.warmth > 0.7f) tags.emplace_back("Warm", features.warmth);
    if (features.roughness > 0.6f) tags.emplace_back("Aggressive", features.roughness);
    if (features.attackTime > 0.5f) tags.emplace_back("Soft", features.attackTime);
    
    return tags;
}

std::vector<std::pair<std::string, float>> PresetMLAnalyzer::analyzeInstrumentTags(const AudioFeatureVector& features) {
    std::vector<std::pair<std::string, float>> tags;
    
    float bassiness = features.energyBands[0] + features.energyBands[1];
    
    if (bassiness > 0.8f) tags.emplace_back("Bass", bassiness);
    if (features.brightness > 0.8f) tags.emplace_back("Lead", features.brightness);
    if (features.attackTime > 0.6f) tags.emplace_back("Pad", features.attackTime);
    if (features.harmonicity > 0.7f) tags.emplace_back("Keys", features.harmonicity);
    
    return tags;
}

std::vector<std::pair<std::string, float>> PresetMLAnalyzer::analyzeTechnicalTags(const AudioFeatureVector& features) {
    std::vector<std::pair<std::string, float>> tags;
    
    if (features.lfoDepth > 0.5f) tags.emplace_back("Modulated", features.lfoDepth);
    if (features.filterResonance > 0.6f) tags.emplace_back("Filtered", features.filterResonance);
    if (features.effectsComplexity > 0.5f) tags.emplace_back("Processed", features.effectsComplexity);
    if (features.oscillatorComplexity > 0.7f) tags.emplace_back("Complex", features.oscillatorComplexity);
    
    return tags;
}

// Initialization methods

void PresetMLAnalyzer::initializeDefaultCategories() {
    categoryLabels_ = {"Bass", "Lead", "Pad", "Pluck", "Keys", "FX", "Drum", "Arp"};
}

void PresetMLAnalyzer::initializeDefaultWeights() {
    similarityWeights_["spectral"] = 0.3f;
    similarityWeights_["temporal"] = 0.2f;
    similarityWeights_["timbral"] = 0.25f;
    similarityWeights_["energy"] = 0.15f;
    similarityWeights_["synthesis"] = 0.1f;
}

void PresetMLAnalyzer::initializeDefaultParameters() {
    analysisParameters_["spectral_resolution"] = 1024.0f;
    analysisParameters_["temporal_window"] = 0.1f;
    analysisParameters_["harmonic_threshold"] = 0.5f;
    analysisParameters_["similarity_threshold"] = 0.3f;
}

// Public configuration methods

void PresetMLAnalyzer::setAnalysisParameters(const std::unordered_map<std::string, float>& params) {
    for (const auto& [key, value] : params) {
        analysisParameters_[key] = value;
    }
}

void PresetMLAnalyzer::setFeatureCategories(bool spectral, bool temporal, bool harmonic, bool synthesis) {
    useSpectralFeatures_ = spectral;
    useTemporalFeatures_ = temporal;
    useHarmonicFeatures_ = harmonic;
    useSynthesisFeatures_ = synthesis;
}

void PresetMLAnalyzer::setSimilarityWeights(const std::unordered_map<std::string, float>& weights) {
    similarityWeights_ = weights;
}

// Cache management

void PresetMLAnalyzer::cacheFeatures(const std::string& presetPath, const AudioFeatureVector& features) {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    featureCache_[presetPath] = features;
}

const AudioFeatureVector* PresetMLAnalyzer::getCachedFeatures(const std::string& presetPath) const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = featureCache_.find(presetPath);
    return (it != featureCache_.end()) ? &it->second : nullptr;
}

void PresetMLAnalyzer::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    featureCache_.clear();
}

// Statistics

PresetMLAnalyzer::AnalysisStats PresetMLAnalyzer::getStatistics() const {
    return stats_;
}

void PresetMLAnalyzer::resetStatistics() {
    stats_ = AnalysisStats{};
}

// Utility functions

std::vector<float> PresetMLAnalyzer::normalizeVector(const std::vector<float>& input) {
    std::vector<float> output = input;
    float sum = std::accumulate(output.begin(), output.end(), 0.0f);
    if (sum > 0.0f) {
        for (float& val : output) {
            val /= sum;
        }
    }
    return output;
}

float PresetMLAnalyzer::euclideanDistance(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) return std::numeric_limits<float>::max();
    
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

float PresetMLAnalyzer::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) return 0.0f;
    
    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    
    for (size_t i = 0; i < a.size(); ++i) {
        dotProduct += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    
    float norm = std::sqrt(normA * normB);
    return (norm > 0.0f) ? dotProduct / norm : 0.0f;
}

std::unordered_map<std::string, AudioFeatureVector> PresetMLAnalyzer::batchExtractFeatures(
    const std::vector<PresetInfo>& presets,
    std::function<void(int, int)> progressCallback) {

    std::unordered_map<std::string, AudioFeatureVector> results;

    for (size_t i = 0; i < presets.size(); ++i) {
        results[presets[i].filePath] = extractFeatures(presets[i]);

        if (progressCallback) {
            progressCallback(static_cast<int>(i + 1), static_cast<int>(presets.size()));
        }
    }

    return results;
}

} // namespace AIMusicHardware