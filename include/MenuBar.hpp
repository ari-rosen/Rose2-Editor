#pragma once

#include "EditorWindow.hpp"

namespace RoseV2::Editor {

class EditorLayer;

class MenuBar : public EditorWindow {
public:
    MenuBar(EditorLayer *layer);
    void render() override;
private:
    void openSceneDialog();
    void saveSceneDialog();
    void openProjectDialog();
    void renderCreateProjectModal();
    void createProject();

private:
    EditorLayer *m_EditorLayer;

    bool m_ShowCreateProjectModal = false;
    char m_CreateProjectName[128] = "Untitled";
    char m_CreateProjectFolder[256] = "";
};

}
