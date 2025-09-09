#include "StdAfx.h"
#include "CadLogger.h"

void CadLogger::Log(LogLevel level, const TCHAR* format, ...)
{
	TCHAR buffer[1024];
	va_list args;
	va_start(args, format);
	_vstprintf_s(buffer, format, args);
	va_end(args);

	switch (level) {
	case LOG_INFO:
		acutPrintf(_T("\n[INFO] %s\n"), buffer);
		break;
	case LOG_WARNING:
		acutPrintf(_T("\n[WARNING] %s\n"), buffer);
		break;
	case LOG_ERROR:
		acutPrintf(_T("\n[ERROR] %s\n"), buffer);
		// 临时注释掉 acedAlert，避免乱码弹窗
		// acedAlert(buffer);
		break;
	}
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
