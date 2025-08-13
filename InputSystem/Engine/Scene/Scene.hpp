// Engine/Scene/Scene.hpp
// Scene graph and management system
#pragma once

#include "../Core/Math.hpp"
#include "../Core/Transform.hpp"
#include "../Camera.hpp"  // Include your existing Camera
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>

namespace Engine {
namespace SceneSystem {

    // Forward declarations
    class SceneNode;
    class Light;
    class Renderable;

    using SceneNodePtr = std::shared_ptr<SceneNode>;
    using LightPtr = std::shared_ptr<Light>;

    // ========================================
    // Scene Node - Base class for all scene objects
    // ========================================

    class SceneNode : public std::enable_shared_from_this<SceneNode> {
    public:
        SceneNode(const std::string& name = "SceneNode")
            : name_(name)
            , isActive_(true)
            , isVisible_(true)
        {}

        virtual ~SceneNode() = default;

        // Hierarchy management
        void addChild(SceneNodePtr child) {
            if (!child || child.get() == this) return;

            // Remove from old parent
            if (auto oldParent = child->parent_.lock()) {
                oldParent->removeChild(child);
            }

            // Add to this node
            children_.push_back(child);
            child->parent_ = weak_from_this();
            child->onParentChanged();
        }

        void removeChild(SceneNodePtr child) {
            auto it = std::find(children_.begin(), children_.end(), child);
            if (it != children_.end()) {
                (*it)->parent_.reset();
                (*it)->onParentChanged();
                children_.erase(it);
            }
        }

        void removeFromParent() {
            if (auto p = parent_.lock()) {
                p->removeChild(shared_from_this());
            }
        }

        SceneNodePtr getParent() const {
            return parent_.lock();
        }

        const std::vector<SceneNodePtr>& getChildren() const {
            return children_;
        }

        // Transform
        Engine::Transform& getTransform() { return transform_; }
        const Engine::Transform& getTransform() const { return transform_; }

        Math::Mat4 getWorldMatrix() const {
            if (auto p = parent_.lock()) {
                return Math::Matrix::multiply(
                    p->getWorldMatrix(),
                    transform_.getLocalMatrix()
                );
            }
            return transform_.getLocalMatrix();
        }

        // Properties
        const std::string& getName() const { return name_; }
        void setName(const std::string& name) { name_ = name; }

        bool isActive() const { return isActive_; }
        void setActive(bool active) {
            isActive_ = active;
            onActiveChanged();
        }

        bool isVisible() const { return isVisible_; }
        void setVisible(bool visible) {
            isVisible_ = visible;
            onVisibilityChanged();
        }

        // Lifecycle callbacks
        virtual void onUpdate(float deltaTime) {
            if (!isActive_) return;

            // Update children
            for (auto& child : children_) {
                child->onUpdate(deltaTime);
            }
        }

        virtual void onFixedUpdate(float fixedDeltaTime) {
            if (!isActive_) return;

            // Update children
            for (auto& child : children_) {
                child->onFixedUpdate(fixedDeltaTime);
            }
        }

        virtual void onRender(const Engine::Camera& camera) {
            if (!isVisible_) return;

            // Render children
            for (auto& child : children_) {
                child->onRender(camera);
            }
        }

        // Events
        virtual void onParentChanged() {}
        virtual void onActiveChanged() {}
        virtual void onVisibilityChanged() {}

        // Search utilities
        SceneNodePtr findChild(const std::string& name) {
            if (name_ == name) return shared_from_this();

            for (auto& child : children_) {
                if (auto found = child->findChild(name)) {
                    return found;
                }
            }
            return nullptr;
        }

        template<typename T>
        std::shared_ptr<T> findChildOfType() {
            if (auto casted = std::dynamic_pointer_cast<T>(shared_from_this())) {
                return casted;
            }

            for (auto& child : children_) {
                if (auto found = child->findChildOfType<T>()) {
                    return found;
                }
            }
            return nullptr;
        }

    protected:
        std::string name_;
        Engine::Transform transform_;
        std::weak_ptr<SceneNode> parent_;
        std::vector<SceneNodePtr> children_;
        bool isActive_;
        bool isVisible_;
    };

    // ========================================
    // Camera Node - Wrapper for Engine::Camera in the scene graph
    // ========================================

    class CameraNode : public SceneNode {
    public:
        CameraNode(const std::string& name = "Camera")
            : SceneNode(name)
            , camera_(Math::Vector::make(0, 0, 5),
                     Math::Vector::Constants::ZERO,
                     Math::Vector::Constants::UP)
        {}

        Engine::Camera& getCamera() { return camera_; }
        const Engine::Camera& getCamera() const { return camera_; }

        // Override to sync camera position with scene node transform
        void onUpdate(float deltaTime) override {
            // Sync camera position with node transform
            camera_.setPosition(transform_.getPosition());

            // Update children
            SceneNode::onUpdate(deltaTime);
        }

    private:
        Engine::Camera camera_;
    };

    using CameraNodePtr = std::shared_ptr<CameraNode>;

    // ========================================
    // Light - Base class for lights
    // ========================================

    class Light : public SceneNode {
    public:
        enum Type {
            DIRECTIONAL,
            POINT,
            SPOT
        };

        Light(Type type, const std::string& name = "Light")
            : SceneNode(name)
            , type_(type)
            , color_{1.0f, 1.0f, 1.0f}
            , intensity_(1.0f)
            , range_(100.0f)
            , innerCone_(30.0f)
            , outerCone_(45.0f)
        {}

        // Light properties
        Type getType() const { return type_; }

        void setColor(const Math::Vec3& color) { color_ = color; }
        const Math::Vec3& getColor() const { return color_; }

        void setIntensity(float intensity) { intensity_ = intensity; }
        float getIntensity() const { return intensity_; }

        // For point/spot lights
        void setRange(float range) { range_ = range; }
        float getRange() const { return range_; }

        // For spot lights
        void setInnerCone(float angle) { innerCone_ = angle; }
        float getInnerCone() const { return innerCone_; }

        void setOuterCone(float angle) { outerCone_ = angle; }
        float getOuterCone() const { return outerCone_; }

        // Get light direction (for directional/spot lights)
        Math::Vec3 getDirection() const {
            // Forward direction from transform
            return transform_.getForward();
        }

        // Get light position (for point/spot lights)
        Math::Vec3 getPosition() const {
            return transform_.getPosition();
        }

    private:
        Type type_;
        Math::Vec3 color_;
        float intensity_;
        float range_;
        float innerCone_;
        float outerCone_;
    };

    // ========================================
    // Scene - Root container for scene graph
    // ========================================

    class Scene {
    public:
        Scene(const std::string& name = "Scene")
            : name_(name)
            , rootNode_(std::make_shared<SceneNode>("Root"))
            , activeCamera_(nullptr)
            , ambientLight_{0.2f, 0.2f, 0.2f}
        {}

        // Node management
        void addNode(SceneNodePtr node) {
            rootNode_->addChild(node);

            // Auto-register special nodes
            if (auto camera = std::dynamic_pointer_cast<CameraNode>(node)) {
                addCamera(camera);
            }
            if (auto light = std::dynamic_pointer_cast<Light>(node)) {
                addLight(light);
            }
        }

        void removeNode(SceneNodePtr node) {
            rootNode_->removeChild(node);

            // Auto-unregister special nodes
            if (auto camera = std::dynamic_pointer_cast<CameraNode>(node)) {
                removeCamera(camera);
            }
            if (auto light = std::dynamic_pointer_cast<Light>(node)) {
                removeLight(light);
            }
        }

        SceneNodePtr getRootNode() const { return rootNode_; }

        // Camera management
        void addCamera(CameraNodePtr camera) {
            cameras_.push_back(camera);
            if (!activeCamera_) {
                activeCamera_ = camera;
            }
        }

        void removeCamera(CameraNodePtr camera) {
            auto it = std::find(cameras_.begin(), cameras_.end(), camera);
            if (it != cameras_.end()) {
                cameras_.erase(it);
                if (activeCamera_ == camera) {
                    activeCamera_ = cameras_.empty() ? nullptr : cameras_[0];
                }
            }
        }

        void setActiveCamera(CameraNodePtr camera) {
            activeCamera_ = camera;
        }

        CameraNodePtr getActiveCamera() const { return activeCamera_; }
        const std::vector<CameraNodePtr>& getCameras() const { return cameras_; }

        // Light management
        void addLight(LightPtr light) {
            lights_.push_back(light);
        }

        void removeLight(LightPtr light) {
            auto it = std::find(lights_.begin(), lights_.end(), light);
            if (it != lights_.end()) {
                lights_.erase(it);
            }
        }

        const std::vector<LightPtr>& getLights() const { return lights_; }

        void setAmbientLight(const Math::Vec3& color) { ambientLight_ = color; }
        const Math::Vec3& getAmbientLight() const { return ambientLight_; }

        // Update and render
        void update(float deltaTime) {
            rootNode_->onUpdate(deltaTime);
        }

        void fixedUpdate(float fixedDeltaTime) {
            rootNode_->onFixedUpdate(fixedDeltaTime);
        }

        void render() {
            if (!activeCamera_) return;
            rootNode_->onRender(activeCamera_->getCamera());
        }

        // Properties
        const std::string& getName() const { return name_; }
        void setName(const std::string& name) { name_ = name; }

        // Search utilities
        SceneNodePtr findNode(const std::string& name) {
            return rootNode_->findChild(name);
        }

        template<typename T>
        std::shared_ptr<T> findNodeOfType() {
            return rootNode_->findChildOfType<T>();
        }

        template<typename T>
        std::vector<std::shared_ptr<T>> findAllNodesOfType() {
            std::vector<std::shared_ptr<T>> result;
            findAllNodesOfTypeRecursive<T>(rootNode_, result);
            return result;
        }

    private:
        template<typename T>
        void findAllNodesOfTypeRecursive(SceneNodePtr node,
                                        std::vector<std::shared_ptr<T>>& result) {
            if (auto casted = std::dynamic_pointer_cast<T>(node)) {
                result.push_back(casted);
            }

            for (auto& child : node->getChildren()) {
                findAllNodesOfTypeRecursive<T>(child, result);
            }
        }

        std::string name_;
        SceneNodePtr rootNode_;
        std::vector<CameraNodePtr> cameras_;
        std::vector<LightPtr> lights_;
        CameraNodePtr activeCamera_;
        Math::Vec3 ambientLight_;
    };

    using ScenePtr = std::shared_ptr<Scene>;

} // namespace SceneSystem
} // namespace Engine