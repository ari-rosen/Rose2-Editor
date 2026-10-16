#include "ViewportWindow.hpp"
#include "EditorLayer.hpp"
#include "MenuBar.hpp"
#include "RoseV2/Graphics/Renderer.hpp"
#include "RoseV2/ECS/Component.hpp"

#include <imgui.h>
#include "ImGuizmo.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace RoseV2::Editor {

ViewportWindow::ViewportWindow(EditorLayer *layer)
    : m_EditorLayer(layer)
{
    Graphics::FramebufferSpecification fbSpec;
    fbSpec.Width = 1280;
    fbSpec.Height = 720;
    fbSpec.Attachments = {
        { Graphics::FramebufferTextureFormat::RGBA8 },
        { Graphics::FramebufferTextureFormat::DEPTH24STENCIL8 }
    };
    m_Framebuffer = Graphics::Framebuffer::create(fbSpec);
}

void ViewportWindow::render() {
    if (!m_Registry) return;

    ImGui::Begin("Viewport");

    if (ImGui::Button("Run")) {
        if (m_EditorLayer)
            m_EditorLayer->startRuntime();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        if (m_EditorLayer)
            m_EditorLayer->stopRuntime();
    }

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x > 0 && viewportSize.y > 0) {
        handleFramebufferResize(viewportSize);

        Graphics::Camera activeCamera = findActiveCamera(viewportSize);
        renderScene(activeCamera);

        uint32_t texID = m_Framebuffer->getColorAttachmentRendererID(0);
        ImGui::Image((ImTextureID)(intptr_t)texID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

        m_ViewportBoundsMin = ImGui::GetItemRectMin();
        m_ViewportBoundsMax = ImGui::GetItemRectMax();

        renderGizmos(activeCamera);
    }

    // Toolbar overlay inside the same window
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetItemRectMin().x + 10,
                                     ImGui::GetItemRectMin().y + 10));
    
    auto DrawToolButton = [&](const char* label, std::optional<ImGuizmo::OPERATION> op, bool last){
        bool isActive = (m_GizmoOperation == op);
        if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.92f, 0.18f, 0.29f, 1.00f));
    
        if (ImGui::Button(label))
            m_GizmoOperation = (m_GizmoOperation == op) ? std::nullopt : op;
    
        if (isActive) ImGui::PopStyleColor();
        if (!last) ImGui::SameLine();
    };
    
    DrawToolButton("Select", std::nullopt, false);
    DrawToolButton("Move",   ImGuizmo::OPERATION::TRANSLATE, false);
    DrawToolButton("Rotate", ImGuizmo::OPERATION::ROTATE, false);
    DrawToolButton("Scale",  ImGuizmo::OPERATION::SCALE, true);
    
    ImGui::End();
}

void ViewportWindow::handleFramebufferResize(const ImVec2& size) {
    if (m_ViewportSize.x != size.x || m_ViewportSize.y != size.y) {
        m_ViewportSize = { size.x, size.y };
        m_Framebuffer->resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

        float aspect = m_ViewportSize.x / m_ViewportSize.y;
        for (auto e : m_Registry->view<ECS::CameraComponent>()) {
            auto& cam = m_Registry->getComponent<ECS::CameraComponent>(e);
            cam.Left   = -aspect;
            cam.Right  =  aspect;
            cam.Bottom = -1.0f;
            cam.Top    =  1.0f;
        }
    }
}

Graphics::Camera ViewportWindow::findActiveCamera(const ImVec2& viewportSize) {
    Graphics::Camera activeCamera{};
    
    for (auto e : m_Registry->view<ECS::CameraComponent, ECS::TransformComponent>()) {
        if (m_Registry->isEntityAlive(e)) {
            const auto& cam = m_Registry->getComponent<ECS::CameraComponent>(e);
            if (cam.Primary) {
                const auto& t = m_Registry->getComponent<ECS::TransformComponent>(e);
                
                activeCamera.setProjection(cam.Left, cam.Right, cam.Bottom, cam.Top);
                activeCamera.setZoom(cam.Zoom);
                activeCamera.setPosition({t.Translation.x, t.Translation.y});
                activeCamera.setRotation(t.Rotation.z);
                
                return activeCamera;
            }
        }
    }

    float aspect = viewportSize.x / viewportSize.y;
    activeCamera.setProjection(-aspect, aspect, -1.0f, 1.0f);
    return activeCamera;
}

void ViewportWindow::renderScene(const Graphics::Camera& camera) {
    m_Framebuffer->bind();
    Graphics::Renderer::beginScene(camera);

    for (auto e : m_Registry->view<ECS::SpriteComponent, ECS::TransformComponent>()) {
        const auto& s = m_Registry->getComponent<ECS::SpriteComponent>(e);
        const auto& t = m_Registry->getComponent<ECS::TransformComponent>(e);

        Graphics::Renderer::submitSprite(t, s);
    }

    Graphics::Renderer::endScene();
    m_Framebuffer->unbind();
}

void ViewportWindow::renderGizmos(const Graphics::Camera& camera) {
    if (!m_GizmoOperation.has_value()) return;

    const auto view = camera.getViewMatrix();
    const auto proj = camera.getProjectionMatrix();
    
    ImGuizmo::SetDrawlist();

    ImGuizmo::SetOrthographic(true);

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

    ImGuizmo::SetRect(windowPos.x + contentMin.x,
                      windowPos.y + contentMin.y,
                      contentMax.x - contentMin.x,
                      contentMax.y - contentMin.y);

    if (m_SelectedEntity != ECS::Entity::Null() && 
        m_EditorLayer->getActiveScene()->getRegistry().hasComponent<ECS::TransformComponent>(m_SelectedEntity)) {
    
        auto& transform = m_EditorLayer->getActiveScene()->getRegistry().getComponent<ECS::TransformComponent>(m_SelectedEntity);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.Translation);
        glm::mat4 rotation = glm::yawPitchRoll(transform.Rotation.y, transform.Rotation.x, transform.Rotation.z);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.Scale);
        glm::mat4 model = translation * rotation * scale;
    
        glm::mat4 modelCopy = model;

        ImGuizmo::OPERATION op = m_GizmoOperation.value();
        ImGuizmo::MODE mode = ImGuizmo::LOCAL;

        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
                             op, mode, glm::value_ptr(modelCopy));

        if (ImGuizmo::IsUsing()) {
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::vec3 newTranslation, newScale;
            glm::quat newRotationQuat;

            bool ok = glm::decompose(modelCopy, newScale, newRotationQuat, newTranslation, skew, perspective);
            if (ok) {
                glm::vec3 newEuler = glm::eulerAngles(newRotationQuat);

                transform.Translation = newTranslation;
                transform.Scale = newScale;
                transform.Rotation = newEuler;
            }
        }
    }
}

}
