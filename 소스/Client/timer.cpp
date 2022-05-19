#include "stdafx.h"
#include "timer.h"

Timer::Timer() : m_tick{}, m_deltaTime{}
{
	QueryPerformanceFrequency(&m_frequency);
}

void Timer::Tick()
{
	LARGE_INTEGER temp;
	QueryPerformanceCounter(&temp);
	m_deltaTime = (temp.QuadPart - m_tick.QuadPart) / static_cast<FLOAT>(m_frequency.QuadPart);
	m_tick = temp;

	if (m_deltaTimes.size() >= 60)
		m_deltaTimes.pop_front();
	m_deltaTimes.push_back(m_deltaTime);
}

FLOAT Timer::GetDeltaTime() const
{
	return m_deltaTime;
}

FLOAT Timer::GetFPS() const
{
	float fps{};
	for (float deltaTime : m_deltaTimes)
		fps += deltaTime;
	fps /= m_deltaTimes.size();
	return 1.0f / fps;
}