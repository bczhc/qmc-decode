//
// Created by bczhc on 25/06/22.
//

#include "lib.h"
#include "def.h"
#include <cassert>
#include <filesystem>

String getFileExtension(const char *file) {
    String extension;
    auto ext = std::filesystem::path(file).extension().string();
    extension = ext.c_str();
    size_t length = extension.length();
    if (length != 0) {
        assert(extension[0] == '.');
        extension = extension.substring(1, length);
    }
#pragma clang diagnostic push
#pragma ide diagnostic ignored "LocalValueEscapesScope"
    return extension;
#pragma clang diagnostic pop
}
