#define _CRT_SECURE_NO_WARNINGS
#include "api.h"
#include <sstream>
#include <winsock2.h> // Windows套接字库
#include <Windows.h>
#include <iphlpapi.h>
#include <boost/asio.hpp>
#include <vector>
#include <string>
#include "shared.h"
#include <wx/msgdlg.h>
#include <locale>
#include <iostream>
#include <codecvt>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

Timer::Timer() : isRunning(false) {}

void Timer::start() {
    startTime = std::chrono::high_resolution_clock::now();
    isRunning = true;
}

void Timer::stop() {
    if (isRunning) {
        endTime = std::chrono::high_resolution_clock::now();
        isRunning = false;
    }
}

double Timer::getDuration(const std::string& unit) const {
    std::chrono::duration<double> duration;
    if (isRunning) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        duration = currentTime - startTime;
    }
    else {
        duration = endTime - startTime;
    }

    if (unit == "milliseconds") {
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        return milliseconds.count();
    }
    else {
        return duration.count(); // 默认返回秒
    }
}

std::string Timer::getStartTime() const {
    return formatTime(std::chrono::system_clock::now());
}

std::string Timer::getEndTime() const {
    return formatTime(std::chrono::system_clock::now());
}

std::string Timer::formatTime(std::chrono::system_clock::time_point timePoint) const {
    std::time_t now_time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm local_tm;
    localtime_s(&local_tm, &now_time);
    char timeBuffer[100];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &local_tm);
    return std::string(timeBuffer);
}

// 路径拼接函数
std::string path_join(const std::string& path1, const std::string& path2) {
    // 如果 path1 以反斜杠结尾，直接连接
    if (!path1.empty() && path1.back() == '\\') {
        return path1 + path2;
    }
    // 如果 path2 以反斜杠开头，去掉 path1 的结尾反斜杠
    if (!path2.empty() && path2.front() == '\\') {
        return path1 + path2;
    }
    // 否则，使用反斜杠连接两个路径
    return path1 + "\\" + path2;
}

// 命令行模式控制台窗口
void OpenConsoleWindow() {
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    freopen("CON", "w", stdout);
    freopen("CON", "r", stdin);
}

void CloseConsoleWindow() {
    printf("服务器已关闭，您可以关闭这个窗口。\n");
    FreeConsole();
}


std::string ConvertWStringToString(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, nullptr, nullptr);
    return str;
}

// 获取网卡信息
std::vector<std::pair<std::string, std::string>> GetNetworkInterfaces() {
    std::vector<std::pair<std::string, std::string>> interfaces;
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES addresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);

    if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, addresses, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        free(addresses);
        addresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    }

    if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, addresses, &bufferSize) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES addr = addresses; addr != nullptr; addr = addr->Next) {
            std::string name = ConvertWStringToString(addr->FriendlyName);
            for (PIP_ADAPTER_UNICAST_ADDRESS ua = addr->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
                char ip[INET6_ADDRSTRLEN];
                if (getnameinfo(ua->Address.lpSockaddr, ua->Address.iSockaddrLength, ip, sizeof(ip), nullptr, 0, NI_NUMERICHOST) == 0) {
                    interfaces.push_back(std::make_pair(name,ip));
                }
            }
        }
    }
    free(addresses);
    return interfaces;
}

std::string GetLastErrorAsString() {
    // 获取错误代码
    DWORD errorMessageID = WSAGetLastError();
    if (errorMessageID == 0) {
        return "端口被占用"; 
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

    std::string message(messageBuffer, size);

    // 释放缓冲区
    LocalFree(messageBuffer);

    return message;
}

// 绑定套接字到特定的网络接口
std::string BindSocketToInterface(SOCKET* sock, bool isIPV6, const std::string& ipAddress, int port) {
    struct sockaddr_in service;

    // 设置套接字地址和端口
    if (isIPV6) {
		return "IPV6 is not supported";
    }
    else {
        service.sin_family = AF_INET;
		if (ipAddress == std::string("All"))service.sin_addr.s_addr = INADDR_ANY; // 所有接口
        else if (InetPtonA(AF_INET, ipAddress.c_str(), &service.sin_addr) != 1) {
            return "Invalid IPV4 address format : " + ipAddress;
        }
		service.sin_port = htons(port);
    }

    // 绑定套接字到特定的网络接口
    if (bind(*sock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        closesocket(*sock);
        WSACleanup();
        return GetLastErrorAsString();
    }
    return "OK";
}






