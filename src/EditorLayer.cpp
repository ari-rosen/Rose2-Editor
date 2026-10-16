#include "EditorLayer.hpp"

#include "MenuBar.hpp"
#include "RoseV2/ECS/Component.hpp"
#include "RoseV2/Core/Application.hpp"
#include "RoseV2/Core/Assert.hpp"
#include "RoseV2/Core/Logger.hpp"
#include "RoseV2/Scene/SceneSerializer.hpp"
#include "RoseV2/Project/Project.hpp"
#include "RoseV2/Script/LuaEngine.hpp"

#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

#include <ImGuizmo.h>

namespace RoseV2::Editor {

EditorLayer::EditorLayer(Core::Application *application)
    : m_ViewportWindow(this), m_MenuBar(this), m_ProjectWindow(this), m_InspectorWindow(this), m_CurrentState(EditorState::Edit)
{
    m_Application = application;
    m_InspectorWindow.setFramebuffer(m_ViewportWindow.getFramebuffer().get());

    Core::Logger::setOutputStream(m_ConsoleWindow.getBuffer());
    initImGui();
    setColorTheme();
}

void EditorLayer::initImGui() {
    IMGUI_CHECKVERSION();

    ImGuiContext *imguiInitContext = ImGui::CreateContext();
    ROSEV2_ASSERT(imguiInitContext != nullptr, "Failed to initialize ImGui: Context failed");

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    bool imguiInitGLFW = ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)m_Application->getWindow()->getHandle(), true);
    ROSEV2_ASSERT(imguiInitGLFW, "Failed to initialize ImGui: GLFW failed");

    bool imguiInitGL = ImGui_ImplOpenGL3_Init("#version 410");
    ROSEV2_ASSERT(imguiInitGL, "Failed to initialize ImGui: OpenGL failed");

}

void EditorLayer::setActiveScene(const std::shared_ptr<Scene::Scene> &scene) {
    m_ActiveScene = scene;
    m_ViewportWindow.setRegistry(&m_ActiveScene->getRegistry());
    
    m_SceneHierarchyWindow.setScene(m_ActiveScene);
    m_SceneHierarchyWindow.setRegistry(&m_ActiveScene->getRegistry());
    
    m_InspectorWindow.setRegistry(&m_ActiveScene->getRegistry());
    m_InspectorWindow.setSelectedEntity(m_SceneHierarchyWindow.getSelectedEntity());
}

void EditorLayer::startRuntime() {
    m_RuntimeScene = std::make_shared<Scene::Scene>();
    m_ViewportWindow.setRegistry(&m_RuntimeScene->getRegistry());
    m_SceneHierarchyWindow.setRegistry(&m_RuntimeScene->getRegistry());

    auto copyScene = [&]() {
        auto& runtimeRegistry = m_RuntimeScene->getRegistry();
        auto& editorRegistry = m_ActiveScene->getRegistry();
    
        for (auto e : editorRegistry.view<ECS::TransformComponent>()) {
            ECS::Entity newEntity = runtimeRegistry.createEntity();
    
            std::apply([&](auto... Components) {
                ((editorRegistry.hasComponent<std::decay_t<decltype(Components)>>(e) ?
                    runtimeRegistry.addComponent<std::decay_t<decltype(Components)>>(
                        newEntity,
                        editorRegistry.getComponent<std::decay_t<decltype(Components)>>(e)
                    ) : void()), ...);
            }, ECS::AllComponents{});
        }
    };
    copyScene();
    
    m_RuntimeScene->onRuntimeStart();
    m_CurrentState = EditorState::Running;
}

void EditorLayer::stopRuntime() {
    m_RuntimeScene->onRuntimeStop();
    m_ViewportWindow.setRegistry(&m_ActiveScene->getRegistry());
    m_SceneHierarchyWindow.setRegistry(&m_ActiveScene->getRegistry());
    m_RuntimeScene.reset();

    m_CurrentState = EditorState::Edit;
}

void EditorLayer::onUpdate(Core::Timestep deltaTime) {
    m_InspectorWindow.setSelectedEntity(m_SceneHierarchyWindow.getSelectedEntity());
    m_ViewportWindow.setSelectedEntity(m_SceneHierarchyWindow.getSelectedEntity());

    m_ProjectWindow.onUpdate();

    if (m_CurrentState == EditorState::Running && m_RuntimeScene) {
        m_RuntimeScene->onRuntimeUpdate(deltaTime);
    }
}

void EditorLayer::onRender() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    ImGuiIO& io = ImGui::GetIO();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                           ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus |
                                           ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("DockSpaceHost", nullptr, window_flags);
    
    ImGui::PopStyleVar(2);
    
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    
    ImGui::End();
    
    // render windows

    m_MenuBar.render();
    if (m_ActiveScene) {
        m_SceneHierarchyWindow.render();
        m_InspectorWindow.render();
        m_ViewportWindow.render();
        m_ProjectWindow.render();
        m_ConsoleWindow.render();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::loadScene(const std::filesystem::path &filepath) {
    std::shared_ptr<Scene::Scene> newScene = std::make_shared<Scene::Scene>();
    Scene::SceneSerializer serializer(newScene);
    std::string fmt = "Failed to load scene: " + filepath.string();
    ROSEV2_ASSERT(serializer.deserialize(filepath), fmt.c_str());
    setActiveScene(newScene);

    Script::LuaEngine::getFieldRegistry().deserializeEntityFields(*m_ActiveScene);
}

void EditorLayer::saveScene(const std::filesystem::path &filepath) {
    if (!m_ActiveScene) return;
    Scene::SceneSerializer serializer(m_ActiveScene);
    serializer.serialize(filepath); 

    Script::LuaEngine::getFieldRegistry().serializeEntityFields(*m_ActiveScene);
}

void EditorLayer::loadActiveProjectAssets() {
    if (auto project = Project::Project::getActiveProject()) {
        bool success = project->getEditorAssetManager()->deserializeAssetRegistry();
        if (!success) {
            Core::Logger::log(Core::LogLevel::Error, "Failed to deserialize asset registry");
        }
    }
}

void EditorLayer::onProjectLoad() {
    auto project = Project::Project::getActiveProject();
    if (!project) return;
    
    loadActiveProjectAssets();
    m_ProjectWindow.setProjectRoot(project->getProjectDirectory());
}

void EditorLayer::setColorTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.Colors[ImGuiCol_Text]                  = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.30f, 0.31f, 0.30f, 0.20f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.60f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.20f, 0.22f, 0.27f, 0.9f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.20f, 0.22f, 0.27f, 0.86f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.60f, 0.18f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.63f, 0.14f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TabSelectedOverline]   = ImVec4(0.92f, 0.18f, 0.29f, 0.00f);
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.13f, 0.14f, 0.17f, 0.00f);
    style.Colors[ImGuiCol_DockingPreview]        = ImVec4(0.60f, 0.18f, 0.29f, 1.00f);

}

} // namespace RoseV2::Editor
