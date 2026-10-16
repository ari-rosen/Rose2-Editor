#include "ConsoleWindow.hpp"
#include "InspectorWindow.hpp"
#include "MenuBar.hpp"
#include "ProjectWindow.hpp"
#include "RoseV2/Core/Application.hpp"
#include "RoseV2/Core/Layer.hpp"
#include "RoseV2/Scene/Scene.hpp"

#include "ViewportWindow.hpp"
#include "SceneHierarchyWindow.hpp"

namespace RoseV2::Editor {

class EditorLayer : public Core::Layer {
public:
    explicit EditorLayer(Core::Application *application);

    void loadScene(const std::filesystem::path &path);
    void saveScene(const std::filesystem::path &path);
    
    void onProjectLoad();
    void setActiveScene(const std::shared_ptr<Scene::Scene> &scene);

    void startRuntime();
    void stopRuntime();
   
    void onUpdate(Core::Timestep deltaTime) override;
    void onRender() override;

    inline Core::Application* getApplication() const { return m_Application; }
    inline std::shared_ptr<Scene::Scene> getActiveScene() const { return m_ActiveScene; }
private:
    void initImGui();
    void loadActiveProjectAssets();
    void setColorTheme();
private:
    std::shared_ptr<Scene::Scene> m_ActiveScene;
    std::shared_ptr<Scene::Scene> m_RuntimeScene;
    
    MenuBar m_MenuBar;
    ViewportWindow m_ViewportWindow;
    SceneHierarchyWindow m_SceneHierarchyWindow;
    InspectorWindow m_InspectorWindow;
    ProjectWindow m_ProjectWindow;
    ConsoleWindow m_ConsoleWindow;

    enum class EditorState {
        Edit = 0,
        Running = 1
    };

    EditorState m_CurrentState;
};

}
