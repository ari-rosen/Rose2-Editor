#pragma once

#include "EditorWindow.hpp"

#include "RoseV2/ECS/Entity.hpp"
#include "RoseV2/ECS/Registry.hpp"
#include "RoseV2/Graphics/Framebuffer.hpp"

namespace RoseV2::Editor {

class EditorLayer;

class InspectorWindow : public EditorWindow {
public:
    InspectorWindow(EditorLayer *layer) : m_EditorLayer(layer) {}

    void setRegistry(ECS::Registry* registry) { m_Registry = registry; }
    void setSelectedEntity(ECS::Entity entity) { m_SelectedEntity = entity; }
    void setFramebuffer(Graphics::Framebuffer *fb) { m_Framebuffer = fb; }
    void render() override;
private:
    EditorLayer *m_EditorLayer = nullptr;
    ECS::Registry* m_Registry = nullptr;
    Graphics::Framebuffer* m_Framebuffer = nullptr;
    ECS::Entity m_SelectedEntity = ECS::Entity::Null();
};

}
