#pragma once

#include "EditorWindow.hpp"

#include "RoseV2/pch.hpp"
#include "RoseV2/Graphics/Texture.hpp"
#include "RoseV2/Asset/Asset.hpp"

namespace RoseV2::Editor {

class EditorLayer;

struct AssetNode {
    std::filesystem::path                   Path;
    Asset::AssetHandle                      Handle = 0; 
    std::weak_ptr<AssetNode>                Parent;
    std::vector<std::shared_ptr<AssetNode>> Children;
};

struct ScriptWatcher {
    std::filesystem::path Path;
    std::filesystem::file_time_type LastWriteTime;
};

class ProjectWindow : public EditorWindow {
public:
    ProjectWindow(EditorLayer *layer);
    void onUpdate();
    void render() override;

    void setProjectRoot(const std::filesystem::path &root);
private:
    void loadIcons();

    void drawFolderTree(const std::shared_ptr<AssetNode> &node);
    void drawFileList(const std::shared_ptr<AssetNode> &node);
    void drawNewAssetPopup();
    void drawImportPopup();
    
    void addScriptWatcher(const std::filesystem::path &script);
    void updateScriptWatchers();
    void reloadScript(const std::filesystem::path &script);
    
    void recursivePopulateRoot(const std::shared_ptr<AssetNode> &node, uint32_t depth);
    void resetTree(std::shared_ptr<AssetNode> &node);
    void resetAndRepopulateTree();

    void navigateUp();
    void onFolderClick(const std::shared_ptr<AssetNode> &folder);
    void onFileClick(const std::shared_ptr<AssetNode> &file);
private:
    EditorLayer *m_EditorLayer;
    
    std::shared_ptr<Graphics::Texture> m_FolderIcon;
    std::shared_ptr<Graphics::Texture> m_FileIcon;

    std::shared_ptr<AssetNode> m_ProjectRoot;
    std::shared_ptr<AssetNode> m_SelectedFolder;

    std::vector<ScriptWatcher> m_WatchedScripts; 

    bool m_ShowNewPopup = false;
    bool m_ShowImportPopup = false;
};

}
