#define _CRT_SECURE_NO_WARNINGS
#include "api.h"
#include <iomanip>
#include <sstream>
#include <Windows.h>

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

