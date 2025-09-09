#pragma once

class CadLogger {
public:
	enum LogLevel {
		LOG_INFO,
		LOG_WARNING,
		LOG_ERROR
	};

	static void Log(LogLevel level, const TCHAR* format, ...);

	// 添加便捷方法，默认为INFO级别
	static void LogInfo(const TCHAR* format, ...);
	static void LogWarning(const TCHAR* format, ...);
	static void LogError(const TCHAR* format, ...);
};