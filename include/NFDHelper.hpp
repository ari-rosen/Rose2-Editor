#pragma once

#include "RoseV2/pch.hpp"

namespace RoseV2::Editor {

std::optional<std::string> openFileDialog(const char* filterList, const char* defaultPath);
std::optional<std::string> pickFolderDialog(const char* defaultPath);

}
