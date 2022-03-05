#pragma once
#include "stdafx.h"

class Timer
{
public:
	Timer();
	~Timer() = default;

	void Tick();
	FLOAT GetDeltaTime() const { return m_deltaTime; }
	FLOAT GetFPS() const { return 1.0f / m_deltaTime; }

private:
	LARGE_INTEGER	m_tick;
	LARGE_INTEGER	m_frequency;
	FLOAT			m_deltaTime;
};