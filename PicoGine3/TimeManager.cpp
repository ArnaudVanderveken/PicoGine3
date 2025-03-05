#include "pch.h"
#include "TimeManager.h"

using namespace std::chrono;

void TimeManager::Initialize()
{
	m_StartTime = high_resolution_clock::now();
	m_FrameBeginTime = high_resolution_clock::now();
	m_IsInitialized = true;
}

bool TimeManager::IsInitialized() const
{
	return m_IsInitialized;
}

void TimeManager::Update()
{
	m_LastFrameBeginTime = m_FrameBeginTime;
	m_FrameBeginTime = high_resolution_clock::now();
	m_DeltaTime = duration<float>(m_FrameBeginTime - m_LastFrameBeginTime).count();
	m_TotalTime = duration<float>(m_FrameBeginTime - m_StartTime).count();
}

float TimeManager::GetElapsedTime() const
{
	return m_DeltaTime;
}

float TimeManager::GetTotalTime() const
{
	return m_TotalTime;
}

std::chrono::duration<float> TimeManager::GetTimeToNextFrame()
{
	m_FrameEndTime = high_resolution_clock::now();
	return duration<float>(m_TimePerFrame - duration<float>(m_FrameEndTime - m_FrameBeginTime).count());
}

float TimeManager::GetFixedTimeStep() const
{
	return m_FixedTimeStep;
}

void TimeManager::SetTargetFPS(float fps)
{
	m_TimePerFrame = 1.0f / fps;
}
