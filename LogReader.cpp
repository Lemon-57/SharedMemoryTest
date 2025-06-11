#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <string>
#include "LogDll.h"

class LogReader
{
private:
    bool m_running;
    std::thread m_readerThread;
    int m_totalLogsRead;

public:
    LogReader() : m_running(false), m_totalLogsRead(0) {}

    bool Start()
    {
        if (m_running) return true;

        // 初始化日志DLL
        if (!InitializeLogDll())
        {
            std::cerr << "错误：初始化日志DLL失败！" << std::endl;
            return false;
        }

        std::cout << "日志读取器初始化成功。" << std::endl;
        std::cout << "等待来自其他进程的日志消息..." << std::endl;
        std::cout << "========================================" << std::endl;

        m_running = true;
        m_readerThread = std::thread(&LogReader::ReaderLoop, this);
        return true;
    }

    void Stop()
    {
        if (!m_running) return;

        m_running = false;
        if (m_readerThread.joinable())
        {
            m_readerThread.join();
        }

        CleanupLogDll();
        std::cout << "\n========================================" << std::endl;
        std::cout << "日志读取器已停止。总共读取日志数：" << m_totalLogsRead << std::endl;
    }

    void ShowStatus()
    {
        LONG currentCount = GetLogCount();
        std::cout << "\n[状态] 当前缓冲区数量：" << currentCount
            << "，总共读取：" << m_totalLogsRead << std::endl;
    }

private:
    void ReaderLoop()
    {
        LogEntry entry;
        auto lastStatusTime = std::chrono::steady_clock::now();

        while (m_running)
        {
            bool hasNewLogs = false;

            // 读取所有可用日志
            while (ReadLog(&entry))
            {
                PrintLog(entry);
                m_totalLogsRead++;
                hasNewLogs = true;
            }

            // 如果没有新日志，每5秒显示一次状态
            auto now = std::chrono::steady_clock::now();
            if (!hasNewLogs &&
                std::chrono::duration_cast<std::chrono::seconds>(now - lastStatusTime).count() >= 5)
            {
                ShowStatus();
                lastStatusTime = now;
            }

            // 短暂休眠以避免高CPU使用率
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void PrintLog(const LogEntry& entry)
    {
        // 获取带颜色的级别字符串
        const char* levelStr = "";
        const char* colorCode = "";

        switch (entry.level)
        {
        case LOG_LEVEL_DEBUG:
            levelStr = "调试";
            colorCode = "\033[36m"; // 青色
            break;
        case LOG_LEVEL_INFO:
            levelStr = "信息";
            colorCode = "\033[32m"; // 绿色
            break;
        case LOG_LEVEL_WARNING:
            levelStr = "警告";
            colorCode = "\033[33m"; // 黄色
            break;
        case LOG_LEVEL_ERROR:
            levelStr = "错误";
            colorCode = "\033[31m"; // 红色
            break;
        }

        // 将时间戳转换为可读格式
        time_t seconds = static_cast<time_t>(entry.timestamp / 1000);
        int milliseconds = static_cast<int>(entry.timestamp % 1000);

        struct tm timeinfo;
        localtime_s(&timeinfo, &seconds);

        // 打印时间戳、级别、进程信息和消息
        std::cout << "["
            << std::setfill('0') << std::setw(4) << (timeinfo.tm_year + 1900) << "-"
            << std::setfill('0') << std::setw(2) << (timeinfo.tm_mon + 1) << "-"
            << std::setfill('0') << std::setw(2) << timeinfo.tm_mday << " "
            << std::setfill('0') << std::setw(2) << timeinfo.tm_hour << ":"
            << std::setfill('0') << std::setw(2) << timeinfo.tm_min << ":"
            << std::setfill('0') << std::setw(2) << timeinfo.tm_sec << "."
            << std::setfill('0') << std::setw(3) << milliseconds
            << "] "
            << colorCode << "[" << std::setw(7) << levelStr << "]" << "\033[0m"
            << " [PID:" << entry.processId << "] [TID:" << entry.threadId << "] "
            << entry.text << std::endl;
    }
};

int main()
{
    // 设置控制台输出为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    // 确保标准错误流也使用 UTF-8
    SetConsoleCP(CP_UTF8);

    std::cout << "=== DLL共享内存日志读取器 ===" << std::endl;
    std::cout << "此程序从共享内存读取日志。" << std::endl;
    std::cout << "启动LogWriter程序以查看日志消息。" << std::endl;
    std::cout << "按 'q' + 回车键退出。" << std::endl << std::endl;

    LogReader reader;
    if (!reader.Start())
    {
        std::cerr << "启动日志读取器失败！" << std::endl;
        return -1;
    }

    // 等待用户输入以退出
    std::string input;
    while (std::getline(std::cin, input))
    {
        if (input == "q" || input == "Q" || input == "quit" || input == "退出")
        {
            break;
        }
        else if (input == "status" || input == "s" || input == "状态")
        {
            reader.ShowStatus();
        }
        else if (input == "help" || input == "h" || input == "帮助")
        {
            std::cout << "命令：" << std::endl;
            std::cout << "  q, quit, 退出 - 退出程序" << std::endl;
            std::cout << "  s, status, 状态 - 显示当前状态" << std::endl;
            std::cout << "  h, help, 帮助 - 显示此帮助" << std::endl;
        }
    }

    reader.Stop();
    return 0;
}
