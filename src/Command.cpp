/************************************************************************
 * Copyright(c) 2024  technology
 * 
 * File        :  Command.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-07-18 22:33:17
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include "include/Command.h"
#include "include/Logger.h"

namespace infra {

std::string command(const char* cmd, ...) {
#ifdef _WIN32
    return std::string();
#else
    va_list ap;
    va_start(ap, cmd);
    char buffer[1024] = { 0 };
    if (vsnprintf(buffer, sizeof(buffer), cmd, ap) <= 0) {
        return std::string();
    }
    va_end(ap);

    std::string result;
    char tmp[1024] = {0};
    FILE *fcmd = popen(buffer, "r");
    if (fcmd == nullptr) {
        errorf("cmd %s failed\n", buffer);
        return std::string();
    }

    while (fgets(tmp, sizeof(tmp), fcmd) != nullptr) {
        result += tmp;
    }

    pclose(fcmd);
    return result;
#endif
}

}