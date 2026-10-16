#include "ProjectWindow.hpp"

#include "RoseV2/Asset/Asset.hpp"
#include "RoseV2/Asset/AssetImporter.hpp"
#include "RoseV2/Asset/AssetManager.hpp"
#include "RoseV2/Core/Logger.hpp"
#include "RoseV2/Script/ScriptInstance.hpp"
#include "RoseV2/Script/LuaEngine.hpp"

#include "EditorLayer.hpp"
#include "NFDHelper.hpp"
#include "RoseV2/Graphics/Texture.hpp"
#include "RoseV2/Project/Project.hpp"

#include <filesystem>
#include <imgui.h>
#include <memory>

namespace RoseV2::Editor {

static bool isImageFile(const std::filesystem::path& path) {
    static const std::vector<std::string> exts = {
        ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tga", ".hdr", ".psd"
    };

    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return std::find(exts.begin(), exts.end(), ext) != exts.end();
}

ProjectWindow::ProjectWindow(EditorLayer *layer)
    :m_EditorLayer(layer)
{
    loadIcons();
}

void ProjectWindow::loadIcons() {
    std::filesystem::path assetDirectory;
#ifdef EDITOR_ASSET_DIR
    assetDirectory = EDITOR_ASSET_DIR;
#endif

    {
        Asset::AssetMetadata folderIconMetadata;
        folderIconMetadata.Type = Asset::AssetType::Texture;
        folderIconMetadata.Path = assetDirectory / "folder.png";
        
        m_FolderIcon = Asset::AssetImporter(folderIconMetadata).loadAs<Graphics::Texture>();

        if (m_FolderIcon == nullptr) {
            Core::Logger::log(Core::LogLevel::Error, "Failed to load internal editor texture");
        }
    }
    {
        Asset::AssetMetadata fileIconMetadata;
        fileIconMetadata.Type = Asset::AssetType::Texture;
        fileIconMetadata.Path = assetDirectory / "file.png";
        
        m_FileIcon = Asset::AssetImporter(fileIconMetadata).loadAs<Graphics::Texture>();

        if (m_FileIcon == nullptr) {
            Core::Logger::log(Core::LogLevel::Error, "Failed to load internal editor texture");
        }
    }
}

void ProjectWindow::drawFolderTree(const std::shared_ptr<AssetNode> &node) {
    if (!node) {
        return;
    }

    std::string nodeName = node->Path.filename().string();
    
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;

    if (node->Path == m_ProjectRoot->Path) {
        nodeFlags |= ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen;
    }

    bool isFolder = std::filesystem::is_directory(node->Path);
    if (!isFolder) {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf;
    }
    
    if (m_SelectedFolder && node->Path == m_SelectedFolder->Path) {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(node->Path.c_str());
    bool nodeOpen = ImGui::TreeNodeEx(nodeName.c_str(), nodeFlags);
    ImGui::PopID();

    if (ImGui::IsItemClicked()) {
        if (isFolder) {
            onFolderClick(node);
        } else {
            onFileClick(node);
        }
    }

    if (nodeOpen) {
        for (const auto& child : node->Children) {
            drawFolderTree(child);
        }
        ImGui::TreePop();
    }
}

void ProjectWindow::drawFileList(const std::shared_ptr<AssetNode> &node) {
    if (!node || node->Children.empty()) {
        return;
    }

    constexpr float iconSize = 64.0f;
    constexpr float cellWidth = 64.0f;
    for (const auto& child : node->Children) {
        bool isDirectory = std::filesystem::is_directory(child->Path);

        auto icon = isDirectory ? m_FolderIcon : m_FileIcon;
        if (auto texture = Asset::AssetManager::getAsset<Graphics::Texture>(child->Handle)) {
            icon = texture;
        }

        std::string pathString = child->Path.generic_string();
        ImGui::PushID(pathString.c_str());
        ImGui::BeginGroup();
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::ImageButton(
            pathString.c_str(),
            icon->getRendererID(),
            ImVec2(iconSize,iconSize)
        );
        ImGui::PopStyleColor();
        if (isImageFile(child->Path)) {
            if (ImGui::BeginDragDropSource()) {
                uint64_t payload = child->Handle;
                ImGui::SetDragDropPayload("Texture Handle", &payload, sizeof(uint64_t));
                ImGui::Text("%s", child->Path.filename().c_str());
                ImGui::EndDragDropSource();
            }
        }
        if (child->Path.extension() == ".lua") {
            if (ImGui::BeginDragDropSource()) {
                uint64_t payload = child->Handle;
                ImGui::SetDragDropPayload("Script Handle", &payload, sizeof(uint64_t));
                ImGui::Text("%s", child->Path.filename().c_str());
                ImGui::EndDragDropSource();
            }
        }
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (isDirectory) {
                onFolderClick(child);
            }
            else {
                onFileClick(child);
            }
        }
        
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + iconSize + 16.0f); 
        ImGui::TextWrapped("%s", child->Path.filename().string().c_str());
        ImGui::PopTextWrapPos();       

        ImGui::EndGroup();
        ImGui::PopID();

        if (ImGui::GetCursorPosX() + cellWidth > ImGui::GetWindowWidth())
            ImGui::NewLine();
        else
            ImGui::SameLine();
    }
}

void ProjectWindow::drawNewAssetPopup() {
    static bool openNewScriptModal = false;
    static bool openNewFolderModal = false;
    static bool openNewModuleScriptModal = false;
    static char newFolderName[128] = "Folder";
    static char newScriptName[128] = "Script";

    if (ImGui::BeginPopup("NewPopup")) {
        if (ImGui::MenuItem("New Scene")) {
        }
        if (ImGui::MenuItem("New Folder")) {
            openNewFolderModal = true;
        }
        if (ImGui::MenuItem("New Script")) {
            openNewScriptModal = true;
        }
        if (ImGui::MenuItem("New Module Script")) {
            openNewModuleScriptModal = true;
        }
        ImGui::EndPopup();
    }

    if (openNewScriptModal) {
        ImGui::OpenPopup("New Script");
        openNewScriptModal = false;
    }

    if (openNewFolderModal) {
        ImGui::OpenPopup("New Folder");
        openNewFolderModal = false;
    }

    if (openNewModuleScriptModal) {
        ImGui::OpenPopup("New Module Script");
        openNewModuleScriptModal = false;
    }

    if (ImGui::BeginPopupModal("New Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Folder Name", newFolderName, sizeof(newFolderName));
        
        if (ImGui::Button("Create")) {
            if (m_SelectedFolder && strlen(newFolderName) > 0) {
                std::filesystem::path newFolderPath = m_SelectedFolder->Path / newFolderName;
    
                std::error_code ec;
                std::filesystem::create_directory(newFolderPath, ec);
                if (ec) {
                    Core::Logger::log(Core::LogLevel::Error, "Failed to create folder: " + ec.message());
                } else {
                    auto newNode = std::make_shared<AssetNode>();
                    newNode->Path = newFolderPath;
                    newNode->Parent = m_SelectedFolder;
                    
                    resetAndRepopulateTree();
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("New Script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::InputText("Script Name", newScriptName, sizeof(newScriptName));

        if (ImGui::Button("Create")) {
            std::string scriptFileName = std::string(newScriptName) + ".lua";
            std::filesystem::path newScriptPath = m_SelectedFolder->Path / scriptFileName;

            if (std::filesystem::exists(newScriptPath)) {
                newScriptPath = m_SelectedFolder->Path / (std::string(newScriptName) + ".lua");
            }

            std::ofstream file(newScriptPath);
            if (file.is_open()) {
                std::string tableName = std::filesystem::path(newScriptPath).stem().string();
                file << tableName << " = {}\n\n"
                     << "function " << tableName << ":onStart(entity)\n\nend\n\n"
                     << "function " << tableName << ":onUpdate(entity, deltaTime)\n\nend\n";
                file.close();
            }

            auto newNode = std::make_shared<AssetNode>();
            newNode->Path = newScriptPath;
            newNode->Parent = m_SelectedFolder;
            
            auto assetManager = Project::Project::getActiveProject()->getEditorAssetManager();
            Asset::AssetHandle handle = assetManager->importAsset(newScriptPath);
            newNode->Handle = handle;

            m_SelectedFolder->Children.push_back(newNode);

            resetAndRepopulateTree();
            assetManager->serializeAssetRegistry();
    
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("New Module Script", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Module Name", newScriptName, sizeof(newScriptName));

        if (ImGui::Button("Create")) {
            std::string scriptFileName = std::string(newScriptName) + "_module.lua";
            std::filesystem::path newScriptPath = m_SelectedFolder->Path / scriptFileName;

            if (std::filesystem::exists(newScriptPath)) {
                newScriptPath = m_SelectedFolder->Path / scriptFileName;
            }

            std::ofstream file(newScriptPath);
            if (file.is_open()) {
                std::string tableName = std::filesystem::path(scriptFileName).stem().string();
                if (tableName.ends_with("_module"))
                    tableName = tableName.substr(0, tableName.size() - 7);
                file << "local " << tableName << " = {}\n\nreturn " << tableName;
                file.close();
            }

            auto newNode = std::make_shared<AssetNode>();
            newNode->Path = newScriptPath;
            newNode->Parent = m_SelectedFolder;
            newNode->Handle = 0;
            
            m_SelectedFolder->Children.push_back(newNode);

            resetAndRepopulateTree();
    
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void ProjectWindow::drawImportPopup() {
    if (ImGui::BeginPopup("ImportPopup")) {
        if (ImGui::MenuItem("Texture")) {
            auto result = Editor::openFileDialog("png,jpg,jpeg", nullptr);
            if (result.has_value()) {
                std::filesystem::path filePath = result.value();
                std::filesystem::path destPath = m_SelectedFolder->Path / filePath.filename();

                std::error_code ec;
                std::filesystem::copy_file(filePath, destPath, std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    Core::Logger::log(Core::LogLevel::Error, "Failed to copy file: " + ec.message());
                    return;
                }

                auto newNode = std::make_shared<AssetNode>();
                newNode->Path = destPath;
                newNode->Parent = m_SelectedFolder;

                auto assetManager = Project::Project::getActiveProject()->getEditorAssetManager();
                std::filesystem::path relativePath = std::filesystem::relative(destPath, m_ProjectRoot->Path);
                Asset::AssetHandle handle = assetManager->importAsset(destPath);
                newNode->Handle = handle;

                m_SelectedFolder->Children.push_back(newNode);           
                
                resetAndRepopulateTree();
                assetManager->serializeAssetRegistry();
            }
        }
        ImGui::EndPopup();
    }
}

void ProjectWindow::render() {
    if (!m_ProjectRoot || !m_SelectedFolder)
        return;

    ImGui::Begin("Project Tree");
    drawFolderTree(m_ProjectRoot);
    ImGui::End();

    ImGui::Begin("File Browser");
    if (ImGui::Button("New")) ImGui::OpenPopup("NewPopup");
    ImGui::SameLine();
    if (ImGui::Button("Import")) ImGui::OpenPopup("ImportPopup");
    ImGui::SameLine();
    if (ImGui::Button("^")) navigateUp();

    ImGui::Text("%s", m_SelectedFolder->Path.string().c_str());

    drawNewAssetPopup();
    drawImportPopup();
    drawFileList(m_SelectedFolder);
    ImGui::End();
}

void ProjectWindow::onUpdate() {
    updateScriptWatchers();
}

void ProjectWindow::addScriptWatcher(const std::filesystem::path &script) {
    if (!std::filesystem::exists(script)) 
        return;

    ScriptWatcher watcher;
    watcher.Path = script;
    watcher.LastWriteTime = std::filesystem::last_write_time(script);

    m_WatchedScripts.push_back(watcher);
}

void ProjectWindow::updateScriptWatchers() {
    for (auto &watcher : m_WatchedScripts) {
        auto currentTime = std::filesystem::last_write_time(watcher.Path);
        if (currentTime != watcher.LastWriteTime) {
            watcher.LastWriteTime = currentTime;
            reloadScript(watcher.Path);
        }
    }
}

void ProjectWindow::reloadScript(const std::filesystem::path &script) {
    auto assetManager = Project::Project::getActiveProject()->getEditorAssetManager();
    Asset::AssetHandle handle = assetManager->getAssetHandleFromPath(script);
    auto instance = Asset::AssetManager::getAsset<Script::ScriptInstance>(handle);
    if (instance) {

        lua_State *L = Script::LuaEngine::getLuaState();
        std::vector<std::byte> bytecodeBuffer;

        if (luaL_loadfile(L, script.string().c_str()) != LUA_OK) {
            Core::Logger::log(Core::LogLevel::Error, 
                              std::string{"Lua compilation failed for "} + script.string() + ": " + lua_tostring(L, -1));
            lua_pop(L, 1);
            return;
        }

        if (lua_dump(L, luaBytecodeWriterWrapper, &bytecodeBuffer, false) != 0) {
            Core::Logger::log(Core::LogLevel::Error, "Failed to dump Lua bytecode to memory during reload: " + script.string());
            lua_pop(L, 1);
            return;
        }
        
        lua_pop(L, 1);

        if (!instance->loadSource(bytecodeBuffer)) {
            Core::Logger::log(Core::LogLevel::Error, "Failed to load *bytecode* into script: " + script.string());
            return;
        }

        Script::LuaEngine::getFieldRegistry().registerDefaultSerializableFields(instance->getName());
        Script::LuaEngine::getFieldRegistry().syncEntityFields(instance->getName(), *m_EditorLayer->getActiveScene());
    }
}

void ProjectWindow::recursivePopulateRoot(const std::shared_ptr<AssetNode> &node, uint32_t depth){
    constexpr uint32_t MAX_DEPTH = 16;
    if (depth == 0)
        m_WatchedScripts.clear();

    if (depth >= MAX_DEPTH) {
        Core::Logger::log(Core::LogLevel::Warning, "Project is too deep to draw Project Window");
        return;
    }

    std::shared_ptr<Asset::EditorAssetManager> assetManager = Project::Project::getActiveProject()->getEditorAssetManager();
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(node->Path)) {
            if (entry.path().filename().string().starts_with(".")) {
                continue;
            }
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                if (ext == ".roseproj" || ext == ".roseres" || ext == ".rosefields") {
                    continue;
                }
                if (ext == ".lua") {
                    std::string filename = entry.path().filename().string();

                    if (!filename.ends_with("_module.lua")) {
                        addScriptWatcher(entry.path());
                        Asset::AssetHandle handle = assetManager->getAssetHandleFromPath(entry.path());
                        auto instance = Asset::AssetManager::getAsset<Script::ScriptInstance>(handle);
                
                        if (instance) {
                            Script::LuaEngine::getFieldRegistry().registerDefaultSerializableFields(instance->getName());
                        }
                    }
                }
            }

            auto childNode = std::make_shared<AssetNode>();
            childNode->Path = entry.path();
            childNode->Parent = node;
            
            childNode->Handle = assetManager->getAssetHandleFromPath(entry.path());

            if (std::filesystem::is_directory(entry.path())) {
                recursivePopulateRoot(childNode, depth + 1);
            }
            
            node->Children.push_back(childNode);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::string fmt = "Filesystem error: " + std::string{e.what()};
        Core::Logger::log(Core::LogLevel::Error, fmt);
    }
}

void ProjectWindow::resetTree(std::shared_ptr<AssetNode> &node) {
    if (!node) return;

    for (auto &child : node->Children) {
        resetTree(child);
    }

    node->Children.clear();
}

void ProjectWindow::resetAndRepopulateTree() {
    auto selectedPath = m_SelectedFolder ? m_SelectedFolder->Path : std::filesystem::path{};

    resetTree(m_ProjectRoot);
    recursivePopulateRoot(m_ProjectRoot, 0);

    if (!selectedPath.empty()) {
        std::function<std::shared_ptr<AssetNode>(const std::shared_ptr<AssetNode>&)> findNode;
        findNode = [&](const std::shared_ptr<AssetNode>& node) -> std::shared_ptr<AssetNode> {
            if (node->Path == selectedPath)
                return node;
            for (auto& child : node->Children) {
                auto found = findNode(child);
                if (found) return found;
            }
            return nullptr;
        };
    
        auto restored = findNode(m_ProjectRoot);
        m_SelectedFolder = restored ? restored : m_ProjectRoot;
    }
}

void ProjectWindow::setProjectRoot(const std::filesystem::path &root) {
    m_ProjectRoot = std::make_shared<AssetNode>();
    m_ProjectRoot->Path = root;
    m_ProjectRoot->Parent.reset();

    m_SelectedFolder = m_ProjectRoot;
    resetTree(m_ProjectRoot);
    recursivePopulateRoot(m_ProjectRoot, 0);
}

void ProjectWindow::onFolderClick(const std::shared_ptr<AssetNode>& folder) {
    m_SelectedFolder = folder;
}

void ProjectWindow::onFileClick(const std::shared_ptr<AssetNode>& file) {
    
}

void ProjectWindow::navigateUp() {
    if (auto parent = m_SelectedFolder->Parent.lock()) {
        m_SelectedFolder = parent;
    }
}

}
