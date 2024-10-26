#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <winsock2.h> // Windows�׽��ֿ�
#include <ws2tcpip.h> // Windows�׽��ֿ�
#include <chrono>  // ʱ���
#include <thread> // �߳̿�֧�ֲ�������
#include <mutex> // ������֧���̰߳�ȫ
#include <ctime> // ʱ���
#include <cstdio> // ��׼�������
#include <filesystem>
#include <unordered_map> // ��ϣ��洢�ͻ���TID
#include "shared.h"
#include "api.h"
#include "UI.h"


std::string ServerRootDirectory = "./File"; // ��������Ŀ¼
int MaxThreadCount = 10; // �����������߳���	
int currentThreadCount = 0; // ��ǰ�߳���
int MAX_RETRIES = 10; // ����ش�����
bool running = false; // �������Ƿ���������
int console_log_switch = 1; // ����̨��־����
std::string console_log_dir = "./log/"; // ����̨��־Ŀ¼
int error_log_switch = 1; // ��������־����
std::string error_log_dir = "./log/"; // ��������־Ŀ¼
int PORT = 69; // �������˿�


#define ASCII_ART "\
               _ooOoo_\n\
              o8888888o\n\
              88\" . \"88\n\
              (| -_- |)\n\
              O\\  =  /O\n\
           ____/`---'\\____\n\
        .'  \\\\|     |//  `.\n\
        /  \\\\|||  :  |||//  \\\n\
       /  _||||| -:- |||||-  \\\n\
       |   | \\\\\\  -  /// |   |\n\
       | \\_|  ''\\---/''  |   |\n\
       \\  .-\\__  `-`  ___/-. /\n\
     ___`. .'  /--.--\\  `. . __\n\
  .\"\" '<  `.___\\_<|>_/___.'  >'\"\".\n\
 | | :  `- \\`.;`\\ _ /`;.`/ - ` : | |\n\
 \\  \\ `-.   \\_ __\\ /__ _/   .-` /  /\n\
======`-.____`-.___\\_____/___.-`____.-'======\n\
                   `=---='\n\
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\
  ���汣��      ����ʺɽ     ��������\n\
**********************************************"

#pragma comment(lib, "Ws2_32.lib")  // Winsock Library

#define BUFFER_SIZE 516 // ��������С
#define TIMEOUT 5 // ��ʱʱ�䣨�룩

void commandThreadFunction(); // �����̺߳���
void closeConsoleWindow();


// TFTP������
enum OpCode {
	RRQ = 1, // ���ļ�����
	WRQ = 2,  // д�ļ�����
	DATA = 3, // ���ݰ�
	ACK = 4,   // ȷ�ϰ�
	ERR = 5  // ����
};



// TFTP����
class TFTPHeader {
public:
	uint16_t opcode; // ������
	uint16_t blockOrErrorCode; // ���ݿ�������
	char data[BUFFER_SIZE - 4]; // ����

	TFTPHeader(uint16_t op = 0, uint16_t blockOrError = 0) : opcode(op), blockOrErrorCode(blockOrError) {
		memset(data, 0, sizeof(data));
	}

	void setData(const std::string& str) {
		strncpy(data, str.c_str(), sizeof(data) - 1);
	}

	void send(SOCKET sock, const sockaddr_in& clientAddr) {
		opcode = htons(opcode);
		blockOrErrorCode = htons(blockOrErrorCode);
		sendto(sock, (char*)this, 4 + strlen(data) + 1, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
	}

	std::string getFilename() const {
		std::string result;
		// �� uint16_t ת��Ϊ���� ASCII �ַ����洢���ַ�����
		result += static_cast<char>((ntohs(blockOrErrorCode) >> 8) & 0xFF);  // ���ֽ�
		result += static_cast<char>(ntohs(blockOrErrorCode) & 0xFF);         // ���ֽ�
		return result + std::string(data);
	}

	std::string getMode() const {
		const char* mode = data + strlen(data) + 1;
		return std::string(mode);
	}
};

// TFTP������
class TFTPServer {
public:
	// ��ʼ��
	TFTPServer() {
		if (is_console_mode) {
			std::thread commandThread(commandThreadFunction); // ���������߳�
			commandThread.detach();
		}


		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			logMessage("WSA��ʼ��ʧ��", sockaddr_in{}, 1);
			return;
		}

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock == INVALID_SOCKET) {
			logMessage("�׽��ִ���ʧ��", sockaddr_in{}, 1);
			WSACleanup();
			return;
		}

		// ���ü����׽���Ϊ������ģʽ
		u_long mode = 1;
		if (ioctlsocket(sock, FIONBIO, &mode) != NO_ERROR) {
			logMessage("�����׽���Ϊ������ģʽʧ��", sockaddr_in{}, 1);
			closesocket(sock);
			WSACleanup();
			return;
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(PORT);
		serverAddr.sin_addr.s_addr = INADDR_ANY;

		if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
			logMessage("��ʧ�ܣ�����"+ std::to_string(PORT) + "�˿��Ƿ��ѱ�ռ��", sockaddr_in{}, 1);
			closesocket(sock);
			WSACleanup();
			return;
		}

		logMessage("TFTP����������,���ڼ���"+ std::to_string(PORT) +"�˿�...", sockaddr_in{}, 2);


		ServerRootDirectory = mainFrame->server_dir->GetPath().ToStdString();
		std::filesystem::path rootDir(ServerRootDirectory);

		// �Զ�������������Ŀ¼
		if (!std::filesystem::exists(rootDir)) {
			try {
				if (std::filesystem::create_directories(rootDir)) {
					logMessage("��������Ŀ¼�����ڣ��Զ������ɹ�: " + ServerRootDirectory, sockaddr_in{}, 2);
					
				}
				else {
					logMessage("��������Ŀ¼�����ڣ��Զ�����ʧ��: " + ServerRootDirectory, sockaddr_in{}, 1);
				}
			}
			catch (const std::filesystem::filesystem_error& e) {
				logMessage("�ļ�ϵͳ����: " + std::string(e.what()), sockaddr_in{}, 1);
			}
		}

		if (!std::filesystem::exists(console_log_dir))std::filesystem::create_directories(console_log_dir);
		if (!std::filesystem::exists(error_log_dir))std::filesystem::create_directories(error_log_dir);

	}

	~TFTPServer() {
		logMessage("�������ѹر�", sockaddr_in{}, 2);
		if (is_console_mode) {
			is_console_mode = false;
			running = false;
		}
		closesocket(sock);
		WSACleanup();
	}

	// �����׽�������ʱ
	void run() {
		while (running) {
			sockaddr_in clientAddr;
			int clientAddrLen = sizeof(clientAddr);
			char buffer[BUFFER_SIZE];
			int recvLen = recvfrom(sock, buffer, BUFFER_SIZE, 0, (sockaddr*)&clientAddr, &clientAddrLen);
			if (recvLen > 0) {
				std::cout << std::endl;

				TFTPHeader* request = (TFTPHeader*)buffer;
				switch (ntohs(request->opcode)) {
				case RRQ:
					std::thread(&TFTPServer::handleRRQ, this, clientAddr, request).detach();
					break;
				case WRQ:
					std::thread(&TFTPServer::handleWRQ, this, clientAddr, request).detach();
					break;
				case ERR:
					char ipBuffer[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &(clientAddr.sin_addr), ipBuffer, INET_ADDRSTRLEN);
					logMessage("�յ����Կͻ���:" + std::string(ipBuffer) + "�Ĵ����, �����룺" + std::to_string(ntohs(request->blockOrErrorCode)) + "\n������Ϣ��" + std::string(request->data), clientAddr, 1);
					break;
				default:
					sendError(clientAddr, 4, "�Ƿ���TFTP����");
					logMessage("�Ƿ���TFTP����", clientAddr, 1);
					break;
				}
			}
		}
	}

private:
	WSADATA wsaData;
	SOCKET sock;
	sockaddr_in serverAddr;

	// ��UI���������־
	void LogToUI(const std::string& message)
	{
		if (g_mainFrame != nullptr)
		{
			g_mainFrame->console_log->AppendText(message + "\n");
		}
	}

	// ��־��¼
	std::mutex logMutex; // ������
	void logMessage(const std::string& message, const sockaddr_in& clientAddr, int mode) {
		std::lock_guard<std::mutex> guard(logMutex); // ȷ����־��¼���̰߳�ȫ�ģ����û�����
		auto now = std::chrono::system_clock::now();
		std::time_t now_time = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm;
		localtime_s(&local_tm, &now_time);

		char timeBuffer[100];
		strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &local_tm);

		char ipBuffer[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), ipBuffer, INET_ADDRSTRLEN);
		std::ostringstream logStream;

		switch (mode) {
		case 0:
			logStream << "��" << timeBuffer << " | IP��" << ipBuffer <<"��" << message;
			break;
		case 1:
			logStream << "������!" << " (" << timeBuffer << " | IP��" << ipBuffer <<")��" << message;
			break;
		case 2:
			logStream << "��" << timeBuffer << "��" << message;
			break;
		}

		printf("%s\n", logStream.str().c_str());
		if(!is_console_mode)LogToUI(logStream.str());

		if (mode == 1 && error_log_switch == 0) return; // ������־���عر�
		else if (console_log_switch == 0) return; // ����̨��־���عر�
		std::ofstream logFile(mode == 1 ? error_log_dir + "\\error.log" : console_log_dir + "\\console.log", std::ios::app);
		if (logFile.is_open()) {
			logFile << logStream.str() << std::endl;
			logFile.close();
		}
	}

	// ���ʹ����
	void sendError(sockaddr_in clientAddr, uint16_t errorCode, const std::string& errorMessage) {
		TFTPHeader errorPacket(ERR, errorCode);
		errorPacket.setData(errorMessage);
		errorPacket.send(sock, clientAddr);
	}

	// ���������
	void handleRRQ(sockaddr_in clientAddr, TFTPHeader* request) {
		if (currentThreadCount + 1 >= MaxThreadCount) {
			sendError(clientAddr, 4, "Access violation");
			logMessage("��������æ���ܾ��ÿͻ��˵�����", clientAddr, 1);
			return;
		}
		currentThreadCount++;
		// ������ʱ�׽������ں���ͨ��
		SOCKET dataSock = socket(AF_INET, SOCK_DGRAM, 0);
		if (dataSock == INVALID_SOCKET) {
			logMessage("�޷��������ݴ����׽���", clientAddr, 1);
			currentThreadCount--;
			return;
		}

		// ���ô����׽���Ϊ������ģʽ
		u_long blockmode = 1;
		ioctlsocket(dataSock, FIONBIO, &blockmode);

		// �󶨴����׽��ֵ���̬����Ķ˿�
		sockaddr_in dataAddr = {};
		dataAddr.sin_family = AF_INET;
		dataAddr.sin_port = 0; // ��̬���䣬�˿�����Ϊ0
		dataAddr.sin_addr.s_addr = INADDR_ANY;
		if (bind(dataSock, (sockaddr*)&dataAddr, sizeof(dataAddr)) == SOCKET_ERROR) {
			logMessage("�����ݴ����׽���ʧ��", clientAddr, 1);
			closesocket(dataSock);
			currentThreadCount--;
			return;
		}

		// ��ȡ�����е��ļ�����ģʽ
		std::string filename = request->getFilename();
		std::string mode = request->getMode();
		logMessage("���ļ�����: " + filename + " ģʽ: " + mode, clientAddr, 0);

		// ��ȡ��̬����Ķ˿ںţ�������TID��������ô�������Ϊ�ͻ��˲�����
		//int dataAddrLen = sizeof(dataAddr); 
		//if (getsockname(dataSock, (sockaddr*)&dataAddr, &dataAddrLen) == SOCKET_ERROR) {
		//	std::cerr << "��ȡ�����׽��ֶ˿ں�ʧ��" << std::endl;
		//	closesocket(dataSock);
		//	return;
		//}
		//uint16_t serverTID = ntohs(dataAddr.sin_port); // ��ȡ��̬����Ķ˿ڣ�������TID��		
		//printf("(������Ϣ)�����TID: %d\n", serverTID);

		// �ļ�������
		std::string filePath = path_join(ServerRootDirectory, filename);
		std::ifstream file(filePath, mode == "netascii" ? std::ios::in : std::ios::binary);
		if (!file.is_open()) {
			sendError(clientAddr, 1, "File not found");
			logMessage("�ͻ��˳��Զ�ȡһ�������ڵ��ļ���" + filename, clientAddr, 1);
			closesocket(dataSock);  // �ر��׽���
			currentThreadCount--;
			return;
		}

		uint16_t clientTID = 0; // �ͻ���TID
		uint16_t blockNumber = 1; // ���ݿ���
		char buffer[BUFFER_SIZE]; // ������
		int bytesRead; // ��ȡ���ֽ���
		size_t totalBytesSent = 0;
		std::string s_throughput; // �������ַ���
		bool isLastBlock = false; // �Ƿ������һ�����ݿ�

		// �ܼ�ʱ�������ڼ�����������
		Timer timer; // ��ʼ����ʱ��
		timer.start(); // ��ʼ��ʱ
		std::string StartTime = timer.getStartTime(); // ��ȡ��ʼʱ��

		// ��ӿͻ�����Ϣ�����
		char clientIp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
		wxTheApp->CallAfter([clientIp, filename, StartTime]() {
			g_mainFrame->AddClientInfo(clientIp, "���أ�"+ filename, StartTime);
			});

		int row = g_mainFrame->currentRow;

		// �ļ����䣨ͨ����ʱ�׽��֣�
		while (!isLastBlock) {
			memset(buffer, 0, BUFFER_SIZE);
			file.read(buffer + 4, 512);
			bytesRead = static_cast<int>(file.gcount());
			if (bytesRead < 512) {
				isLastBlock = true;
			}

			TFTPHeader* dataPacket = (TFTPHeader*)buffer;
			dataPacket->opcode = htons(DATA);
			dataPacket->blockOrErrorCode = htons(blockNumber);

			int packetSize = 4 + bytesRead;
			int retries = 0;

			while (retries < MAX_RETRIES) {
				sendto(dataSock, buffer, packetSize, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
				totalBytesSent += bytesRead;

				// ʹ�� select() ���г�ʱ����
				fd_set readfds;
				FD_ZERO(&readfds);
				FD_SET(dataSock, &readfds);
				
				// ���ó�ʱʱ��
				struct timeval timeout;
				timeout.tv_sec = 0;
				timeout.tv_usec = 500000; // 500ms

				int selectResult = select(0, &readfds, NULL, NULL, &timeout);

				if (selectResult > 0) {
					// ����ACK
					sockaddr_in recvAddr;
					int recvAddrLen = sizeof(recvAddr);
					char ackBuffer[BUFFER_SIZE];
					int recvLen = recvfrom(dataSock, ackBuffer, BUFFER_SIZE, 0, (sockaddr*)&recvAddr, &recvAddrLen);

					if (recvLen > 0) {
						// ���㲢����������
						s_throughput = std::to_string(totalBytesSent / timer.getDuration() / 1024.0);
						wxTheApp->CallAfter([row, s_throughput]() {
							g_mainFrame->UpdateClientStatus(row, "", s_throughput);
							});

						// �ͻ���TID��֤
						uint16_t receivedClientTID = ntohs(recvAddr.sin_port);
						// printf("(������Ϣ)�յ���%d��ACK���ͻ���TID: %d\n", blockNumber, receivedClientTID);
						if (!clientTID) clientTID = receivedClientTID;
						else if (clientTID != receivedClientTID) {
							sendError(recvAddr, 5, "clientTID Error");
							logMessage("���Կͻ��˵�����Դ�˿�У��ʧ��,Client_TID:" + std::to_string(receivedClientTID) + ",Ԥ�ڵ�Client_TIDΪ��" + std::to_string(clientTID), clientAddr, 1);
							continue; // �ð������д�������������һ��ACK
						}

						// �����TID��֤
						//uint16_t receivedServerTID = ntohs(recvAddr.sin_port); // �����Ǵ���ģ�Ӧ���ǿͻ��˵Ķ˿ں�
						//if (receivedServerTID != serverTIDs[clientAddr.sin_addr.S_un.S_addr]) {
						//	sendError(clientAddr, 5, "serverTID Error");
						//	logMessage("���Կͻ��˵����ݰ�Ŀ�Ķ˿�У��ʧ��,Server_TID:" + std::to_string(receivedServerTID) + ",Ԥ�ڵ�Server_TIDΪ��" + std::to_string(serverTIDs[clientAddr.sin_addr.S_un.S_addr]), clientAddr, 1);
						//	continue; // �ð������д�������������һ��ACK
						//}

						TFTPHeader* ack = (TFTPHeader*)ackBuffer;
						if (ntohs(ack->opcode) == ACK && ntohs(ack->blockOrErrorCode) == blockNumber) {
							if (retries) {
								logMessage("���ش�����: " + std::to_string(retries), clientAddr, 1);
								retries = 0;
							}
							wxTheApp->CallAfter([row, blockNumber, retries]() {
								g_mainFrame->UpdateClientStatus(row, "�������ݰ���:" + std::to_string(blockNumber), "", retries);
								});
							blockNumber++;
							break; // �յ�ACK��,������һ�����ݰ�
						}
						else if (ntohs(ack->opcode) == ERR) {
							// �յ��ͻ��˱���
							file.close();
							closesocket(dataSock); // �رմ����׽���
							logMessage("�յ��ͻ��˴����,�����룺" + std::to_string(ntohs(ack->blockOrErrorCode)) + "\n������Ϣ��" + std::string(request->data), clientAddr, 1);
							wxTheApp->CallAfter([row]() {
								g_mainFrame->UpdateClientStatus(row, "�ͻ��˱�����鿴��־", "-");
								});
							wxTheApp->CallAfter([row]() {
								g_mainFrame->TableStatus(row, false);
								});
							currentThreadCount--;
							return;
						}
					}
				}
				else if (selectResult == 0) {
					if (retries == 0)logMessage("��" + std::to_string(blockNumber) + "�����ݰ���ACK���ճ�ʱ���������Է��͵�" + std::to_string(blockNumber) + "�����ݰ�....", clientAddr, 1);
					retries++;
					// ���±��
					wxTheApp->CallAfter([row, blockNumber, retries]() {
						g_mainFrame->UpdateClientStatus(row, "�ش�ACK:" + std::to_string(blockNumber), "", retries);
					});
				}
				else {
					logMessage("select() ����", clientAddr, 1);
					break;
				}
			}

			if (retries == MAX_RETRIES) {
				// �ﵽ������Դ���
				if (isLastBlock)break;
				file.close();
				sendError(clientAddr, 0, "Max Retries");
				closesocket(dataSock); // �رմ����׽���
				logMessage("�ﵽ������Դ���������ʧ��", clientAddr, 1);
				wxTheApp->CallAfter([row, retries]() {
					g_mainFrame->UpdateClientStatus(row, "�ͻ������س�ʱ", "-", retries);
					});
				wxTheApp->CallAfter([row]() {
					g_mainFrame->TableStatus(row, false);
					});
				currentThreadCount--;
				return;
			}
		}

		// ���ͳɹ�
		s_throughput = std::to_string(totalBytesSent / timer.getDuration() / 1024.0);  // ���㲢����������
		file.close();
		logMessage("�ļ��������: " + filename + "�����ݰ�������" + std::to_string(blockNumber) + "�����ֽ�����" + std::to_string(totalBytesSent) + "B����ʱ��" + std::to_string(timer.getDuration()) + "�룬��������" + s_throughput + "KB/S", clientAddr, 0);
		timer.stop(); // ֹͣ��ʱ
		closesocket(dataSock); // �رմ����׽���
		currentThreadCount--;
		// ���±��
		wxTheApp->CallAfter([row, s_throughput]() {
			g_mainFrame->UpdateClientStatus(row, "���سɹ�", s_throughput);
			});
		wxTheApp->CallAfter([row]() {
			g_mainFrame->TableStatus(row, true);
			});
	}

	// ����д����
	void handleWRQ(sockaddr_in clientAddr, TFTPHeader* request) {
		// ���������Ƿ�æ
		if (currentThreadCount + 1 >= MaxThreadCount) {
			sendError(clientAddr, 0, "Server Busy");
			logMessage("��������æ���ܾ�����", clientAddr, 1);
			return;
		}
		currentThreadCount++;
		// ���������׽������ں���ͨ��
		SOCKET dataSock = socket(AF_INET, SOCK_DGRAM, 0);
		if (dataSock == INVALID_SOCKET) {
			logMessage("�޷��������ݴ����׽���", clientAddr, 1);
			currentThreadCount--;
			return;
		}

		// ���ô����׽���Ϊ������ģʽ
		u_long blockmode = 1;
		ioctlsocket(dataSock, FIONBIO, &blockmode);

		// �󶨵���̬����Ķ˿�
		sockaddr_in dataAddr = {};
		dataAddr.sin_family = AF_INET;
		dataAddr.sin_port = 0; // ��̬����˿�
		dataAddr.sin_addr.s_addr = INADDR_ANY;
		if (bind(dataSock, (sockaddr*)&dataAddr, sizeof(dataAddr)) == SOCKET_ERROR) {
			logMessage("�����ݴ����׽���ʧ��", clientAddr, 1);
			closesocket(dataSock);
			currentThreadCount--;
			return;
		}

		// ��ȡ�����е��ļ�����ģʽ
		std::string filename = request->getFilename();
		std::string mode = request->getMode();
		logMessage("д�ļ�����: " + filename + " ģʽ: " + mode, clientAddr, 0);

		std::string filePath = path_join(ServerRootDirectory, filename);


		// ����ļ��Ƿ��Ѵ���
		std::ifstream existingFile(filePath);
		if (existingFile) {
			sendError(clientAddr, 6, "File already exists");
			logMessage("�ͻ��˳���д��һ���Ѵ��ڵ��ļ���" + filename, clientAddr, 1);
			closesocket(dataSock); // �رմ����׽���
			currentThreadCount--;
			return;
		}

		// �ļ�д����
		std::ofstream file(filePath, mode == "netascii" ? std::ios::out : std::ios::binary);
		if (!file.is_open()) {
			logMessage("�������޷������ļ�����ʹ�ù���ԱȨ������������", clientAddr, 1);
			sendError(clientAddr, 2, "Cannot create file");
			closesocket(dataSock); // �رմ����׽���
			currentThreadCount--;
			return;
		}

		uint16_t clientTID = 0; // �ͻ���TID
		uint16_t blockNumber = 0; // ���ݿ���
		char buffer[BUFFER_SIZE]; // ������	
		int retries = 0; // ���Դ���
		size_t totalBytesReceived = 0; // ���յ����ֽ���
		std::string s_throughput; // �������ַ���
		bool isLastBlock = false; // �Ƿ������һ�����ݿ�
		
		// �ܼ�ʱ�������ڼ�����������
		Timer timer; // ��ʼ����ʱ��
		timer.start(); // ��ʼ��ʱ
		std::string StartTime = timer.getStartTime(); // ��ȡ��ʼʱ��


		// ��ӿͻ�����Ϣ�����
		char clientIp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
		wxTheApp->CallAfter([clientIp,filename, StartTime]() {
			g_mainFrame->AddClientInfo(clientIp, "�ϴ���" + filename, StartTime);
			});
		int row = g_mainFrame->currentRow;


		while (!isLastBlock) {
			// ����ACK
			TFTPHeader ackPacket(ACK, blockNumber);
			ackPacket.send(dataSock, clientAddr);
			
			// ʹ�� select() ���г�ʱ����
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(dataSock, &readfds);

			struct timeval timeout;
			timeout.tv_sec = 0;  // ���ó�ʱʱ��Ϊ5��
			timeout.tv_usec = 200000; // 200ms

			int selectResult = select(0, &readfds, NULL, NULL, &timeout);

			if (selectResult > 0) {
				// �������ݰ�
				sockaddr_in recvAddr;
				int recvAddrLen = sizeof(recvAddr);
				int recvLen = recvfrom(dataSock, buffer, BUFFER_SIZE, 0, (sockaddr*)&recvAddr, &recvAddrLen);

				if (recvLen > 0) {
					// ���㲢����������
					s_throughput = std::to_string(totalBytesReceived / timer.getDuration() / 1024.0);
					wxTheApp->CallAfter([row, s_throughput]() {
						g_mainFrame->UpdateClientStatus(row, "", s_throughput);
						});

					// �ͻ���TID��֤
					uint16_t receivedClientTID = ntohs(recvAddr.sin_port);
					if (!clientTID) clientTID = receivedClientTID;
					else if (clientTID != receivedClientTID) {
						sendError(recvAddr, 5, "clientTID Error");
						logMessage("���Կͻ��˵�����Դ�˿�У��ʧ��,Client_TID:" + std::to_string(receivedClientTID) + ",Ԥ�ڵ�Client_TIDΪ��" + std::to_string(clientTID), clientAddr, 1);
						continue; // �ð������д�������������һ��ACK
					}


					TFTPHeader* dataPacket = (TFTPHeader*)buffer;
					if (ntohs(dataPacket->opcode) == DATA && ntohs(dataPacket->blockOrErrorCode) == blockNumber + 1) {
						if (retries)logMessage("���ش�����: " + std::to_string(retries), clientAddr, 1);
						retries = 0;
						wxTheApp->CallAfter([row, blockNumber, retries]() {
							g_mainFrame->UpdateClientStatus(row, "�������ݰ���:" + std::to_string(blockNumber + 1), "", retries);
							});
						file.write(dataPacket->data, recvLen - 4); // д���ļ�
						totalBytesReceived += recvLen - 4;
						blockNumber++;
						if (recvLen < 516) {
							isLastBlock = true;
						}
						continue; // �յ����ݰ���׼��������һ�����ݰ�
					}
					else if (ntohs(dataPacket->opcode) == ERR) {
						// �յ��ͻ��˱���
						file.close();
						remove(filePath.c_str()); // ɾ���ļ�
						closesocket(dataSock); // �ر���ʱ�׽���
						logMessage("�յ��ͻ��˴����,�����룺" + std::to_string(ntohs(dataPacket->blockOrErrorCode)) + "\n������Ϣ��" + std::string(request->data), clientAddr, 1);
						wxTheApp->CallAfter([row]() {
							g_mainFrame->UpdateClientStatus(row, "�ͻ��˱�����鿴��־", "-");
							});
						wxTheApp->CallAfter([row]() {
							g_mainFrame->TableStatus(row, false);
							});
						currentThreadCount--;
						return;
					}
					else if (retries < MAX_RETRIES) {
						// ACK�������ش�ACK
						if (retries == 0)logMessage("ȷ�ϵ�" + std::to_string(blockNumber) + "�����ݰ���ACKδ�ʹ�" + "���������Է���ȷ�ϵ�" + std::to_string(blockNumber) + "�����ݰ���ACK....", clientAddr, 1);
						retries++;
						// ���±��
						wxTheApp->CallAfter([row, blockNumber, retries]() {
							g_mainFrame->UpdateClientStatus(row, "�ش�ACK:" + std::to_string(blockNumber), "", retries);
							});
						continue;
					}
				}
			}
			else if (selectResult == 0) {
			    if (retries < MAX_RETRIES) {
					// ��һ�����ݰ��������ش�ACK		
					if (retries == 0)logMessage("��" + std::to_string(blockNumber + 1) + "�����ݰ���ʧ���������Է���ȷ�ϵ�" + std::to_string(blockNumber) + "�����ݰ���ACK....", clientAddr, 1);
					retries++;
					wxTheApp->CallAfter([row, blockNumber, retries]() {
						g_mainFrame->UpdateClientStatus(row, "�ش����ݰ���" + std::to_string(blockNumber + 1), "", retries);
						});
					continue;
			    }
				else {
					// �ﵽ������Դ���
					file.close();
					remove(filePath.c_str()); // ɾ���ļ�
					sendError(clientAddr, 0, "Max Retries");
					closesocket(dataSock); // �ر���ʱ�׽���
					logMessage("�ﵽ������Դ���������ʧ��", clientAddr, 1);
					wxTheApp->CallAfter([row, retries]() {
						g_mainFrame->UpdateClientStatus(row, "�ͻ����ϴ���ʱ", "-", retries);
						});
					wxTheApp->CallAfter([row]() {
						g_mainFrame->TableStatus(row, false);
						});
					currentThreadCount--;
					return;
				}
			}
		}
		timer.stop(); // ֹͣ��ʱ
		// �ȴ����һ��ACK���ͻ��˽���
		int recvLen, recvAddrLen;
		do {
			// ����ACK
			TFTPHeader ackPacket(ACK, blockNumber);
			ackPacket.send(dataSock, clientAddr);

			// �������ݰ�
			sockaddr_in recvAddr;
			recvAddrLen = sizeof(recvAddr);
			recvLen = recvfrom(dataSock, buffer, BUFFER_SIZE, 0, (sockaddr*)&recvAddr, &recvAddrLen);
		} while (recvLen > 0);


		// ���ͳɹ�
		s_throughput = std::to_string(totalBytesReceived / timer.getDuration() / 1024.0); // ���㲢����������
		file.close();
		closesocket(dataSock); // �ر���ʱ�׽���
		logMessage("�ļ�д�����: " + filename + "�����ݰ�������" + std::to_string(blockNumber) + "�����ֽ�����" + std::to_string(totalBytesReceived) + "����ʱ��" + std::to_string(timer.getDuration()) + "�룬��������" + s_throughput + "KB/S", clientAddr, 0);
		timer.stop(); // ֹͣ��ʱ
		currentThreadCount--;
		wxTheApp->CallAfter([row, s_throughput]() {
			g_mainFrame->UpdateClientStatus(row, "�ϴ��ɹ�", s_throughput);
		});
		wxTheApp->CallAfter([row]() {
			g_mainFrame->TableStatus(row, true);
			});
	}

};

// ������ģʽ�µ������߳�
void commandThreadFunction() {
	std::string command;
	running = true;
	printf("*********************************************\nTFTP���������������ʺɽ�� Version 1.0\n@Copyright by Cormac Created on 2024 Nov 29\n");
	printf(ASCII_ART);
	printf("\n��ϵͳ��ʾ���벻Ҫֱ�ӹر������д��ڣ������˳���������������/stop\n");
	Sleep(500);
	while (running) {
		printf(">> root@TFTPServer # ");
		std::getline(std::cin, command);
		if (command == "/help") {
			printf("------------------------------------------------------\n");
			printf("���������Ϣ��\n");
			printf("/help - ��ʾ���������Ϣ\n");
			printf("/set MaxThreadCount [count] - ��������߳�����\n");
			printf("/set ServerRootDirectory [directory] - ���÷�������Ŀ¼\n");
			printf("/set MAXRETRIES [retries] - ��������ش�����\n");
			printf("/stop - ֹͣTFTP������\n");
			printf("------------------------------------------------------\n");
		}
		else if (command.find("/set MaxThreadCount") == 0) {
			std::string countStr = command.substr(20);
			int count = std::stoi(countStr);
			MaxThreadCount = count;
			printf("����߳���������Ϊ��%d\n", count);
		}
		else if (command.find("/set ServerRootDirectory") == 0) {
			std::string directory = command.substr(25);
			ServerRootDirectory = directory;
			printf("��������Ŀ¼����Ϊ��%s\n", directory.c_str());
		}
		else if (command.find("/set MAXRETRIES") == 0) {
			std::string retriesStr = command.substr(15);
			int retries = std::stoi(retriesStr);
			MAX_RETRIES = retries;
			printf("����ش���������Ϊ��%d\n", retries);
		}
		else if (command == "/stop") {
			printf("����ֹͣTFTP������...\n");
			Sleep(1000);
			running = false;
			is_console_mode = false;
			CloseConsoleWindow();
			mainFrame->Server_Switch->SetSelection(1);
			break;
		}
		else {
			printf("δ֪���������/help��ȡ������Ϣ\n");
		}
	}
}

// �������߳�
void StartServer() {
	TFTPServer server;
	std::thread serverThread(&TFTPServer::run, &server); // �����������߳�
	serverThread.join();
	return;
}
