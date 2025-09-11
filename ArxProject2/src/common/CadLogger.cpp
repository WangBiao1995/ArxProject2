#include "StdAfx.h"
#include "CadLogger.h"
#include <iostream>
#include <fstream>
#include <ctime>

void CadLogger::Log(LogLevel level, const TCHAR* format, ...)
{
    TCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    _vstprintf_s(buffer, format, args);
    va_end(args);

    // 获取当前时间
    time_t now = time(0);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    
    TCHAR timeStr[64];
    _stprintf_s(timeStr, _T("%04d-%02d-%02d %02d:%02d:%02d"), 
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    switch (level) {
    case LOG_INFO:
        // 使用英文避免乱码
        acutPrintf(_T("\n[INFO %s] %s\n"), timeStr, buffer);
        break;
    case LOG_WARNING:
        acutPrintf(_T("\n[WARNING %s] %s\n"), timeStr, buffer);
        break;
    case LOG_ERROR:
        acutPrintf(_T("\n[ERROR %s] %s\n"), timeStr, buffer);
        break;
    }
    
    // 同时写入到文件日志（可选）
    writeToLogFile(level, timeStr, buffer);
}

void CadLogger::LogInfo(const TCHAR* format, ...)
{
    TCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    _vstprintf_s(buffer, format, args);
    va_end(args);
    
    Log(LOG_INFO, _T("%s"), buffer);
}

void CadLogger::LogWarning(const TCHAR* format, ...)
{
    TCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    _vstprintf_s(buffer, format, args);
    va_end(args);
    
    Log(LOG_WARNING, _T("%s"), buffer);
}

void CadLogger::LogError(const TCHAR* format, ...)
{
    TCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    _vstprintf_s(buffer, format, args);
    va_end(args);
    
    Log(LOG_ERROR, _T("%s"), buffer);
}

void CadLogger::writeToLogFile(LogLevel level, const TCHAR* timeStr, const TCHAR* message)
{
    // 写入文件日志，支持中文
    std::wofstream logFile;
    logFile.open(L"ArxProject2.log", std::ios::app);
    if (logFile.is_open()) {
        const wchar_t* levelStr = L"INFO";
        switch (level) {
        case LOG_WARNING: levelStr = L"WARNING"; break;
        case LOG_ERROR: levelStr = L"ERROR"; break;
        }
        
        logFile << L"[" << levelStr << L" " << timeStr << L"] " << message << std::endl;
        logFile.close();
    }
}
