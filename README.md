# 简介

一个基于C++的Socket编程实现的TFTP服务器，支持文件上传下载、图形界面、多线程传输等功能。

主页面：

<img src=".\assets\UI-main.png" alt="主页面" style="zoom:80%;" />

设置页面：

<img src=".\assets\UI-settings.png" alt="UI-settings.png" style="zoom: 67%;" />

## 功能特性

- 严格遵循TFTP协议标准实现
- 支持文件上传和下载
- 图形用户界面(GUI)和命令行界面(CLI)双模式
- 多线程并发传输
- 文件传输结果展示
- 传输失败原因提示
- 显示文件传输吞吐量
- 完整的日志记录系统
- 支持指定网卡接口运行
- 异常环境下的可靠传输

## 开发环境

- 操作系统: Windows 11
- IDE: Visual Studio 2022

## 依赖库

- wxWidgets: wxMSW-3.2.6_vc14x_Dev (用于GUI)
- Boost: boost_1_86_0 (用于获取网卡信息)

## 系统架构

系统包含4个主要模块:

- UI: 图形界面实现
- API: 封装的功能函数
- Server: TFTP服务器核心实现
- Shared: 全局变量和公共组件

## 主要特性

- 超时重传机制
- 客户端TID验证
- 多线程互斥锁保护
- 可配置的传输参数
- 完整的错误处理
- 友好的用户界面

# 构建

构建本项目需编译wxWidgets和Boost库

# 待优化

- 动态更新超时阈值
- 减少全局变量使用
- 优化日志记录系统
- 改进线程管理(线程池)
- 提升重传效率

# 贡献

欢迎提交Issue和PR来帮助改进项目。