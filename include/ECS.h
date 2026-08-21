#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <bitset>
#include <queue>
#include <array>       // std::array for the signature table
#include <algorithm>   // std::remove - without this, the unqualified call in
                       // SystemManager::EntityDestroyed resolves to C's
                       // ::remove(const char*) from <cstdio> instead
#include <cassert>

namespace Nexus {

// Entity is just an ID
using Entity = uint32_t;
constexpr Entity NULL_ENTITY = 0;

// Component type ID
using ComponentType = uint8_t;
constexpr ComponentType MAX_COMPONENTS = 64;

// Component signature (which components an entity has)
using Signature = std::bitset<MAX_COMPONENTS>;

// Base component interface
struct IComponentArray {
    virtual ~IComponentArray() = default;
    virtual void EntityDestroyed(Entity entity) = 0;
};

// Component storage
template<typename T>
class ComponentArray : public IComponentArray {
public:
    void InsertData(Entity entity, T component) {
        auto existing = entityToIndexMap_.find(entity);
        if (existing != entityToIndexMap_.end()) {
            // Component already exists, update it
            componentArray_[existing->second] = component;
            return;
        }

        // push_back rather than indexing: componentArray_ was never grown, so
        // `componentArray_[newIndex] = component` wrote past the end of an empty
        // vector on the very first AddComponent for any component type. Growing
        // the storage with the insert also keeps componentArray_.size() equal to
        // the live component count, which is what GetAllComponents relies on.
        const size_t newIndex = componentArray_.size();
        componentArray_.push_back(component);
        entityToIndexMap_[entity] = newIndex;
        indexToEntityMap_[newIndex] = entity;
        size_ = componentArray_.size();
    }

    void RemoveData(Entity entity) {
        auto removed = entityToIndexMap_.find(entity);
        if (removed == entityToIndexMap_.end()) {
            return; // Component doesn't exist
        }

        // Move the last element into the freed slot to keep the array dense,
        // then drop the tail so size stays in step with the live count.
        const size_t indexOfRemovedEntity = removed->second;
        const size_t indexOfLastElement = componentArray_.size() - 1;
        componentArray_[indexOfRemovedEntity] = componentArray_[indexOfLastElement];

        // Update map to point to moved spot
        const Entity entityOfLastElement = indexToEntityMap_[indexOfLastElement];
        entityToIndexMap_[entityOfLastElement] = indexOfRemovedEntity;
        indexToEntityMap_[indexOfRemovedEntity] = entityOfLastElement;

        entityToIndexMap_.erase(entity);
        indexToEntityMap_.erase(indexOfLastElement);

        componentArray_.pop_back();
        size_ = componentArray_.size();
    }

    /// Precondition: the entity has this component. Callers should check
    /// HasData first.
    T& GetData(Entity entity) {
        auto found = entityToIndexMap_.find(entity);
        // find rather than operator[]: the latter default-inserts index 0 for an
        // absent entity, so a lookup miss silently returned - and allowed writes
        // to - some other entity's component.
        assert(found != entityToIndexMap_.end() && "GetData called for an entity without this component");
        return componentArray_[found->second];
    }

    bool HasData(Entity entity) const {
        return entityToIndexMap_.find(entity) != entityToIndexMap_.end();
    }

    void EntityDestroyed(Entity entity) override {
        if (entityToIndexMap_.find(entity) != entityToIndexMap_.end()) {
            RemoveData(entity);
        }
    }

    /// All live components, densely packed. Index i corresponds to the entity
    /// returned by GetEntityAt(i).
    std::vector<T>& GetAllComponents() { return componentArray_; }
    size_t GetSize() const { return size_; }

private:
    std::vector<T> componentArray_;
    std::unordered_map<Entity, size_t> entityToIndexMap_;
    std::unordered_map<size_t, Entity> indexToEntityMap_;
    size_t size_ = 0;
};

// Component Manager - handles all component types
class ComponentManager {
public:
    template<typename T>
    void RegisterComponent() {
        const char* typeName = typeid(T).name();

        if (componentTypes_.find(typeName) != componentTypes_.end()) {
            return; // Already registered
        }

        componentTypes_[typeName] = nextComponentType_;
        componentArrays_[typeName] = std::make_shared<ComponentArray<T>>();
        ++nextComponentType_;
    }

    template<typename T>
    ComponentType GetComponentType() {
        const char* typeName = typeid(T).name();
        return componentTypes_[typeName];
    }

    template<typename T>
    void AddComponent(Entity entity, T component) {
        GetComponentArray<T>()->InsertData(entity, component);
    }

    template<typename T>
    void RemoveComponent(Entity entity) {
        GetComponentArray<T>()->RemoveData(entity);
    }

    template<typename T>
    T& GetComponent(Entity entity) {
        return GetComponentArray<T>()->GetData(entity);
    }

    template<typename T>
    bool HasComponent(Entity entity) {
        return GetComponentArray<T>()->HasData(entity);
    }

    void EntityDestroyed(Entity entity) {
        for (auto const& pair : componentArrays_) {
            auto const& component = pair.second;
            component->EntityDestroyed(entity);
        }
    }

private:
    std::unordered_map<const char*, ComponentType> componentTypes_;
    std::unordered_map<const char*, std::shared_ptr<IComponentArray>> componentArrays_;
    ComponentType nextComponentType_ = 0;

    template<typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray() {
        const char* typeName = typeid(T).name();
        return std::static_pointer_cast<ComponentArray<T>>(componentArrays_[typeName]);
    }
};

// Entity Manager - creates and destroys entities
class EntityManager {
public:
    EntityManager() {
        // Initialize the queue with all possible entity IDs
        for (Entity entity = 1; entity < MAX_ENTITIES; ++entity) {
            availableEntities_.push(entity);
        }
    }

    Entity CreateEntity() {
        if (livingEntityCount_ >= MAX_ENTITIES) {
            return NULL_ENTITY;
        }

        Entity id = availableEntities_.front();
        availableEntities_.pop();
        ++livingEntityCount_;

        return id;
    }

    void DestroyEntity(Entity entity) {
        if (entity >= MAX_ENTITIES) {
            return;
        }

        signatures_[entity].reset();
        availableEntities_.push(entity);
        --livingEntityCount_;
    }

    void SetSignature(Entity entity, Signature signature) {
        if (entity >= MAX_ENTITIES) {
            return;
        }

        signatures_[entity] = signature;
    }

    Signature GetSignature(Entity entity) {
        if (entity >= MAX_ENTITIES) {
            return Signature();
        }

        return signatures_[entity];
    }

    uint32_t GetLivingEntityCount() const {
        return livingEntityCount_;
    }

private:
    static constexpr uint32_t MAX_ENTITIES = 100000;

    std::queue<Entity> availableEntities_;
    std::array<Signature, MAX_ENTITIES> signatures_;
    uint32_t livingEntityCount_ = 0;
};

// System - processes entities with specific components
class System {
public:
    std::vector<Entity> entities;

    virtual void Update(float deltaTime) = 0;
    virtual ~System() = default;
};

// System Manager - manages all systems
class SystemManager {
public:
    template<typename T>
    std::shared_ptr<T> RegisterSystem() {
        const char* typeName = typeid(T).name();

        auto system = std::make_shared<T>();
        systems_[typeName] = system;
        return system;
    }

    template<typename T>
    void SetSignature(Signature signature) {
        const char* typeName = typeid(T).name();
        signatures_[typeName] = signature;
    }

    void EntityDestroyed(Entity entity) {
        for (auto const& pair : systems_) {
            auto const& system = pair.second;
            system->entities.erase(std::remove(system->entities.begin(), system->entities.end(), entity), system->entities.end());
        }
    }

    void EntitySignatureChanged(Entity entity, Signature entitySignature) {
        for (auto const& pair : systems_) {
            auto const& type = pair.first;
            auto const& system = pair.second;
            auto const& systemSignature = signatures_[type];

            // Entity signature matches system signature - insert into set
            if ((entitySignature & systemSignature) == systemSignature) {
                if (std::find(system->entities.begin(), system->entities.end(), entity) == system->entities.end()) {
                    system->entities.push_back(entity);
                }
            }
            // Entity signature does not match system signature - erase from set
            else {
                system->entities.erase(std::remove(system->entities.begin(), system->entities.end(), entity), system->entities.end());
            }
        }
    }

    template<typename T>
    std::shared_ptr<T> GetSystem() {
        const char* typeName = typeid(T).name();
        return std::static_pointer_cast<T>(systems_[typeName]);
    }

private:
    std::unordered_map<const char*, Signature> signatures_;
    std::unordered_map<const char*, std::shared_ptr<System>> systems_;
};

// ECS Coordinator - main interface
class ECS {
public:
    void Init() {
        componentManager_ = std::make_unique<ComponentManager>();
        entityManager_ = std::make_unique<EntityManager>();
        systemManager_ = std::make_unique<SystemManager>();
    }

    // Entity methods
    Entity CreateEntity() {
        return entityManager_->CreateEntity();
    }

    void DestroyEntity(Entity entity) {
        entityManager_->DestroyEntity(entity);
        componentManager_->EntityDestroyed(entity);
        systemManager_->EntityDestroyed(entity);
    }

    // Component methods
    template<typename T>
    void RegisterComponent() {
        componentManager_->RegisterComponent<T>();
    }

    template<typename T>
    void AddComponent(Entity entity, T component) {
        componentManager_->AddComponent<T>(entity, component);

        auto signature = entityManager_->GetSignature(entity);
        signature.set(componentManager_->GetComponentType<T>(), true);
        entityManager_->SetSignature(entity, signature);

        systemManager_->EntitySignatureChanged(entity, signature);
    }

    template<typename T>
    void RemoveComponent(Entity entity) {
        componentManager_->RemoveComponent<T>(entity);

        auto signature = entityManager_->GetSignature(entity);
        signature.set(componentManager_->GetComponentType<T>(), false);
        entityManager_->SetSignature(entity, signature);

        systemManager_->EntitySignatureChanged(entity, signature);
    }

    template<typename T>
    T& GetComponent(Entity entity) {
        return componentManager_->GetComponent<T>(entity);
    }

    template<typename T>
    bool HasComponent(Entity entity) {
        return componentManager_->HasComponent<T>(entity);
    }

    template<typename T>
    ComponentType GetComponentType() {
        return componentManager_->GetComponentType<T>();
    }

    // System methods
    template<typename T>
    std::shared_ptr<T> RegisterSystem() {
        return systemManager_->RegisterSystem<T>();
    }

    template<typename T>
    void SetSystemSignature(Signature signature) {
        systemManager_->SetSignature<T>(signature);
    }

    template<typename T>
    std::shared_ptr<T> GetSystem() {
        return systemManager_->GetSystem<T>();
    }

    // Stats
    uint32_t GetLivingEntityCount() const {
        return entityManager_->GetLivingEntityCount();
    }

private:
    std::unique_ptr<ComponentManager> componentManager_;
    std::unique_ptr<EntityManager> entityManager_;
    std::unique_ptr<SystemManager> systemManager_;
};

} // namespace Nexus
