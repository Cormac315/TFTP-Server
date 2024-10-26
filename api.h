#pragma once
#include <chrono>
#include <ctime>
#include <string>

// ��ʱ��
class Timer {
public:
    Timer();
    void start();
    void stop();
	double getDuration(const std::string& unit = "seconds") const; // Ĭ�Ϸ����룬��ѡ���غ���
    std::string getStartTime() const;
    std::string getEndTime() const;

private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    bool isRunning;
    std::string formatTime(std::chrono::system_clock::time_point timePoint) const;
};

// ·��ƴ��
std::string path_join(const std::string& path1, const std::string& path2);


// ������ģʽ-����̨����
void OpenConsoleWindow();
void CloseConsoleWindow();



