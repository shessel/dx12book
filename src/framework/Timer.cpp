#include "Timer.h"

Timer::Timer() :
    m_baseTime(clock_type::now()),
    m_prevTime(clock_type::now()),
    m_curTime(clock_type::now()),
    m_stopTime(clock_type::now()),
    m_pausedTime(0),
    m_delta(0.0f),
    m_stopped(true)
{}

void Timer::reset()
{
    m_baseTime = clock_type::now();
    m_prevTime = clock_type::now();
    m_curTime = clock_type::now();
    m_stopTime = clock_type::now();
    m_pausedTime = duration(0);
    m_delta = 0.0f;
}

void Timer::tick()
{
    m_curTime = clock_type::now();
    m_delta = std::chrono::duration<float>(m_curTime - m_prevTime).count();
    m_prevTime = m_curTime;
}

void Timer::stop()
{
    if (!m_stopped)
    {
        m_stopTime = clock_type::now();
        m_stopped = true;
    }
}

void Timer::start()
{
    if (m_stopped)
    {
        time_point startTime = clock_type::now();
        m_prevTime = startTime;
        m_pausedTime += startTime - m_stopTime;
        m_stopped = false;
    }
}

float Timer::getDelta() const
{
    return m_delta;
}

float Timer::getElapsedTime() const
{
    if (m_stopped)
    {
        return std::chrono::duration<float>((m_stopTime - m_baseTime) - m_pausedTime).count();
    }
    else
    {
        return std::chrono::duration<float>((m_curTime - m_baseTime) - m_pausedTime).count();
    }
}
