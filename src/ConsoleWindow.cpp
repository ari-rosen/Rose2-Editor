#include "ConsoleWindow.hpp"

#include <imgui.h>

namespace RoseV2::Editor {

void ConsoleWindow::render() {
    ImGui::Begin("Console");

    std::string content = m_Buffer.str();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::TextUnformatted(content.c_str());

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

}
