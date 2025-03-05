#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include "Singleton.h"

#include <chrono>

class TimeManager : public Singleton<TimeManager>
{
	friend class Singleton<TimeManager>;
	explicit TimeManager() = default;

public:
	~TimeManager() override = default;

	TimeManager(const TimeManager&) noexcept = delete;
	TimeManager& operator=(const TimeManager&) noexcept = delete;
	TimeManager(TimeManager&&) noexcept = delete;
	TimeManager& operator=(TimeManager&&) noexcept = delete;

	void Initialize();
	[[nodiscard]] bool IsInitialized() const;

	void Update();

	[[nodiscard]] float GetElapsedTime() const;
	[[nodiscard]] float GetTotalTime() const;
	[[nodiscard]] std::chrono::duration<float> GetTimeToNextFrame();
	[[nodiscard]] float GetFixedTimeStep() const;

	void SetTargetFPS(float fps);

private:
	bool m_IsInitialized{};

	const float m_FixedTimeStep{ 1.0f / 256.0f };
	float m_TimePerFrame{ 1.0f / 240.0f };

	float m_DeltaTime{};
	float m_TotalTime{};

	std::chrono::steady_clock::time_point m_StartTime{};
	std::chrono::steady_clock::time_point m_FrameBeginTime{};
	std::chrono::steady_clock::time_point m_LastFrameBeginTime{};
	std::chrono::steady_clock::time_point m_FrameEndTime{};

};

#endif //TIMEMANAGER_H