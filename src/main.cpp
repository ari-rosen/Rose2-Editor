#include "EditorLayer.hpp"

#include "RoseV2/Core/Application.hpp"

#include <memory>

int main() {
    RoseV2::Core::Application app("RoseV2 Editor", 1280, 720);
    app.pushLayer(std::make_unique<RoseV2::Editor::EditorLayer>(&app));
    app.run();
}
