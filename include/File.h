#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace infra {

class File {
public:
    static bool createPath(const char *file, unsigned int mod);
    static size_t fileSize(const char *file);
    static std::string loadFile(const std::string &path);
    static std::string loadFile(const char* path);
    static bool getFileNoPathNameAndSize(const std::string &path, std::string &no_path_name, int32_t &size);
    static std::vector<std::string> traverseDirectory(const char* directory, const char* suffix = nullptr);
}; 
}