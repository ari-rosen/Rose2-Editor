#include "InspectorWindow.hpp"
#include "EditorLayer.hpp"

#include <imgui.h>

#include "RoseV2/Core/UUID.hpp"
#include "RoseV2/ECS/Component.hpp"
#include "RoseV2/Project/Project.hpp"
#include "RoseV2/Script/LuaEngine.hpp"
#include "RoseV2/Script/ScriptFieldRegistry.hpp"

namespace RoseV2::Editor {

void InspectorWindow::render() {
    if (!m_Registry) return;

    static ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen;

    ImGui::Begin("Inspector");
    
    if (ImGui::Button("Add Component")) {
        if (m_SelectedEntity != ECS::Entity::Null())
            ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
        if (ImGui::MenuItem("Sprite")) {
            if (!m_Registry->hasComponent<ECS::SpriteComponent>(m_SelectedEntity)) {
                m_Registry->addComponent<ECS::SpriteComponent>(m_SelectedEntity, {});
            }
        }
        if (ImGui::MenuItem("Camera")) {
            if (!m_Registry->hasComponent<ECS::CameraComponent>(m_SelectedEntity)) {
                if (!m_Framebuffer) {
                    m_Registry->addComponent<ECS::CameraComponent>(m_SelectedEntity, {});
                }
                else {
                    auto spec = m_Framebuffer->getSpecification();
                    ECS::CameraComponent c;
                    c.Right = spec.Width;
                    c.Left = -spec.Width;

                    m_Registry->addComponent<ECS::CameraComponent>(m_SelectedEntity, c);
                }
            }
        }
        if (ImGui::MenuItem("Transform")) {
            if (!m_Registry->hasComponent<ECS::TransformComponent>(m_SelectedEntity)) {
                m_Registry->addComponent<ECS::TransformComponent>(m_SelectedEntity, {});
            }
        }
        if (ImGui::MenuItem("Script")) {
            if (!m_Registry->hasComponent<ECS::ScriptComponent>(m_SelectedEntity)) {
                m_Registry->addComponent<ECS::ScriptComponent>(m_SelectedEntity, {});
            }
            if (!m_Registry->hasComponent<ECS::UUIDComponent>(m_SelectedEntity)) {
                m_Registry->addComponent<ECS::UUIDComponent>(m_SelectedEntity, {.ID=Core::UUID{}});
            }
        }

        ImGui::EndPopup();
    }

    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

    if (m_SelectedEntity == ECS::Entity::Null()) {
        ImGui::Text("No entity selected");
    } else {
        // TagComponent
        if (m_Registry->hasComponent<ECS::TagComponent>(m_SelectedEntity)) {
            auto& tag = m_Registry->getComponent<ECS::TagComponent>(m_SelectedEntity);
            char buffer[128];
            strncpy(buffer, tag.Tag.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = 0;

            if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
                tag.Tag = std::string(buffer);
            }
        }

        ImGui::Separator();

        // TransformComponent
        if (m_Registry->hasComponent<ECS::TransformComponent>(m_SelectedEntity)) {
            auto& transform = m_Registry->getComponent<ECS::TransformComponent>(m_SelectedEntity);

            bool isCamera = m_Registry->hasComponent<ECS::CameraComponent>(m_SelectedEntity);
            
            if (ImGui::TreeNodeEx("Transform", flags)) {
                if (!isCamera) {
                    ImGui::DragFloat3("Position", &transform.Translation.x, 0.01f);
                    ImGui::DragFloat3("Rotation", &transform.Rotation.x, 0.1f);
                    ImGui::DragFloat3("Scale", &transform.Scale.x, 0.01f);
                }
                else {
                    ImGui::DragFloat2("Position", &transform.Translation.x, 0.01f);
                    ImGui::DragFloat("Rotation", &transform.Rotation.z, 0.1f);
                }
                ImGui::TreePop();
            }
        }

        // SpriteComponent
        if (m_Registry->hasComponent<ECS::SpriteComponent>(m_SelectedEntity)) {
            auto& sprite = m_Registry->getComponent<ECS::SpriteComponent>(m_SelectedEntity);

            if (ImGui::TreeNodeEx("Sprite", flags)) {
                ImGui::ColorEdit4("Color", &sprite.Color.x);

                char buffer[128] = "No Texture";

                if (sprite.TextureHandle != 0) {
                    auto assetManager = Project::Project::getActiveProject()->getEditorAssetManager();
                    auto& metadata = assetManager->getAssetMetadata(sprite.TextureHandle);
                    strncpy(buffer, metadata.Path.filename().string().c_str(), sizeof(buffer));
                    buffer[sizeof(buffer)-1] = 0;
                }

                ImGui::InputText("##dropfield", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);

                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Texture Handle")) {
                        IM_ASSERT(payload->DataSize == sizeof(uint64_t));
                        uint64_t data = *static_cast<const uint64_t*>(payload->Data);
                        sprite.TextureHandle = data;
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::SameLine();
                ImGui::Text("Texture");
                ImGui::TreePop();
            }
        }

        // CameraComponent
        if (m_Registry->hasComponent<ECS::CameraComponent>(m_SelectedEntity)) {
            auto &camera = m_Registry->getComponent<ECS::CameraComponent>(m_SelectedEntity);
            
            if (ImGui::TreeNodeEx("Camera", flags)) {
                ImGui::DragFloat("Zoom", &camera.Zoom, 0.01f, 0.0f);

                bool temp = camera.Primary;
                if (ImGui::Checkbox("Primary", &temp)) {
                    camera.Primary = temp;
                }
                ImGui::TreePop();
            }
        }

        // ScriptComponent
        if (m_Registry->hasComponent<ECS::ScriptComponent>(m_SelectedEntity)) {
            auto &sc = m_Registry->getComponent<ECS::ScriptComponent>(m_SelectedEntity);

            if (ImGui::TreeNodeEx("Script", flags)) {
                char buffer[128] = "No Script";
                
                if (sc.ScriptHandle != 0) {
                    auto assetManager = Project::Project::getActiveProject()->getEditorAssetManager();
                    auto& metadata = assetManager->getAssetMetadata(sc.ScriptHandle);
                    strncpy(buffer, metadata.Path.filename().string().c_str(), sizeof(buffer));
                    buffer[sizeof(buffer)-1] = 0;
                }

                ImGui::InputText("##dropfield", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);

                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Script Handle")) {
                        IM_ASSERT(payload->DataSize == sizeof(uint64_t));
                        uint64_t data = *static_cast<const uint64_t*>(payload->Data);
                        sc.ScriptHandle = data;
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::SameLine();
                ImGui::Text("Script");
                    
                if (sc.ScriptHandle != 0) {

                    std::string scriptName = "";
                    auto assetManager = Project::Project::getActiveProject()->getEditorAssetManager();
                    auto& metadata = assetManager->getAssetMetadata(sc.ScriptHandle);
                    scriptName = metadata.Path.stem().string();
                    
                    Core::UUID uuid = m_EditorLayer->getActiveScene()->getRegistry().getComponent<ECS::UUIDComponent>(m_SelectedEntity).ID;
                    auto& fields = Script::LuaEngine::getFieldRegistry().getEntitySerializeableFields(uuid, scriptName);
                    
                    for (Script::SerializableField &field : fields) {
                        switch (field.Type) {
                            case Script::FieldType::Number: {
                                double numberValue = std::get<double>(field.DefaultValue);
                                if (ImGui::DragScalar(field.Name.c_str(), ImGuiDataType_Double, &numberValue, 0.1))
                                    field.DefaultValue = numberValue;
                                break;
                            }
                                
                            case Script::FieldType::Bool: {
                                bool boolValue = std::get<bool>(field.DefaultValue);
                                if (ImGui::Checkbox(field.Name.c_str(), &boolValue)) {
                                    field.DefaultValue = boolValue;
                                }
                                break;
                            }
                                
                            case Script::FieldType::String: {
                                std::string value = std::get<std::string>(field.DefaultValue);
                                char buffer[256];
                                strncpy(buffer, value.c_str(), sizeof(buffer));
                                buffer[sizeof(buffer) - 1] = 0;
                
                                if (ImGui::InputText(field.Name.c_str(), buffer, sizeof(buffer)))
                                {
                                    field.DefaultValue = std::string(buffer);
                                }
                                break;
                            }
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
    }

    ImGui::EndChild();

    ImGui::End();
}

}
