#pragma once

#include "EditorWindow.hpp"
#include "RoseV2/ECS/Registry.hpp"
#include "RoseV2/Graphics/Framebuffer.hpp"
#include "RoseV2/Graphics/Camera.hpp"

#include <imgui.h>
#include "ImGuizmo.h"
#include <glm/glm.hpp>

namespace RoseV2::Editor {

class EditorLayer;

class ViewportWindow : public EditorWindow {
public:
    ViewportWindow(EditorLayer *layer);

    void render() override;
    void setRegistry(ECS::Registry* registry) { m_Registry = registry; }
    void setSelectedEntity(ECS::Entity e) { m_SelectedEntity = e; }

    std::shared_ptr<Graphics::Framebuffer> getFramebuffer() const { return m_Framebuffer; }
private:
    void handleFramebufferResize(const ImVec2& size);
    Graphics::Camera findActiveCamera(const ImVec2& viewportSize);
    void renderScene(const Graphics::Camera& camera);
    void renderGizmos(const Graphics::Camera& camera);
private:
    EditorLayer *m_EditorLayer = nullptr;

    glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
    ImVec2 m_ViewportBoundsMin = ImVec2(0,0);
    ImVec2 m_ViewportBoundsMax = ImVec2(0,0);

    ECS::Registry* m_Registry = nullptr;
    ECS::Entity m_SelectedEntity = ECS::Entity::Null();
    
    std::optional<ImGuizmo::OPERATION> m_GizmoOperation;

    std::shared_ptr<Graphics::Framebuffer> m_Framebuffer;
};

}
