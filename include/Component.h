/************************************************************************
 * Copyright(c) 2024 technology
 * 
 * File        :  Component.h
 * Author      :  mengshunxiang 
 * Data        :  2025-02-11 19:21:26
 * Description :  None
 * Note        : 
 ************************************************************************/
namespace infra {
namespace component {
class IUnknown {
public:
    IUnknown() {};
    virtual ~IUnknown() {};
};

class IFactoryUnknown {
public:
    virtual ~IFactoryUnknown() {};
    bool registerFactory(const char* iid);
    bool unregisterFactory(const char* iid);
};

IFactoryUnknown* getComponentFactory(const char* iid);

template<class T>
inline typename T::IFactory* detailGetComponentFactory() {
    return dynamic_cast<typename T::IFactory*>(getComponentFactory(T::IFactory::iid()));
}

template<class T>
inline T* getComponentInstance() {
    typename T::IFactory* cf = detailGetComponentFactory<T>();
    if (cf) {
        return cf->instance();
    }
    return nullptr;
}

#define COMPONENT_UNIMPLEMENTED_OPERATION(x)        \
{                                                   \
	errorf("not implement %s\n", #x);               \
	return false;                                   \
}

}
}
