#include "../stdafx.h"
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
		acutPrintf(_T("\n[信息] %s\n"), buffer);
		break;
	case LOG_WARNING:
		acutPrintf(_T("\n[警告] %s\n"), buffer);
		break;
	case LOG_ERROR:
		acutPrintf(_T("\n[错误] %s\n"), buffer);
		acedAlert(buffer);
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
