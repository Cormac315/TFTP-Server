#pragma once
#include <string>
// ȫ�ֱ���
extern std::string ServerRootDirectory; // ��������Ŀ¼
extern int MaxThreadCount; // �����������߳���
extern int currentThreadCount; // ��ǰ�߳���
extern int MAX_RETRIES; // ����ش�����
extern bool running; // �������Ƿ���������
extern int console_log_switch; // ����̨��־����
extern std::string console_log_dir; // ����̨��־Ŀ¼
extern int error_log_switch; // ��������־����
extern std::string error_log_dir; // ��������־Ŀ¼
extern bool is_console_mode; // �Ƿ�Ϊ����̨ģʽ
extern int PORT; // �������˿�
//extern int TIMEOUT; // ��ʱʱ��
extern std::vector<std::pair<std::string, std::string>> interfaces; // ������Ϣ
extern int selectedInterfaceIdx; // ѡ�������

extern void CloseConsoleWindow(); // �رտ���̨����


