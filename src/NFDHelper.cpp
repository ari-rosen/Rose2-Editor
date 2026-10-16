#include "NFDHelper.hpp"

#include <nfd.h>

namespace RoseV2::Editor {

std::optional<std::string> openFileDialog(const char* filterList, const char* defaultPath) {
    nfdchar_t* outPath = nullptr;
    nfdresult_t result = NFD_OpenDialog(filterList, defaultPath, &outPath);

    if (result == NFD_OKAY) {
        std::string filePath(outPath);
        free(outPath);
        return filePath;
    } else if (result == NFD_CANCEL) {
        return std::nullopt;
    } else {
        std::string error = NFD_GetError();
        return std::nullopt;
    }
}

std::optional<std::string> pickFolderDialog(const char* defaultPath) {
    nfdchar_t* outPath = nullptr;
    nfdresult_t result = NFD_PickFolder(defaultPath, &outPath);
    
    if (result == NFD_OKAY) {
        std::string folderPath(outPath);
        free(outPath);
        return folderPath;
    } else if (result == NFD_CANCEL) {
        return std::nullopt;
    } else {
        std::string error = NFD_GetError();
        return std::nullopt;
    }
}

}
