/************************************************************************
 * Copyright(c) 2024  technology
 * 
 * File        :  Utils.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-18 15:49:45
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "include/Utils.h"
#ifdef _WIN32
#include <winsock.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
extern "C" const IMAGE_DOS_HEADER __ImageBase;
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/syscall.h>        //gettid
#endif // _WIN32

namespace infra {

uint32_t infra_htonl(uint32_t value) {
    return htonl(value);
}

uint16_t infra_htons(uint16_t value) {
    return htons(value);
}

uint32_t infra_ntohl(uint32_t value) {
    return ntohl(value);
}

uint16_t infra_ntohs(uint16_t value) {
    return ntohs(value);
}

int32_t getCurrentThreadId() {
#ifdef _WIN32
    return  GetCurrentThreadId();
    //#define getCurrentThreadId() std::this_thread::get_id()
#else
    return (int32_t)syscall(SYS_gettid);
#endif
}

#if defined(__MACH__) || defined(__APPLE__)
#include <limits.h>
#include <mach-o/dyld.h> /* _NSGetExecutablePath */

int uv_exepath(char *buffer, int *size) {
    /* realpath(exepath) may be > PATH_MAX so double it to be on the safe side. */
    char abspath[PATH_MAX * 2 + 1];
    char exepath[PATH_MAX + 1];
    uint32_t exepath_size;
    size_t abspath_size;

    if (buffer == nullptr || size == nullptr || *size == 0) {
        return -EINVAL;
    }

    exepath_size = sizeof(exepath);
    if (_NSGetExecutablePath(exepath, &exepath_size)) {
        return -EIO;
    }

    if (realpath(exepath, abspath) != abspath) {
        return -errno;
    }

    abspath_size = strlen(abspath);
    if (abspath_size == 0) {
        return -EIO;
    }

    *size -= 1;
    if ((size_t) *size > abspath_size) {
        *size = abspath_size;
    }

    memcpy(buffer, abspath, *size);
    buffer[*size] = '\0';

    return 0;
}

#endif //defined(__MACH__) || defined(__APPLE__)

std::string exePath(bool isExe /*= true*/) {
    char buffer[256 + 1] = {0};
    int n = -1;
#if defined(_WIN32)
    n = GetModuleFileNameA(isExe ? nullptr:(HINSTANCE)&__ImageBase, buffer, sizeof(buffer));
#elif defined(__MACH__) || defined(__APPLE__)
    n = sizeof(buffer);
    if (uv_exepath(buffer, &n) != 0) {
        n = -1;
    }
#elif defined(__linux__)
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif
    std::string file_path;
    if (n <= 0) {
        file_path = "./";
    } else {
        file_path = buffer;
    }

#if defined(_WIN32)
    //windows下把路径统一转换层unix风格，因为后续都是按照unix风格处理的
    for (auto &ch : file_path) {
        if (ch == '\\') {
            ch = '/';
        }
    }
#endif //defined(_WIN32)
    return file_path;
}

}