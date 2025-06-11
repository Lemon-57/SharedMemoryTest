#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "LogDll.h"


// 定义共享数据段
#pragma data_seg("SHARED")
SharedLogBuffer g_sharedBuffer = { 0 };
volatile LONG g_initialized = 0;
#pragma data_seg()

// 告诉链接器将SHARED段设置为可读写共享
#pragma comment(linker, "/section:SHARED,RWS")

// 本地变量
static HANDLE g_mutex = NULL;
static const char* MUTEX_NAME = "Global\\MainLogDllMutex";

// 获取当前时间戳（毫秒）
static LogTimestamp GetCurrentTimestamp()
{
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // 转换为毫秒（Windows FILETIME是从1601年开始的100纳秒单位）
    return (uli.QuadPart / 10000) - 11644473600000LL;
}

// 初始化DLL
LOGAPI BOOL InitializeLogDll()
{
    // 创建或打开全局互斥体
    g_mutex = CreateMutexA(NULL, FALSE, MUTEX_NAME);
    if (g_mutex == NULL)
    {
        return FALSE;
    }

    // 使用互斥体保护初始化过程
    WaitForSingleObject(g_mutex, INFINITE);

    // 检查是否已经初始化
    if (InterlockedCompareExchange(&g_initialized, 1, 0) == 0)
    {
        // 首次初始化
        memset((void*)&g_sharedBuffer, 0, sizeof(SharedLogBuffer));
        g_sharedBuffer.writeIndex = 0;
        g_sharedBuffer.readIndex = 0;
        g_sharedBuffer.entryCount = 0;
    }

    ReleaseMutex(g_mutex);
    return TRUE;
}

// 清理DLL
LOGAPI void CleanupLogDll()
{
    if (g_mutex)
    {
        CloseHandle(g_mutex);
        g_mutex = NULL;
    }
}

// 发送日志
LOGAPI BOOL PostLog(LogTimestamp timestamp, LogLevel level, const char* logText)
{
    if (!logText || !g_mutex)
    {
        return FALSE;
    }

    WaitForSingleObject(g_mutex, INFINITE);

    // 计算写入位置
    LONG writePos = g_sharedBuffer.writeIndex % LOG_CAPACITY;

    // 填充日志条目
    LogEntry* entry = &g_sharedBuffer.entries[writePos];
    entry->timestamp = timestamp;
    entry->level = level;
    entry->processId = GetCurrentProcessId();
    entry->threadId = GetCurrentThreadId();

    // 安全复制文本
    strncpy_s(entry->text, LOG_TEXT_MAX_LEN, logText, _TRUNCATE);

    // 原子更新写入索引
    InterlockedIncrement(&g_sharedBuffer.writeIndex);

    // 更新条目计数（但不超过容量）
    if (g_sharedBuffer.entryCount < LOG_CAPACITY)
    {
        InterlockedIncrement(&g_sharedBuffer.entryCount);
    }
    else
    {
        // 缓冲区已满，移动读取索引
        InterlockedIncrement(&g_sharedBuffer.readIndex);
    }

    ReleaseMutex(g_mutex);
    return TRUE;
}

// 使用当前时间发送日志
LOGAPI BOOL PostLogWithCurrentTime(LogLevel level, const char* logText)
{
    return PostLog(GetCurrentTimestamp(), level, logText);
}

// 读取日志
LOGAPI BOOL ReadLog(LogEntry* entry)
{
    if (!entry || !g_mutex)
    {
        return FALSE;
    }

    WaitForSingleObject(g_mutex, INFINITE);

    // 检查是否有可读取的日志
    if (g_sharedBuffer.readIndex >= g_sharedBuffer.writeIndex)
    {
        ReleaseMutex(g_mutex);
        return FALSE; // 没有新日志
    }

    // 读取日志条目
    LONG readPos = g_sharedBuffer.readIndex % LOG_CAPACITY;
    *entry = g_sharedBuffer.entries[readPos];

    // 更新读取索引
    InterlockedIncrement(&g_sharedBuffer.readIndex);
    InterlockedDecrement(&g_sharedBuffer.entryCount);

    ReleaseMutex(g_mutex);
    return TRUE;
}

// 获取当前日志数量
LOGAPI LONG GetLogCount()
{
    return g_sharedBuffer.entryCount;
}

// 清除日志
LOGAPI void ClearLogs()
{
    if (!g_mutex) return;

    WaitForSingleObject(g_mutex, INFINITE);
    g_sharedBuffer.writeIndex = 0;
    g_sharedBuffer.readIndex = 0;
    g_sharedBuffer.entryCount = 0;
    ReleaseMutex(g_mutex);
}

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        return InitializeLogDll();
    case DLL_PROCESS_DETACH:
        CleanupLogDll();
        break;
    }
    return TRUE;
}
