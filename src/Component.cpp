/************************************************************************
 * Copyright(c) 2024 technology
 * 
 * File        :  Component.cpp
 * Author      :  mengshunxiang 
 * Data        :  2025-02-11 19:23:38
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <mutex>
#include <map>
#include "infra/include/Component.h"
#include "infra/include/Logger.h"

namespace infra {
namespace component {

typedef std::map<std::string, IFactoryUnknown*> FactoryMap;
static FactoryMap s_interfaces;
static std::recursive_mutex s_mutex;

bool IFactoryUnknown::registerFactory(const char* iid) {
    std::lock_guard<std::recursive_mutex> guard(s_mutex);
    s_interfaces[iid] = this;
    return true;
}

bool IFactoryUnknown::unregisterFactory(const char* iid) {
    std::lock_guard<std::recursive_mutex> guard(s_mutex);
    s_interfaces[iid] = NULL;
    return true;
}

IFactoryUnknown* getComponentFactory(const char* iid) {
    std::lock_guard<std::recursive_mutex> guard(s_mutex);			
    auto it = s_interfaces.find(iid);
    if (it == s_interfaces.end()) {
        warnf("get component factory %s failed\n", iid);
        return nullptr;
    }
    return it->second;
}

}
}
