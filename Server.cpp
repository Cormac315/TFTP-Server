#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <winsock2.h> // Windows套接字库
#include <ws2tcpip.h> // Windows套接字库
#include <chrono>  // 时间库
#include <thread> // 线程库支持并发请求
#include <mutex> // 互斥锁支持线程安全
#include <ctime> // 时间库
#include <cstdio> // 标准输入输出
#include <filesystem>
#include <unordered_map> // 哈希表存储客户端TID
#include "shared.h"
#include "api.h"
#include "UI.h"


std::string ServerRootDirectory = "./File"; // 服务器根目录
int MaxThreadCount = 10; // 允许处理的最大线程数	
int currentThreadCount = 0; // 当前线程数
int MAX_RETRIES = 10; // 最大重传次数
bool running = false; // 服务器是否正在运行
int console_log_switch = 1; // 控制台日志开关
std::string console_log_dir = "./log/"; // 控制台日志目录
int error_log_switch = 1; // 服务器日志开关
std::string error_log_dir = "./log/"; // 服务器日志目录
int PORT = 69; // 服务器端口


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
  佛祖保佑      虽是屎山     但是能跑\n\
**********************************************"

#pragma comment(lib, "Ws2_32.lib")  // Winsock Library

#define BUFFER_SIZE 516 // 缓冲区大小
#define TIMEOUT 5 // 超时时间（秒）

void commandThreadFunction(); // 命令线程函数
void closeConsoleWindow();


// TFTP操作码
enum OpCode {
	RRQ = 1, // 读文件请求
	WRQ = 2,  // 写文件请求
	DATA = 3, // 数据包
	ACK = 4,   // 确认包
	ERR = 5  // 错误
};



// TFTP报文
class TFTPHeader {
public:
	uint16_t opcode; // 操作码
	uint16_t blockOrErrorCode; // 数据块或错误码
	char data[BUFFER_SIZE - 4]; // 数据

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
		// 将 uint16_t 转换为两个 ASCII 字符并存储在字符串中
		result += static_cast<char>((ntohs(blockOrErrorCode) >> 8) & 0xFF);  // 高字节
		result += static_cast<char>(ntohs(blockOrErrorCode) & 0xFF);         // 低字节
		return result + std::string(data);
	}

	std::string getMode() const {
		const char* mode = data + strlen(data) + 1;
		return std::string(mode);
	}
};

// TFTP服务器
class TFTPServer {
public:
	// 初始化
	TFTPServer() {
		if (is_console_mode) {
			std::thread commandThread(commandThreadFunction); // 创建命令线程
			commandThread.detach();
		}


		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			logMessage("WSA初始化失败", sockaddr_in{}, 1);
			return;
		}

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock == INVALID_SOCKET) {
			logMessage("套接字创建失败", sockaddr_in{}, 1);
			WSACleanup();
			return;
		}

		// 设置监听套接字为非阻塞模式
		u_long mode = 1;
		if (ioctlsocket(sock, FIONBIO, &mode) != NO_ERROR) {
			logMessage("设置套接字为非阻塞模式失败", sockaddr_in{}, 1);
			closesocket(sock);
			WSACleanup();
			return;
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(PORT);
		serverAddr.sin_addr.s_addr = INADDR_ANY;

		if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
			logMessage("绑定失败，请检查"+ std::to_string(PORT) + "端口是否已被占用", sockaddr_in{}, 1);
			closesocket(sock);
			WSACleanup();
			return;
		}

		logMessage("TFTP服务器启动,正在监听"+ std::to_string(PORT) +"端口...", sockaddr_in{}, 2);


		ServerRootDirectory = mainFrame->server_dir->GetPath().ToStdString();
		std::filesystem::path rootDir(ServerRootDirectory);

		// 自动创建服务器根目录
		if (!std::filesystem::exists(rootDir)) {
			try {
				if (std::filesystem::create_directories(rootDir)) {
					logMessage("服务器根目录不存在，自动创建成功: " + ServerRootDirectory, sockaddr_in{}, 2);
					
				}
				else {
					logMessage("服务器根目录不存在，自动创建失败: " + ServerRootDirectory, sockaddr_in{}, 1);
				}
			}
			catch (const std::filesystem::filesystem_error& e) {
				logMessage("文件系统错误: " + std::string(e.what()), sockaddr_in{}, 1);
			}
		}

		if (!std::filesystem::exists(console_log_dir))std::filesystem::create_directories(console_log_dir);
		if (!std::filesystem::exists(error_log_dir))std::filesystem::create_directories(error_log_dir);

	}

	~TFTPServer() {
		logMessage("服务器已关闭", sockaddr_in{}, 2);
		if (is_console_mode) {
			is_console_mode = false;
			running = false;
		}
		closesocket(sock);
		WSACleanup();
	}

	// 监听套接字运行时
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
					logMessage("收到来自客户端:" + std::string(ipBuffer) + "的错误包, 错误码：" + std::to_string(ntohs(request->blockOrErrorCode)) + "\n错误信息：" + std::string(request->data), clientAddr, 1);
					break;
				default:
					sendError(clientAddr, 4, "非法的TFTP操作");
					logMessage("非法的TFTP操作", clientAddr, 1);
					break;
				}
			}
		}
	}

private:
	WSADATA wsaData;
	SOCKET sock;
	sockaddr_in serverAddr;

	// 向UI界面输出日志
	void LogToUI(const std::string& message)
	{
		if (g_mainFrame != nullptr)
		{
			g_mainFrame->console_log->AppendText(message + "\n");
		}
	}

	// 日志记录
	std::mutex logMutex; // 互斥锁
	void logMessage(const std::string& message, const sockaddr_in& clientAddr, int mode) {
		std::lock_guard<std::mutex> guard(logMutex); // 确保日志记录是线程安全的，采用互斥锁
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
			logStream << "【" << timeBuffer << " | IP：" << ipBuffer <<"】" << message;
			break;
		case 1:
			logStream << "【警告!" << " (" << timeBuffer << " | IP：" << ipBuffer <<")】" << message;
			break;
		case 2:
			logStream << "【" << timeBuffer << "】" << message;
			break;
		}

		printf("%s\n", logStream.str().c_str());
		if(!is_console_mode)LogToUI(logStream.str());

		if (mode == 1 && error_log_switch == 0) return; // 错误日志开关关闭
		else if (console_log_switch == 0) return; // 控制台日志开关关闭
		std::ofstream logFile(mode == 1 ? error_log_dir + "\\error.log" : console_log_dir + "\\console.log", std::ios::app);
		if (logFile.is_open()) {
			logFile << logStream.str() << std::endl;
			logFile.close();
		}
	}

	// 发送错误包
	void sendError(sockaddr_in clientAddr, uint16_t errorCode, const std::string& errorMessage) {
		TFTPHeader errorPacket(ERR, errorCode);
		errorPacket.setData(errorMessage);
		errorPacket.send(sock, clientAddr);
	}

	// 处理读请求
	void handleRRQ(sockaddr_in clientAddr, TFTPHeader* request) {
		if (currentThreadCount + 1 >= MaxThreadCount) {
			sendError(clientAddr, 4, "Access violation");
			logMessage("服务器繁忙，拒绝该客户端的请求", clientAddr, 1);
			return;
		}
		currentThreadCount++;
		// 创建临时套接字用于后续通信
		SOCKET dataSock = socket(AF_INET, SOCK_DGRAM, 0);
		if (dataSock == INVALID_SOCKET) {
			logMessage("无法创建数据传输套接字", clientAddr, 1);
			currentThreadCount--;
			return;
		}

		// 设置传输套接字为非阻塞模式
		u_long blockmode = 1;
		ioctlsocket(dataSock, FIONBIO, &blockmode);

		// 绑定传输套接字到动态分配的端口
		sockaddr_in dataAddr = {};
		dataAddr.sin_family = AF_INET;
		dataAddr.sin_port = 0; // 动态分配，端口设置为0
		dataAddr.sin_addr.s_addr = INADDR_ANY;
		if (bind(dataSock, (sockaddr*)&dataAddr, sizeof(dataAddr)) == SOCKET_ERROR) {
			logMessage("绑定数据传输套接字失败", clientAddr, 1);
			closesocket(dataSock);
			currentThreadCount--;
			return;
		}

		// 获取请求中的文件名和模式
		std::string filename = request->getFilename();
		std::string mode = request->getMode();
		logMessage("读文件请求: " + filename + " 模式: " + mode, clientAddr, 0);

		// 获取动态分配的端口号（服务器TID），这个用处不大，因为客户端不会检查
		//int dataAddrLen = sizeof(dataAddr); 
		//if (getsockname(dataSock, (sockaddr*)&dataAddr, &dataAddrLen) == SOCKET_ERROR) {
		//	std::cerr << "获取数据套接字端口号失败" << std::endl;
		//	closesocket(dataSock);
		//	return;
		//}
		//uint16_t serverTID = ntohs(dataAddr.sin_port); // 获取动态分配的端口（服务器TID）		
		//printf("(调试信息)服务端TID: %d\n", serverTID);

		// 文件读操作
		std::string filePath = path_join(ServerRootDirectory, filename);
		std::ifstream file(filePath, mode == "netascii" ? std::ios::in : std::ios::binary);
		if (!file.is_open()) {
			sendError(clientAddr, 1, "File not found");
			logMessage("客户端尝试读取一个不存在的文件：" + filename, clientAddr, 1);
			closesocket(dataSock);  // 关闭套接字
			currentThreadCount--;
			return;
		}

		uint16_t clientTID = 0; // 客户端TID
		uint16_t blockNumber = 1; // 数据块编号
		char buffer[BUFFER_SIZE]; // 缓冲区
		int bytesRead; // 读取的字节数
		size_t totalBytesSent = 0;
		std::string s_throughput; // 吞吐量字符串
		bool isLastBlock = false; // 是否是最后一个数据块

		// 总计时器（用于计算吞吐量）
		Timer timer; // 初始化计时器
		timer.start(); // 开始计时
		std::string StartTime = timer.getStartTime(); // 获取开始时间

		// 添加客户端信息到表格
		char clientIp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
		wxTheApp->CallAfter([clientIp, filename, StartTime]() {
			g_mainFrame->AddClientInfo(clientIp, "下载："+ filename, StartTime);
			});

		int row = g_mainFrame->currentRow;

		// 文件传输（通过临时套接字）
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

				// 使用 select() 进行超时管理
				fd_set readfds;
				FD_ZERO(&readfds);
				FD_SET(dataSock, &readfds);
				
				// 设置超时时间
				struct timeval timeout;
				timeout.tv_sec = 0;
				timeout.tv_usec = 500000; // 500ms

				int selectResult = select(0, &readfds, NULL, NULL, &timeout);

				if (selectResult > 0) {
					// 接收ACK
					sockaddr_in recvAddr;
					int recvAddrLen = sizeof(recvAddr);
					char ackBuffer[BUFFER_SIZE];
					int recvLen = recvfrom(dataSock, ackBuffer, BUFFER_SIZE, 0, (sockaddr*)&recvAddr, &recvAddrLen);

					if (recvLen > 0) {
						// 计算并更新吞吐量
						s_throughput = std::to_string(totalBytesSent / timer.getDuration() / 1024.0);
						wxTheApp->CallAfter([row, s_throughput]() {
							g_mainFrame->UpdateClientStatus(row, "", s_throughput);
							});

						// 客户端TID认证
						uint16_t receivedClientTID = ntohs(recvAddr.sin_port);
						// printf("(调试信息)收到第%d个ACK，客户端TID: %d\n", blockNumber, receivedClientTID);
						if (!clientTID) clientTID = receivedClientTID;
						else if (clientTID != receivedClientTID) {
							sendError(recvAddr, 5, "clientTID Error");
							logMessage("来自客户端的数据源端口校验失败,Client_TID:" + std::to_string(receivedClientTID) + ",预期的Client_TID为：" + std::to_string(clientTID), clientAddr, 1);
							continue; // 该包不进行处理，继续接收下一个ACK
						}

						// 服务端TID认证
						//uint16_t receivedServerTID = ntohs(recvAddr.sin_port); // 这行是错误的，应该是客户端的端口号
						//if (receivedServerTID != serverTIDs[clientAddr.sin_addr.S_un.S_addr]) {
						//	sendError(clientAddr, 5, "serverTID Error");
						//	logMessage("来自客户端的数据包目的端口校验失败,Server_TID:" + std::to_string(receivedServerTID) + ",预期的Server_TID为：" + std::to_string(serverTIDs[clientAddr.sin_addr.S_un.S_addr]), clientAddr, 1);
						//	continue; // 该包不进行处理，继续接收下一个ACK
						//}

						TFTPHeader* ack = (TFTPHeader*)ackBuffer;
						if (ntohs(ack->opcode) == ACK && ntohs(ack->blockOrErrorCode) == blockNumber) {
							if (retries) {
								logMessage("总重传次数: " + std::to_string(retries), clientAddr, 1);
								retries = 0;
							}
							wxTheApp->CallAfter([row, blockNumber, retries]() {
								g_mainFrame->UpdateClientStatus(row, "发送数据包数:" + std::to_string(blockNumber), "", retries);
								});
							blockNumber++;
							break; // 收到ACK包,发送下一个数据包
						}
						else if (ntohs(ack->opcode) == ERR) {
							// 收到客户端报错
							file.close();
							closesocket(dataSock); // 关闭传输套接字
							logMessage("收到客户端错误包,错误码：" + std::to_string(ntohs(ack->blockOrErrorCode)) + "\n错误信息：" + std::string(request->data), clientAddr, 1);
							wxTheApp->CallAfter([row]() {
								g_mainFrame->UpdateClientStatus(row, "客户端报错，请查看日志", "-");
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
					if (retries == 0)logMessage("第" + std::to_string(blockNumber) + "个数据包的ACK接收超时，正在重试发送第" + std::to_string(blockNumber) + "个数据包....", clientAddr, 1);
					retries++;
					// 更新表格
					wxTheApp->CallAfter([row, blockNumber, retries]() {
						g_mainFrame->UpdateClientStatus(row, "重传ACK:" + std::to_string(blockNumber), "", retries);
					});
				}
				else {
					logMessage("select() 出错", clientAddr, 1);
					break;
				}
			}

			if (retries == MAX_RETRIES) {
				// 达到最大重试次数
				if (isLastBlock)break;
				file.close();
				sendError(clientAddr, 0, "Max Retries");
				closesocket(dataSock); // 关闭传输套接字
				logMessage("达到最大重试次数，传输失败", clientAddr, 1);
				wxTheApp->CallAfter([row, retries]() {
					g_mainFrame->UpdateClientStatus(row, "客户端下载超时", "-", retries);
					});
				wxTheApp->CallAfter([row]() {
					g_mainFrame->TableStatus(row, false);
					});
				currentThreadCount--;
				return;
			}
		}

		// 发送成功
		s_throughput = std::to_string(totalBytesSent / timer.getDuration() / 1024.0);  // 计算并更新吞吐量
		file.close();
		logMessage("文件发送完成: " + filename + "，数据包个数：" + std::to_string(blockNumber) + "，总字节数：" + std::to_string(totalBytesSent) + "B，耗时：" + std::to_string(timer.getDuration()) + "秒，吞吐量：" + s_throughput + "KB/S", clientAddr, 0);
		timer.stop(); // 停止计时
		closesocket(dataSock); // 关闭传输套接字
		currentThreadCount--;
		// 更新表格
		wxTheApp->CallAfter([row, s_throughput]() {
			g_mainFrame->UpdateClientStatus(row, "下载成功", s_throughput);
			});
		wxTheApp->CallAfter([row]() {
			g_mainFrame->TableStatus(row, true);
			});
	}

	// 处理写请求
	void handleWRQ(sockaddr_in clientAddr, TFTPHeader* request) {
		// 检查服务器是否繁忙
		if (currentThreadCount + 1 >= MaxThreadCount) {
			sendError(clientAddr, 0, "Server Busy");
			logMessage("服务器繁忙，拒绝服务", clientAddr, 1);
			return;
		}
		currentThreadCount++;
		// 创建传输套接字用于后续通信
		SOCKET dataSock = socket(AF_INET, SOCK_DGRAM, 0);
		if (dataSock == INVALID_SOCKET) {
			logMessage("无法创建数据传输套接字", clientAddr, 1);
			currentThreadCount--;
			return;
		}

		// 设置传输套接字为非阻塞模式
		u_long blockmode = 1;
		ioctlsocket(dataSock, FIONBIO, &blockmode);

		// 绑定到动态分配的端口
		sockaddr_in dataAddr = {};
		dataAddr.sin_family = AF_INET;
		dataAddr.sin_port = 0; // 动态分配端口
		dataAddr.sin_addr.s_addr = INADDR_ANY;
		if (bind(dataSock, (sockaddr*)&dataAddr, sizeof(dataAddr)) == SOCKET_ERROR) {
			logMessage("绑定数据传输套接字失败", clientAddr, 1);
			closesocket(dataSock);
			currentThreadCount--;
			return;
		}

		// 获取请求中的文件名和模式
		std::string filename = request->getFilename();
		std::string mode = request->getMode();
		logMessage("写文件请求: " + filename + " 模式: " + mode, clientAddr, 0);

		std::string filePath = path_join(ServerRootDirectory, filename);


		// 检查文件是否已存在
		std::ifstream existingFile(filePath);
		if (existingFile) {
			sendError(clientAddr, 6, "File already exists");
			logMessage("客户端尝试写入一个已存在的文件：" + filename, clientAddr, 1);
			closesocket(dataSock); // 关闭传输套接字
			currentThreadCount--;
			return;
		}

		// 文件写操作
		std::ofstream file(filePath, mode == "netascii" ? std::ios::out : std::ios::binary);
		if (!file.is_open()) {
			logMessage("服务器无法创建文件，请使用管理员权限启动服务器", clientAddr, 1);
			sendError(clientAddr, 2, "Cannot create file");
			closesocket(dataSock); // 关闭传输套接字
			currentThreadCount--;
			return;
		}

		uint16_t clientTID = 0; // 客户端TID
		uint16_t blockNumber = 0; // 数据块编号
		char buffer[BUFFER_SIZE]; // 缓冲区	
		int retries = 0; // 重试次数
		size_t totalBytesReceived = 0; // 接收的总字节数
		std::string s_throughput; // 吞吐量字符串
		bool isLastBlock = false; // 是否是最后一个数据块
		
		// 总计时器（用于计算吞吐量）
		Timer timer; // 初始化计时器
		timer.start(); // 开始计时
		std::string StartTime = timer.getStartTime(); // 获取开始时间


		// 添加客户端信息到表格
		char clientIp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
		wxTheApp->CallAfter([clientIp,filename, StartTime]() {
			g_mainFrame->AddClientInfo(clientIp, "上传：" + filename, StartTime);
			});
		int row = g_mainFrame->currentRow;


		while (!isLastBlock) {
			// 发送ACK
			TFTPHeader ackPacket(ACK, blockNumber);
			ackPacket.send(dataSock, clientAddr);
			
			// 使用 select() 进行超时管理
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(dataSock, &readfds);

			struct timeval timeout;
			timeout.tv_sec = 0;  // 设置超时时间为5秒
			timeout.tv_usec = 200000; // 200ms

			int selectResult = select(0, &readfds, NULL, NULL, &timeout);

			if (selectResult > 0) {
				// 接收数据包
				sockaddr_in recvAddr;
				int recvAddrLen = sizeof(recvAddr);
				int recvLen = recvfrom(dataSock, buffer, BUFFER_SIZE, 0, (sockaddr*)&recvAddr, &recvAddrLen);

				if (recvLen > 0) {
					// 计算并更新吞吐量
					s_throughput = std::to_string(totalBytesReceived / timer.getDuration() / 1024.0);
					wxTheApp->CallAfter([row, s_throughput]() {
						g_mainFrame->UpdateClientStatus(row, "", s_throughput);
						});

					// 客户端TID认证
					uint16_t receivedClientTID = ntohs(recvAddr.sin_port);
					if (!clientTID) clientTID = receivedClientTID;
					else if (clientTID != receivedClientTID) {
						sendError(recvAddr, 5, "clientTID Error");
						logMessage("来自客户端的数据源端口校验失败,Client_TID:" + std::to_string(receivedClientTID) + ",预期的Client_TID为：" + std::to_string(clientTID), clientAddr, 1);
						continue; // 该包不进行处理，继续接收下一个ACK
					}


					TFTPHeader* dataPacket = (TFTPHeader*)buffer;
					if (ntohs(dataPacket->opcode) == DATA && ntohs(dataPacket->blockOrErrorCode) == blockNumber + 1) {
						if (retries)logMessage("总重传次数: " + std::to_string(retries), clientAddr, 1);
						retries = 0;
						wxTheApp->CallAfter([row, blockNumber, retries]() {
							g_mainFrame->UpdateClientStatus(row, "接收数据包数:" + std::to_string(blockNumber + 1), "", retries);
							});
						file.write(dataPacket->data, recvLen - 4); // 写入文件
						totalBytesReceived += recvLen - 4;
						blockNumber++;
						if (recvLen < 516) {
							isLastBlock = true;
						}
						continue; // 收到数据包，准备接收下一个数据包
					}
					else if (ntohs(dataPacket->opcode) == ERR) {
						// 收到客户端报错
						file.close();
						remove(filePath.c_str()); // 删除文件
						closesocket(dataSock); // 关闭临时套接字
						logMessage("收到客户端错误包,错误码：" + std::to_string(ntohs(dataPacket->blockOrErrorCode)) + "\n错误信息：" + std::string(request->data), clientAddr, 1);
						wxTheApp->CallAfter([row]() {
							g_mainFrame->UpdateClientStatus(row, "客户端报错，请查看日志", "-");
							});
						wxTheApp->CallAfter([row]() {
							g_mainFrame->TableStatus(row, false);
							});
						currentThreadCount--;
						return;
					}
					else if (retries < MAX_RETRIES) {
						// ACK丢包，重传ACK
						if (retries == 0)logMessage("确认第" + std::to_string(blockNumber) + "个数据包的ACK未送达" + "，正在重试发送确认第" + std::to_string(blockNumber) + "个数据包的ACK....", clientAddr, 1);
						retries++;
						// 更新表格
						wxTheApp->CallAfter([row, blockNumber, retries]() {
							g_mainFrame->UpdateClientStatus(row, "重传ACK:" + std::to_string(blockNumber), "", retries);
							});
						continue;
					}
				}
			}
			else if (selectResult == 0) {
			    if (retries < MAX_RETRIES) {
					// 下一个数据包丢包，重传ACK		
					if (retries == 0)logMessage("第" + std::to_string(blockNumber + 1) + "个数据包丢失，正在重试发送确认第" + std::to_string(blockNumber) + "个数据包的ACK....", clientAddr, 1);
					retries++;
					wxTheApp->CallAfter([row, blockNumber, retries]() {
						g_mainFrame->UpdateClientStatus(row, "重传数据包：" + std::to_string(blockNumber + 1), "", retries);
						});
					continue;
			    }
				else {
					// 达到最大重试次数
					file.close();
					remove(filePath.c_str()); // 删除文件
					sendError(clientAddr, 0, "Max Retries");
					closesocket(dataSock); // 关闭临时套接字
					logMessage("达到最大重试次数，传输失败", clientAddr, 1);
					wxTheApp->CallAfter([row, retries]() {
						g_mainFrame->UpdateClientStatus(row, "客户端上传超时", "-", retries);
						});
					wxTheApp->CallAfter([row]() {
						g_mainFrame->TableStatus(row, false);
						});
					currentThreadCount--;
					return;
				}
			}
		}
		timer.stop(); // 停止计时
		// 等待最后一个ACK被客户端接收
		int recvLen, recvAddrLen;
		do {
			// 发送ACK
			TFTPHeader ackPacket(ACK, blockNumber);
			ackPacket.send(dataSock, clientAddr);

			// 接收数据包
			sockaddr_in recvAddr;
			recvAddrLen = sizeof(recvAddr);
			recvLen = recvfrom(dataSock, buffer, BUFFER_SIZE, 0, (sockaddr*)&recvAddr, &recvAddrLen);
		} while (recvLen > 0);


		// 发送成功
		s_throughput = std::to_string(totalBytesReceived / timer.getDuration() / 1024.0); // 计算并更新吞吐量
		file.close();
		closesocket(dataSock); // 关闭临时套接字
		logMessage("文件写入完成: " + filename + "，数据包个数：" + std::to_string(blockNumber) + "，总字节数：" + std::to_string(totalBytesReceived) + "，耗时：" + std::to_string(timer.getDuration()) + "秒，吞吐量：" + s_throughput + "KB/S", clientAddr, 0);
		timer.stop(); // 停止计时
		currentThreadCount--;
		wxTheApp->CallAfter([row, s_throughput]() {
			g_mainFrame->UpdateClientStatus(row, "上传成功", s_throughput);
		});
		wxTheApp->CallAfter([row]() {
			g_mainFrame->TableStatus(row, true);
			});
	}

};

// 命令行模式下的命令线程
void commandThreadFunction() {
	std::string command;
	running = true;
	printf("*********************************************\nTFTP服务器：面向对象屎山版 Version 1.0\n@Copyright by Cormac Created on 2024 Nov 29\n");
	printf(ASCII_ART);
	printf("\n【系统提示】请不要直接关闭命令行窗口，若需退出服务器，请输入/stop\n");
	Sleep(500);
	while (running) {
		printf(">> root@TFTPServer # ");
		std::getline(std::cin, command);
		if (command == "/help") {
			printf("------------------------------------------------------\n");
			printf("命令帮助信息：\n");
			printf("/help - 显示命令帮助信息\n");
			printf("/set MaxThreadCount [count] - 设置最大线程数量\n");
			printf("/set ServerRootDirectory [directory] - 设置服务器根目录\n");
			printf("/set MAXRETRIES [retries] - 设置最大重传次数\n");
			printf("/stop - 停止TFTP服务器\n");
			printf("------------------------------------------------------\n");
		}
		else if (command.find("/set MaxThreadCount") == 0) {
			std::string countStr = command.substr(20);
			int count = std::stoi(countStr);
			MaxThreadCount = count;
			printf("最大线程数量设置为：%d\n", count);
		}
		else if (command.find("/set ServerRootDirectory") == 0) {
			std::string directory = command.substr(25);
			ServerRootDirectory = directory;
			printf("服务器根目录设置为：%s\n", directory.c_str());
		}
		else if (command.find("/set MAXRETRIES") == 0) {
			std::string retriesStr = command.substr(15);
			int retries = std::stoi(retriesStr);
			MAX_RETRIES = retries;
			printf("最大重传次数设置为：%d\n", retries);
		}
		else if (command == "/stop") {
			printf("正在停止TFTP服务器...\n");
			Sleep(1000);
			running = false;
			is_console_mode = false;
			CloseConsoleWindow();
			mainFrame->Server_Switch->SetSelection(1);
			break;
		}
		else {
			printf("未知命令，请输入/help获取帮助信息\n");
		}
	}
}

// 服务器线程
void StartServer() {
	TFTPServer server;
	std::thread serverThread(&TFTPServer::run, &server); // 创建服务器线程
	serverThread.join();
	return;
}
