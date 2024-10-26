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
        return duration.count(); // Ĭ�Ϸ�����
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

// ·��ƴ�Ӻ���
std::string path_join(const std::string& path1, const std::string& path2) {
    // ��� path1 �Է�б�ܽ�β��ֱ������
    if (!path1.empty() && path1.back() == '\\') {
        return path1 + path2;
    }
    // ��� path2 �Է�б�ܿ�ͷ��ȥ�� path1 �Ľ�β��б��
    if (!path2.empty() && path2.front() == '\\') {
        return path1 + path2;
    }
    // ����ʹ�÷�б����������·��
    return path1 + "\\" + path2;
}

// ������ģʽ����̨����
void OpenConsoleWindow() {
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    freopen("CON", "w", stdout);
    freopen("CON", "r", stdin);
}

void CloseConsoleWindow() {
    printf("�������ѹرգ������Թر�������ڡ�\n");
    FreeConsole();
}

