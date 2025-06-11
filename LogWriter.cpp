#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <string>
#include "LogDll.h"

class LogWriter
{
private:
    bool m_running;
    std::thread m_writerThread;
    std::mt19937 m_rng;
    int m_messageCounter;
    int m_totalMessagesSent;

    // 仿真用的示例日志消息
    std::vector<std::pair<LogLevel, std::string>> m_sampleMessages = {
        {LOG_LEVEL_INFO, "系统初始化开始"},
        {LOG_LEVEL_INFO, "正在加载配置文件"},
        {LOG_LEVEL_DEBUG, "内存分配：1024KB"},
        {LOG_LEVEL_INFO, "网络连接已建立"},
        {LOG_LEVEL_DEBUG, "正在处理数据包"},
        {LOG_LEVEL_WARNING, "检测到高CPU使用率：85%"},
        {LOG_LEVEL_INFO, "数据处理完成"},
        {LOG_LEVEL_DEBUG, "缓存命中率：92%"},
        {LOG_LEVEL_WARNING, "内存使用超过阈值：90%"},
        {LOG_LEVEL_ERROR, "连接超时"},
        {LOG_LEVEL_INFO, "尝试重新连接"},
        {LOG_LEVEL_INFO, "备份进程已启动"},
        {LOG_LEVEL_DEBUG, "正在验证数据完整性"},
        {LOG_LEVEL_WARNING, "磁盘空间不足：剩余15%"},
        {LOG_LEVEL_ERROR, "写入磁盘失败"},
        {LOG_LEVEL_INFO, "恢复程序已启动"},
        {LOG_LEVEL_DEBUG, "线程池大小：8"},
        {LOG_LEVEL_INFO, "性能优化已应用"},
        {LOG_LEVEL_WARNING, "检测到已弃用的API使用"},
        {LOG_LEVEL_INFO, "系统健康检查通过"}
    };

public:
    LogWriter() : m_running(false), m_rng(std::random_device{}()),
        m_messageCounter(0), m_totalMessagesSent(0)
    {
    }

    bool Start()
    {
        if (m_running) return true;

        // 初始化日志DLL
        if (!InitializeLogDll())
        {
            std::cerr << "错误：初始化日志DLL失败！" << std::endl;
            return false;
        }

        std::cout << "日志写入器初始化成功。" << std::endl;
        std::cout << "开始仿真并发送日志..." << std::endl;
        std::cout << "========================================" << std::endl;

        // 发送初始启动消息
        POST_INFO("日志写入器仿真已启动");

        m_running = true;
        m_writerThread = std::thread(&LogWriter::WriterLoop, this);
        return true;
    }

    void Stop()
    {
        if (!m_running) return;

        POST_INFO("日志写入器仿真正在停止");
        m_running = false;

        if (m_writerThread.joinable())
        {
            m_writerThread.join();
        }

        POST_INFO("日志写入器仿真已停止");
        CleanupLogDll();

        std::cout << "\n========================================" << std::endl;
        std::cout << "仿真已停止。总共发送消息数：" << m_totalMessagesSent << std::endl;
    }

    void SendBurstMessages(int count)
    {
        std::cout << "发送批量消息：" << count << " 条..." << std::endl;
        for (int i = 0; i < count; i++)
        {
            SendRandomMessage();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void SendTestSequence()
    {
        std::cout << "发送测试序列..." << std::endl;

        POST_DEBUG("=== 测试序列开始 ===");
        POST_INFO("测试所有日志级别");
        POST_WARNING("这是一条警告消息");
        POST_ERROR("这是一条错误消息");
        POST_DEBUG("=== 测试序列完成 ===");

        std::cout << "测试序列已发送。" << std::endl;
    }

private:
    void WriterLoop()
    {
        std::uniform_int_distribution<int> delayDist(500, 3000); // 0.5-3秒
        std::uniform_int_distribution<int> burstDist(1, 100);    // 1%的批量发送概率

        while (m_running)
        {
            // 正常消息发送
            SendRandomMessage();

            // 偶尔发送批量消息
            if (burstDist(m_rng) == 1)
            {
                int burstSize = std::uniform_int_distribution<int>(3, 8)(m_rng);
                for (int i = 0; i < burstSize && m_running; i++)
                {
                    SendRandomMessage();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }

            // 消息间的随机延迟
            int delay = delayDist(m_rng);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }

    void SendRandomMessage()
    {
        if (m_sampleMessages.empty()) return;

        // 选择随机消息
        std::uniform_int_distribution<size_t> msgDist(0, m_sampleMessages.size() - 1);
        auto& [level, baseMessage] = m_sampleMessages[msgDist(m_rng)];

        // 添加计数器使消息唯一
        std::string message = baseMessage + " (#" + std::to_string(++m_messageCounter) + ")";

        // 发送消息
        PostLogWithCurrentTime(level, message.c_str());
        m_totalMessagesSent++;

        // 打印发送的内容到控制台
        const char* levelStr = "";
        switch (level)
        {
        case LOG_LEVEL_DEBUG:   levelStr = "调试"; break;
        case LOG_LEVEL_INFO:    levelStr = "信息"; break;
        case LOG_LEVEL_WARNING: levelStr = "警告"; break;
        case LOG_LEVEL_ERROR:   levelStr = "错误"; break;
        }

        std::cout << "[已发送] [" << levelStr << "] " << message << std::endl;
    }
};

int main()
{
    // 设置控制台输出为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    // 确保标准错误流也使用 UTF-8
    SetConsoleCP(CP_UTF8);

    std::cout << "=== DLL共享内存日志写入器（仿真器） ===" << std::endl;
    std::cout << "此程序模拟向共享内存发送日志的进程。" << std::endl;
    std::cout << "确保LogReader正在运行以查看消息。" << std::endl;
    std::cout << "命令：'q' 退出，'b' 批量发送，'t' 测试序列" << std::endl << std::endl;

    LogWriter writer;
    if (!writer.Start())
    {
        std::cerr << "启动日志写入器失败！" << std::endl;
        return -1;
    }

    // 交互命令循环
    std::string input;
    std::cout << "仿真运行中。输入命令：" << std::endl;

    while (std::getline(std::cin, input))
    {
        if (input == "q" || input == "Q" || input == "quit" || input == "退出")
        {
            break;
        }
        else if (input == "b" || input == "burst" || input == "批量")
        {
            writer.SendBurstMessages(5);
        }
        else if (input == "t" || input == "test" || input == "测试")
        {
            writer.SendTestSequence();
        }
        else if (input == "help" || input == "h" || input == "帮助")
        {
            std::cout << "命令：" << std::endl;
            std::cout << "  q, quit, 退出 - 退出程序" << std::endl;
            std::cout << "  b, burst, 批量 - 发送批量消息" << std::endl;
            std::cout << "  t, test, 测试 - 发送测试序列" << std::endl;
            std::cout << "  h, help, 帮助 - 显示此帮助" << std::endl;
        }
        else if (!input.empty())
        {
            std::cout << "未知命令。输入 '帮助' 查看可用命令。" << std::endl;
        }
    }

    writer.Stop();
    return 0;
}
