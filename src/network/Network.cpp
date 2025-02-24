#include "infra/include/network/Network.h"
#include "infra/include/network/Defines.h"
#include "infra/include/Logger.h"
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <iphlpapi.h>
#pragma comment(lib, "Iphlpapi.lib")
#else
#include <signal.h>
#include <unistd.h>
#endif

namespace infra {

bool network_init() {
#ifdef _WIN32
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(wVersionRequested, &wsaData);
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    return true;
}

bool network_deinit() {
#ifdef _WIN32
    WSACleanup();
#endif
    return true;
}

#ifdef _WIN32
template<typename FUN>
static void for_each_netAdapter_win32(FUN && fun) { //type: PIP_ADAPTER_INFO
    unsigned long nSize = sizeof(IP_ADAPTER_INFO);
    PIP_ADAPTER_INFO adapterList = (PIP_ADAPTER_INFO)new char[nSize];
    int nRet = GetAdaptersInfo(adapterList, &nSize);
    if (ERROR_BUFFER_OVERFLOW == nRet) {
        delete[] adapterList;
        adapterList = (PIP_ADAPTER_INFO)new char[nSize];
        nRet = GetAdaptersInfo(adapterList, &nSize);
    }
    auto adapterPtr = adapterList;
    while (adapterPtr && ERROR_SUCCESS == nRet) {
        if (fun(adapterPtr)) {
            break;
        }
        adapterPtr = adapterPtr->Next;
    }
    //释放内存空间
    delete[] adapterList;
}
#else
template<typename FUN>
static void for_each_netAdapter_posix(FUN &&fun) { //type: struct ifaddrs *
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *adapter = nullptr;
    if (getifaddrs(&interfaces) == 0) {
        adapter = interfaces;
        while (adapter) {
            if (adapter->ifa_addr && adapter->ifa_addr->sa_family == AF_INET) {
                if (fun(adapter)) {
                    break;
                }
            }
            adapter = adapter->ifa_next;
        }
        freeifaddrs(interfaces);
    }
}
#endif


static bool getInterfaceMac(const char *if_name, std::string &mac) {
#ifdef _WIN32
#else
    struct ifreq ifreq;
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        errorf("socket fail\n");
        return false;
    }
    strncpy(ifreq.ifr_name, if_name, sizeof(ifreq.ifr_name));
    if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0) {
        errorf("ioctl SIOCGIFHWADDR %s fail\n", if_name);
        close(sock);
        return -1;
    }
    char tmp[32] = {0};
    unsigned char *p = (unsigned char *)ifreq.ifr_hwaddr.sa_data;
    sprintf((char *)tmp, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
    close(sock);
    mac = tmp;
#endif
    return 0;
}

std::vector<Interface> getInterfaceList() {
    std::vector<Interface> result;

#ifdef _WIN32
    for_each_netAdapter_win32([&](PIP_ADAPTER_INFO adapter) {
        IP_ADDR_STRING *ipAddr = &(adapter->IpAddressList);
        while (ipAddr) {
            Interface interface0;
            interface0.ip = ipAddr->IpAddress.String;
            interface0.netmask = ipAddr->IpMask.String;
            interface0.name = adapter->AdapterName;
            result.emplace_back(std::move(interface0));
            ipAddr = ipAddr->Next;
        }
        return false;
    });
#else
    for_each_netAdapter_posix([&](struct ifaddrs *adapter) {
        if (IFF_LOOPBACK == (adapter->ifa_flags & IFF_LOOPBACK)) {
            //skip loopback
            return false;
        }
        char buffer[64] = {0};
        Interface interface;
        interface.name = adapter->ifa_name;

        getInterfaceMac(adapter->ifa_name, interface.mac);

        int32_t family = adapter->ifa_addr->sa_family;
        if (family == AF_INET) {
            memset(buffer, 0x00, sizeof(buffer));
            struct sockaddr_in *pstIPv4Addr = (struct sockaddr_in *)adapter->ifa_addr;
            inet_ntop(family, &pstIPv4Addr->sin_addr, buffer, INET_ADDRSTRLEN);
            interface.ip = buffer;

            memset(buffer, 0x00, sizeof(buffer));
			pstIPv4Addr = (struct sockaddr_in *)adapter->ifa_netmask;
			inet_ntop(family, &pstIPv4Addr->sin_addr, buffer, INET_ADDRSTRLEN);
            interface.netmask = buffer;

            memset(buffer, 0x00, sizeof(buffer));
			pstIPv4Addr = (struct sockaddr_in *)adapter->ifa_broadaddr;
			inet_ntop(family, &pstIPv4Addr->sin_addr, buffer, INET_ADDRSTRLEN);
            interface.broadaddr = buffer;
        } else if (family == AF_INET6) {
            memset(buffer, 0x00, sizeof(buffer));
			struct sockaddr_in6 *pstIPv6Addr = (struct sockaddr_in6 *)adapter->ifa_addr;
			inet_ntop(family, &pstIPv6Addr->sin6_addr, buffer, INET6_ADDRSTRLEN);
            interface.ip = buffer;

            memset(buffer, 0x00, sizeof(buffer));
			pstIPv6Addr = (struct sockaddr_in6 *)adapter->ifa_netmask;
			inet_ntop(family, &pstIPv6Addr->sin6_addr, buffer, INET6_ADDRSTRLEN);
            interface.netmask = buffer;
        }
        result.push_back(std::move(interface));
        return false;
    });
#endif
    return result;
}

std::string getIpv4DefaultGateway() {
    std::string gateway;
#ifdef _WIN32
    errorf("win32 not impl getIpv4DefaultGateway\n");
#else
    FILE *fp = fopen("/proc/net/route", "r");
    if (fp == nullptr) {
        errorf("get default geteway open /proc/net/route error");
        return gateway;
    }
    char buffer[128] = {0};
    char ifname[32] = {0};
    unsigned long dest_addr = 0, gateway_addr = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (sscanf(buffer, "%s\t%lX\t%lX", ifname, &dest_addr, &gateway_addr) != 3 || dest_addr != 0) {
            continue;
        }
        char ip[32] = {0};
        snprintf(ip, sizeof(ip), "%d.%d.%d.%d", gateway_addr & 0xff, (gateway_addr >> 8) & 0xff, (gateway_addr >> 16) & 0xff, (gateway_addr >> 24) & 0xff);
        gateway = ip;
        infof("ipv4 default gateway:%s, if:%s\n", ip, ifname);
        break;
    }
    fclose(fp);
#endif
    return gateway;
}

}
