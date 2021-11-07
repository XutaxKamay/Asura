#include "pch.h"

#include "timer.h"

auto XKLib::Timer::start() -> void
{
    _start = std::chrono::high_resolution_clock::now();
}

auto XKLib::Timer::end() -> void
{
    _end = std::chrono::high_resolution_clock::now();

    _difference = std::chrono::nanoseconds(_end - _start).count();
}

auto XKLib::Timer::nanos() -> uint16_t
{
    return _difference % 1000;
}

auto XKLib::Timer::micros() -> uint16_t
{
    return (_difference / 1000) % 1000;
}

auto XKLib::Timer::millis() -> uint16_t
{
    return (_difference / 1000000) % 1000;
}

auto XKLib::Timer::seconds() -> uint64_t
{
    return (_difference / 1000000000);
}

auto XKLib::Timer::difference() -> uint64_t
{
    return _difference;
}
