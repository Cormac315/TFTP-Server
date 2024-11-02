#pragma once
#include <string>
// 全局变量
extern std::string ServerRootDirectory; // 服务器根目录
extern int MaxThreadCount; // 允许处理的最大线程数
extern int currentThreadCount; // 当前线程数
extern int MAX_RETRIES; // 最大重传次数
extern bool running; // 服务器是否正在运行
extern int console_log_switch; // 控制台日志开关
extern std::string console_log_dir; // 控制台日志目录
extern int error_log_switch; // 服务器日志开关
extern std::string error_log_dir; // 服务器日志目录
extern bool is_console_mode; // 是否为控制台模式
extern int PORT; // 服务器端口
//extern int TIMEOUT; // 超时时间
extern std::vector<std::pair<std::string, std::string>> interfaces; // 网卡信息
extern int selectedInterfaceIdx; // 选择的网卡

extern void CloseConsoleWindow(); // 关闭控制台窗口


