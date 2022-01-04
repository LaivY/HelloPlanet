#include "timer.h"

Timer::Timer() : m_tick{}, m_deltaTime { 0.0f }
{
	QueryPerformanceFrequency(&m_frequency);
}

void Timer::Tick()
{
	LARGE_INTEGER temp;
	QueryPerformanceCounter(&temp);
	m_deltaTime = (temp.QuadPart - m_tick.QuadPart) / static_cast<FLOAT>(m_frequency.QuadPart);
	m_tick = temp;
}