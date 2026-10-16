#include "MenuBar.hpp"

#include "EditorLayer.hpp"
#include "NFDHelper.hpp"

#include "ProjectWindow.hpp"
#include "RoseV2/Project/Project.hpp"

#include <imgui.h>

namespace RoseV2::Editor {

static const char* defaultScene = R"({
    "Name": "MainScene",
    "Entities": []
})";

static const char* defaultAssetRegistry = R"({
    "Assets": []
})";


MenuBar::MenuBar(EditorLayer *layer)
    : m_EditorLayer(layer)
{}

void MenuBar::render() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open Scene")) {
                openSceneDialog();
            }
            if (ImGui::MenuItem("Save Scene")) {
                saveSceneDialog();
            }
            if (ImGui::MenuItem("Exit")) {
                m_EditorLayer->getApplication()->stop();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Project")) {
            if (ImGui::MenuItem("Create Project")) 
                m_ShowCreateProjectModal = true;
            if (ImGui::MenuItem("Open Project")) {
                openProjectDialog();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    renderCreateProjectModal();
}

void MenuBar::openSceneDialog() {
    auto project = Project::Project::getActiveProject();
    if (!project) return;

    if (auto filePath = openFileDialog("rose", (project->GetAssetDirectory() / "Scenes").c_str())) {
        if (filePath->rfind(project->GetAssetDirectory(), 0) == 0) {
            m_EditorLayer->loadScene(*filePath);
        }
    }
}

void MenuBar::saveSceneDialog() {
    auto project = Project::Project::getActiveProject();
    auto scene   = m_EditorLayer->getActiveScene();
    if (!project || !scene) return;

    std::filesystem::path scenePath = project->GetAssetDirectory() / "Scenes" / (scene->getName() + ".rose");

    m_EditorLayer->saveScene(scenePath);
}

void MenuBar::renderCreateProjectModal() {
    if (m_ShowCreateProjectModal) {
        ImGui::OpenPopup("Create Project");
        m_ShowCreateProjectModal = false;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Create Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Project Name", m_CreateProjectName, sizeof(m_CreateProjectName));
        ImGui::InputText("Project Folder", m_CreateProjectFolder, sizeof(m_CreateProjectFolder));

        if (ImGui::Button("Browse")) {
            if (auto folder = pickFolderDialog(nullptr)) {
                strcpy(m_CreateProjectFolder, folder->c_str());
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Create")) {
            createProject();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void MenuBar::createProject() {
    std::filesystem::path parentFolder = m_CreateProjectFolder;
    std::filesystem::path projectFolder = parentFolder / m_CreateProjectName;

    std::filesystem::create_directories(projectFolder / "Assets" / "Scenes");

    auto newProject = Project::Project::newProject();
    auto& projectData = Project::Project::getActiveProject()->getData();
    projectData.Name = m_CreateProjectName;
    projectData.StartScene = "Scenes/MainScene.rose";
    projectData.AssetDirectory = (projectFolder / "Assets").string();
    Project::Project::saveActiveProject(projectFolder / (std::string(m_CreateProjectName) + ".roseproj"));

    std::filesystem::path mainScenePath = projectFolder / "Assets" / "Scenes" / "MainScene.rose";
    if (!std::filesystem::exists(mainScenePath)) {
        std::ofstream sceneFile(mainScenePath);
        sceneFile << defaultScene;
    }
    
    std::filesystem::path assetRegistryPath = projectFolder / "Assets.roseres";
    if (!std::filesystem::exists(assetRegistryPath)) {
        std::ofstream assetRegistryFile(assetRegistryPath);
        assetRegistryFile << defaultAssetRegistry;
    }

    std::filesystem::path scriptFieldReigistryPath = projectFolder / "Fields.rosefields";
    if (!std::filesystem::exists(scriptFieldReigistryPath)) {
        std::ofstream scriptFieldReigistryFile(scriptFieldReigistryPath);
        scriptFieldReigistryFile << "{}";
    }

    m_EditorLayer->onProjectLoad();
}

void MenuBar::openProjectDialog() {
    if (auto filePath = openFileDialog("roseproj", nullptr)) {
        auto project = Project::Project::loadProject(*filePath);
        if (!project) return;

        if (std::filesystem::exists(Project::Project::GetAssetDirectory() / project->getData().StartScene)) {
            m_EditorLayer->loadScene(Project::Project::GetAssetDirectory() / project->getData().StartScene);
        }
        
        m_EditorLayer->onProjectLoad();
    }
}

}
