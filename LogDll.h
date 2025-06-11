#pragma once

#include <windows.h>

#ifdef MAINLOGDLL_EXPORTS
#define LOGAPI __declspec(dllexport)
#else
#define LOGAPI __declspec(dllimport)
#endif

// 日志配置
#define LOG_TEXT_MAX_LEN 512
#define LOG_CAPACITY 1024

#ifdef __cplusplus
extern "C" {
#endif

    // 日志级别
    typedef enum
    {
        LOG_LEVEL_DEBUG = 0,
        LOG_LEVEL_INFO = 1,
        LOG_LEVEL_WARNING = 2,
        LOG_LEVEL_ERROR = 3
    } LogLevel;

    // 使用64位整数表示时间戳（毫秒）
    typedef __int64 LogTimestamp;

    // 日志条目结构
    typedef struct
    {
        LogTimestamp timestamp;           // 时间戳（毫秒）
        LogLevel level;                   // 日志级别
        char text[LOG_TEXT_MAX_LEN];     // 日志文本（UTF-8编码）
        DWORD processId;                 // 进程ID
        DWORD threadId;                  // 线程ID
    } LogEntry;

    // 共享数据结构
    typedef struct
    {
        volatile LONG writeIndex;        // 写入索引
        volatile LONG readIndex;         // 读取索引
        volatile LONG entryCount;        // 当前条目数量
        LogEntry entries[LOG_CAPACITY];  // 日志条目数组
    } SharedLogBuffer;

    // 导出函数接口
    LOGAPI BOOL InitializeLogDll();
    LOGAPI void CleanupLogDll();
    LOGAPI BOOL PostLog(LogTimestamp timestamp, LogLevel level, const char* logText);
    LOGAPI BOOL PostLogWithCurrentTime(LogLevel level, const char* logText);
    LOGAPI BOOL ReadLog(LogEntry* entry);
    LOGAPI LONG GetLogCount();
    LOGAPI void ClearLogs();

    // 便利宏定义
#define POST_DEBUG(text) PostLogWithCurrentTime(LOG_LEVEL_DEBUG, text)
#define POST_INFO(text) PostLogWithCurrentTime(LOG_LEVEL_INFO, text)
#define POST_WARNING(text) PostLogWithCurrentTime(LOG_LEVEL_WARNING, text)
#define POST_ERROR(text) PostLogWithCurrentTime(LOG_LEVEL_ERROR, text)

#ifdef __cplusplus
}
#endif
