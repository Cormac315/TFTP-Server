#pragma once
#include <chrono>
#include <ctime>
#include <string>

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



