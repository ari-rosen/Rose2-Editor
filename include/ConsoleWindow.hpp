#pragma once

#include "EditorWindow.hpp"
#include "RoseV2/pch.hpp"

namespace RoseV2::Editor {

class ConsoleWindow : public EditorWindow {
public:
    void render() override;
    
    inline std::stringstream& getBuffer() { return m_Buffer; }
private:
    std::stringstream m_Buffer;
};

}

