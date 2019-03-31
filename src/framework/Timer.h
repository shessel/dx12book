#pragma once
#include <chrono>

class Timer
{
public:
    Timer();

    void reset();
    void tick();
    void stop();
    void start();
    float getDelta() const;
    float getElapsedTime() const;
private:
    using clock_type = std::chrono::high_resolution_clock;
    using time_point = std::chrono::time_point<clock_type>;
    using duration = clock_type::duration;

    time_point m_baseTime;
    time_point m_prevTime;
    time_point m_curTime;
    time_point m_stopTime;
    duration m_pausedTime;
    float m_delta;
    bool m_stopped;
};