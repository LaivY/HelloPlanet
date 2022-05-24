#pragma once
#include "stdafx.h"

class Timer
{
public:
	Timer();
	~Timer() = default;

	void Tick();
	FLOAT GetDeltaTime() const;
	FLOAT GetFPS() const;

private:
	LARGE_INTEGER	m_tick;
	LARGE_INTEGER	m_frequency;
	FLOAT			m_deltaTime;
	deque<FLOAT>	m_deltaTimes;
};