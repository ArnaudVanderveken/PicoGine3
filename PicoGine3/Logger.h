#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <source_location>

#include "Singleton.h"


class Logger final : public Singleton<Logger>
{
public:
	enum class LogLevel
	{
		Info,
		Debug,
		Warning,
		Error,
		Todo,
	};

	~Logger() override;

	Logger(const Logger&) noexcept = delete;
	Logger& operator=(const Logger&) noexcept = delete;
	Logger(Logger&&) noexcept = delete;
	Logger& operator=(Logger&&) noexcept = delete;

	void LogInfo(const std::wstring& logString, const std::source_location& sourceLocation = std::source_location::current());
	void LogDebug(const std::wstring& logString, const std::source_location& sourceLocation = std::source_location::current());
	void LogWarning(const std::wstring& logString, const std::source_location& sourceLocation = std::source_location::current());
	void LogError(const std::wstring& logString, const std::source_location& sourceLocation = std::source_location::current());
	void LogError(HRESULT hres, const std::source_location& sourceLocation = std::source_location::current());
	void LogTodo(const std::wstring& logString, const std::source_location& sourceLocation = std::source_location::current());

protected:
	friend Singleton<Logger>;
	Logger();

private:
	inline static const std::wstring k_LogFileName{ L"PicoGine3_Log.txt" };
	inline static const std::wstring k_LevelsToString[]{ L"INFO", L"DEBUG", L"WARNING", L"ERROR", L"TODO" };

	std::wofstream m_OutputStream{};

	void ProcessLog(LogLevel level, const std::wstring& logString, const std::source_location& sourceLocation);

};

#endif //LOGGER_H