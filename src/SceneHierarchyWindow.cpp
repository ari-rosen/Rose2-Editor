#include "SceneHierarchyWindow.hpp"
#include "RoseV2/ECS/Entity.hpp"
#include "RoseV2/ECS/Component.hpp"
#include "RoseV2/Graphics/Texture.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>

namespace RoseV2::Editor {

void SceneHierarchyWindow::render() {
    if (!m_Registry) return;
    if (!m_Scene) return;

    static std::shared_ptr<Graphics::Texture> cubeIcon = nullptr;
    static std::filesystem::path cubePath;
    #ifdef EDITOR_ASSET_DIR
        cubePath = std::filesystem::path(std::string(EDITOR_ASSET_DIR)) / "cube.png";
    #endif
    if (!cubeIcon) {
        cubeIcon = Graphics::Texture::create(cubePath);
    }

    ImGui::Begin("Hierarchy");

    if (ImGui::Button("New (+)")) {
        ECS::Entity e = m_Registry->createEntity();
        m_Registry->addComponent(e, ECS::TagComponent{ "Entity" });
        m_Registry->addComponent(e, ECS::TransformComponent{});
        m_SelectedEntity = e;
    }
    ImGui::SameLine();

    if (ImGui::Button("Delete Selected (-)")) {
        if (m_SelectedEntity != ECS::Entity::Null()) {
            ECS::Entity entityToDestroy = m_SelectedEntity;
            m_SelectedEntity = ECS::Entity::Null(); 
            m_Registry->destroyEntity(entityToDestroy);
        }

    }

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

    const char* sceneName = m_Scene->getName().c_str();

    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen 
                                | ImGuiTreeNodeFlags_OpenOnArrow
                                | ImGuiTreeNodeFlags_Framed ;   

    if (ImGui::TreeNodeEx((void*)sceneName, rootFlags, "%s", sceneName)) {
        for (ECS::Entity e : m_Registry->view<ECS::TagComponent>()) {
            if (!m_Registry->isEntityAlive(e))
                continue;
            
            const std::string name = m_Registry->getComponent<ECS::TagComponent>(e).Tag;
            ImGuiTreeNodeFlags flags = ((e == m_SelectedEntity) ? ImGuiTreeNodeFlags_Selected : 0)
                                    | ImGuiTreeNodeFlags_Leaf
                                    | ImGuiTreeNodeFlags_SpanFullWidth
                                    | ImGuiTreeNodeFlags_NoTreePushOnOpen;

            ImGui::PushID((void*)(uint64_t)(uint32_t)e.id);
            ImGui::Image((void*)(intptr_t)cubeIcon->getRendererID(), 
                        ImVec2(16,16),
                        ImVec2(0,0), 
                        ImVec2(1,1), 
                        ImVec4(0.80f, 0.28f, 0.29f, 1.00f),
                        ImVec4(0.0f, 0.0f, 0.0f, 0.0f)
            );
            ImGui::SameLine(0.0f, 4.0f);

            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 1.0f, 1.0f, 0.3));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.3f));
            ImGui::TreeNodeEx("##node", flags, "%s", name.c_str());
            ImGui::PopStyleColor(3);

            if (ImGui::IsItemClicked())
                m_SelectedEntity = e;

            ImGui::PopID();
        }

        ImGui::TreePop();
    }

    ImGui::EndChild();

    ImGui::End();
}

}
