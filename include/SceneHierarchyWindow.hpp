#pragma once

#include "EditorWindow.hpp"
#include "RoseV2/ECS/Registry.hpp"
#include "RoseV2/Scene/Scene.hpp"

namespace RoseV2::Editor {

class SceneHierarchyWindow : public EditorWindow {
public:
    void setRegistry(ECS::Registry *registry) { m_Registry = registry; }
    void setScene(const std::shared_ptr<Scene::Scene> scene) { m_Scene = scene; };

    void render() override;

    inline ECS::Entity getSelectedEntity() const { return m_SelectedEntity; }
private:
    std::shared_ptr<Scene::Scene> m_Scene;
    ECS::Registry *m_Registry = nullptr;
    ECS::Entity m_SelectedEntity = ECS::Entity::Null();
};

}
