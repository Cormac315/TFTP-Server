#pragma once
#include <chrono>
#include <ctime>
#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h> // Windows套接字库

// 计时器
class Timer {
public:
    Timer();
    void start();
    void stop();
	double getDuration(const std::string& unit = "seconds") const; // 默认返回秒，可选返回毫秒
    std::string getStartTime() const;
    std::string getEndTime() const;

private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    bool isRunning;
    std::string formatTime(std::chrono::system_clock::time_point timePoint) const;
};

// 路径拼接
std::string path_join(const std::string& path1, const std::string& path2);


// 命令行模式-控制台窗口
void OpenConsoleWindow();
void CloseConsoleWindow();

// 获取网卡信息
std::vector<std::pair<std::string, std::string>> GetNetworkInterfaces();

// 绑定套接字到特定的网络接口
std::string BindSocketToInterface(SOCKET* sock, bool isIPV6, const std::string& ipAddress,int PORT);
