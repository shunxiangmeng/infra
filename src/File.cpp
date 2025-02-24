#include <string>
#include <string.h>
#include "infra/include/Utils.h"
#include "infra/include/File.h"
#include "infra/include/Logger.h"
#if defined(_WIN32)
#include <io.h>   
#include <direct.h>
#else
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#endif // WIN32

#if !defined(_WIN32)
#define    _unlink    unlink
#define    _rmdir     rmdir
#define    _access    access
#endif

namespace infra {

#if defined(_WIN32)
int mkdir(const char *path, int mode) {
    return _mkdir(path);
}
#endif // defined(_WIN32)

bool File::createPath(const char *file, unsigned int mod) {
    std::string path = file;
    std::string dir;
    size_t index = 1;
    while (1) {
        index = path.find('/', index) + 1;
        dir = path.substr(0, index);
        if (dir.length() == 0) {
            break;
        }
        if (_access(dir.c_str(), 0) == -1) {      //access函数是查看是不是存在
            if (mkdir(dir.c_str(), mod) == -1) {  //如果不存在就用mkdir函数来创建
                errorf("mkdir %s failed\n", dir.c_str());
                return false;
            }
        }
    }
    return true;    
}

size_t File::fileSize(const char *path) {
    FILE *fp = fopen (path, "rb");
    if (fp == NULL) {
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fclose(fp);
    return size;
}

std::string File::loadFile(const std::string &path) {
    FILE *fp = fopen(path.data(), "rb");
    if (!fp) {
        return {};
    }
    fseek(fp, 0, SEEK_END);
    auto len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*)malloc(len + 1);
    if (buffer == nullptr) {
        return {};
    }
    buffer[len] = '\0';
    if (len != (decltype(len))fread(buffer, sizeof(char), len, fp)) {
        warnf("fread %s failed\n", path.data());
    }
    std::string content = buffer;
    free(buffer);
    fclose(fp);
    return content;
}

std::string File::loadFile(const char* path) {
    std::string path_str(path);
    return loadFile(path_str);
}

bool File::getFileNoPathNameAndSize(const std::string &path, std::string &no_path_name, int32_t &size) {
    FILE *fp = fopen(path.data(), "rb");
    if (!fp) {
        return false;
    }
    fseek(fp, 0, SEEK_END);
    size = (int32_t)ftell(fp);
    fclose(fp);
    no_path_name = noPathFileName(path);
    return true;
}

#ifdef _WIN32
#else
static void traverseDirectoryImplement(const std::string& directory, std::vector<std::string>& result, const char* suffix = nullptr) {
    DIR* dir = opendir(directory.data());
    if (dir == nullptr) {
        errorf("Failed to open directory: %s\n", directory.data());
        return;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string entryName = entry->d_name;
        if (entryName == "." || entryName == "..") {
            continue;
        }

        std::string fullPath = directory + "/" + entryName;
        struct stat statbuf;
        if (stat(fullPath.c_str(), &statbuf) == -1) {
            errorf("Failed to get file status: %s\n", fullPath);;
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // 如果是目录，递归遍历
            traverseDirectoryImplement(fullPath, result, suffix);
        } else if (S_ISREG(statbuf.st_mode)) {
            if (suffix) {
                std::string file_suffix = fullPath.substr(fullPath.size() - strlen(suffix));
                if (file_suffix == suffix) {
                    tracef("found %s file, %2d, %s\n", suffix, result.size(), fullPath.data());
                    result.push_back(fullPath);
                }
            } else {
                tracef("found file %2d, %s\n", result.size(), fullPath.data());
                result.push_back(fullPath);
            }
        }
    }
    closedir(dir);
}
#endif

std::vector<std::string> File::traverseDirectory(const char* directory, const char* suffix) {
    std::vector<std::string> result;
#ifdef _WIN32
    //todo 
    errorf("win32 not impl traverseDirectoryImplement\n");
#else
    traverseDirectoryImplement(directory, result, suffix);
#endif
    return result;
}

}