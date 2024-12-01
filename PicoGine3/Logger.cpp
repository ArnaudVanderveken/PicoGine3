#include "pch.h"
#include "Logger.h"

#include <comdef.h>
#include <filesystem>
#include <format>

Logger::~Logger()
{
	if (m_OutputStream)
		m_OutputStream.close();
}

void Logger::LogInfo(const std::wstring& logString, const std::source_location& sourceLocation)
{
	ProcessLog(LogLevel::Info, logString, sourceLocation);
}

void Logger::LogDebug(const std::wstring& logString, const std::source_location& sourceLocation)
{
	ProcessLog(LogLevel::Debug, logString, sourceLocation);
}

void Logger::LogWarning(const std::wstring& logString, const std::source_location& sourceLocation)
{
	ProcessLog(LogLevel::Warning, logString, sourceLocation);
}

void Logger::LogError(const std::wstring& logString, const std::source_location& sourceLocation)
{
	ProcessLog(LogLevel::Error, logString, sourceLocation);
}

void Logger::LogError(HRESULT hres, const std::source_location& sourceLocation)
{
	std::wstringstream logStream{};
	const _com_error err{ hres };
	logStream << L"[HRESULT 0x" << std::hex << hres << L"] " << err.ErrorMessage() << L'\n';

	ProcessLog(LogLevel::Error, logStream.str(), sourceLocation);
}

void Logger::LogTodo(const std::wstring& logString, const std::source_location& sourceLocation)
{
	ProcessLog(LogLevel::Todo, logString, sourceLocation);
}

Logger::Logger()
{
	m_OutputStream.open(k_LogFileName);
}

void Logger::ProcessLog(LogLevel level, const std::wstring& logString, const std::source_location& sourceLocation)
{
#ifndef _DEBUG
	if (level == LogLevel::Debug || level == LogLevel::Todo)
		return;
#endif //_DEBUG

	if (!m_OutputStream)
	{
		std::cerr << "Unable to write log to file" << std::endl;
		return;
	}

	// Extract information
	const auto& levelStr{ k_LevelsToString[static_cast<int>(level)] };
	const auto filename{ std::filesystem::path{ sourceLocation.file_name() }.filename().wstring() };
	const auto lineNumber{ sourceLocation.line() };
	SYSTEMTIME st;
	GetSystemTime(&st);

	// Add log entry
	std::wstringstream logEntry{};
	logEntry << L"[" << st.wYear << L"-" << st.wMonth << L"-" << st.wDay << L"|";
	logEntry << st.wHour << L":" << st.wMinute << L":" << st.wSecond << L"]";
	logEntry << std::format(L"[{}] {}:{}\n -> {}\n", levelStr, filename, lineNumber, logString);
	m_OutputStream << logEntry.str();
	m_OutputStream.flush();

	if (level == LogLevel::Error)
	{
		MessageBox(nullptr, logEntry.str().c_str(), L"ERROR", MB_OK | MB_ICONERROR);
	}
}
