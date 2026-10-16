#pragma once

namespace RoseV2::Editor {

class EditorWindow {
public:
    virtual ~EditorWindow() = default;

    virtual void render() = 0;
};

}

