#pragma once

#include "Parameter.h"
#include <string>
#include <map>
#include <memory>
#include <sstream>
#include <iostream>

namespace AIMusicHardware {

/**
 * @brief Group of related parameters
 * 
 * ParameterGroup provides hierarchical organization of parameters,
 * allowing parameters to be logically grouped and accessed by path.
 */
class ParameterGroup {
public:
    using GroupId = std::string;
    
    /**
     * @brief Constructor
     * 
     * @param id Unique identifier for the group
     * @param name Display name
     */
    ParameterGroup(const GroupId& id, const std::string& name)
        : id_(id), name_(name), parent_(nullptr) {
    }
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ParameterGroup() = default;
    
    // Group properties
    
    /**
     * @brief Get group ID
     * 
     * @return GroupId Unique identifier
     */
    GroupId getId() const { return id_; }
    
    /**
     * @brief Get display name
     * 
     * @return std::string Display name
     */
    std::string getName() const { return name_; }
    
    // Parameter management
    
    /**
     * @brief Create parameter of type T
     * 
     * @tparam T Parameter type (derived from Parameter)
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return T* Pointer to created parameter
     */
    template<typename T, typename... Args>
    T* createParameter(Args&&... args) {
        static_assert(std::is_base_of<Parameter, T>::value, 
                      "T must be derived from Parameter");
                      
        auto parameter = std::make_unique<T>(std::forward<Args>(args)...);
        T* paramPtr = parameter.get();
        
        // Register with manager (to be implemented)
        // ParameterManager::getInstance().registerParameter(paramPtr);
        
        // Add to our collection
        const auto& id = paramPtr->getId();
        parameters_[id] = std::move(parameter);
        
        return paramPtr;
    }
    
    /**
     * @brief Add existing parameter
     * 
     * @param parameter Parameter to add
     */
    void addParameter(std::unique_ptr<Parameter> parameter) {
        if (parameter) {
            const auto& id = parameter->getId();
            parameters_[id] = std::move(parameter);
        }
    }
    
    /**
     * @brief Get parameter by ID
     * 
     * @param id Parameter ID
     * @return Parameter* Pointer to parameter (nullptr if not found)
     */
    Parameter* getParameter(const Parameter::ParameterId& id) {
        auto it = parameters_.find(id);
        if (it != parameters_.end()) {
            return it->second.get();
        }
        return nullptr;
    }
    
    /**
     * @brief Remove parameter
     * 
     * @param id Parameter ID
     * @return true if parameter was removed
     */
    bool removeParameter(const Parameter::ParameterId& id) {
        return parameters_.erase(id) > 0;
    }
    
    // Nested groups
    
    /**
     * @brief Create and add a new group
     * 
     * @param id Group ID
     * @param name Display name
     * @return ParameterGroup* Pointer to created group
     */
    ParameterGroup* createGroup(const GroupId& id, const std::string& name) {
        auto group = std::make_unique<ParameterGroup>(id, name);
        ParameterGroup* groupPtr = group.get();
        
        // Set parent
        groupPtr->setParent(this);
        
        // Add to our collection
        groups_[id] = std::move(group);
        
        return groupPtr;
    }
    
    /**
     * @brief Add existing group
     * 
     * @param group Group to add
     */
    void addGroup(std::unique_ptr<ParameterGroup> group) {
        if (group) {
            const auto& id = group->getId();
            group->setParent(this);
            groups_[id] = std::move(group);
        }
    }
    
    /**
     * @brief Get group by ID
     * 
     * @param id Group ID
     * @return ParameterGroup* Pointer to group (nullptr if not found)
     */
    ParameterGroup* getGroup(const GroupId& id) {
        auto it = groups_.find(id);
        if (it != groups_.end()) {
            return it->second.get();
        }
        return nullptr;
    }
    
    /**
     * @brief Remove group
     * 
     * @param id Group ID
     * @return true if group was removed
     */
    bool removeGroup(const GroupId& id) {
        return groups_.erase(id) > 0;
    }
    
    // Tree traversal
    
    /**
     * @brief Get parent group
     * 
     * @return ParameterGroup* Pointer to parent (nullptr if root)
     */
    ParameterGroup* getParent() const { return parent_; }
    
    /**
     * @brief Get absolute path of this group
     * 
     * @return std::string Path (e.g., "root/synth/oscillator1")
     */
    std::string getPath() const {
        if (!parent_) {
            return id_;
        }
        
        std::string parentPath = parent_->getPath();
        return parentPath + "/" + id_;
    }
    
    // Parameter access
    
    /**
     * @brief Get all parameters
     * 
     * @return const std::map<Parameter::ParameterId, std::unique_ptr<Parameter>>& Parameter map
     */
    const std::map<Parameter::ParameterId, std::unique_ptr<Parameter>>& 
    getParameters() const { return parameters_; }
    
    /**
     * @brief Get all nested groups
     * 
     * @return const std::map<GroupId, std::unique_ptr<ParameterGroup>>& Group map
     */
    const std::map<GroupId, std::unique_ptr<ParameterGroup>>& 
    getGroups() const { return groups_; }
    
    // Path-based access
    
    /**
     * @brief Get parameter by path
     * 
     * @param path Path to parameter (e.g., "oscillator1/detune")
     * @return Parameter* Pointer to parameter (nullptr if not found)
     */
    Parameter* getParameterByPath(const std::string& path) {
        std::istringstream pathStream(path);
        std::string segment;
        std::vector<std::string> segments;
        
        while (std::getline(pathStream, segment, '/')) {
            segments.push_back(segment);
        }
        
        if (segments.empty()) {
            return nullptr;
        }
        
        // Last segment is the parameter name
        std::string paramName = segments.back();
        segments.pop_back();
        
        // Navigate to the right group
        ParameterGroup* currentGroup = this;
        for (const auto& groupName : segments) {
            currentGroup = currentGroup->getGroup(groupName);
            if (!currentGroup) {
                return nullptr;
            }
        }
        
        return currentGroup->getParameter(paramName);
    }
    
    /**
     * @brief Get group by path
     * 
     * @param path Path to group (e.g., "oscillator1/envelopes")
     * @return ParameterGroup* Pointer to group (nullptr if not found)
     */
    ParameterGroup* getGroupByPath(const std::string& path) {
        std::istringstream pathStream(path);
        std::string segment;
        std::vector<std::string> segments;
        
        while (std::getline(pathStream, segment, '/')) {
            segments.push_back(segment);
        }
        
        if (segments.empty()) {
            return this;
        }
        
        // Navigate to the right group
        ParameterGroup* currentGroup = this;
        for (const auto& groupName : segments) {
            currentGroup = currentGroup->getGroup(groupName);
            if (!currentGroup) {
                return nullptr;
            }
        }
        
        return currentGroup;
    }
    
protected:
    GroupId id_;
    std::string name_;
    ParameterGroup* parent_;
    
    std::map<Parameter::ParameterId, std::unique_ptr<Parameter>> parameters_;
    std::map<GroupId, std::unique_ptr<ParameterGroup>> groups_;
    
    /**
     * @brief Set parent group
     * 
     * @param parent Pointer to parent group
     */
    void setParent(ParameterGroup* parent) { parent_ = parent; }
};

} // namespace AIMusicHardware