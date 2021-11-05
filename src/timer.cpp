#include "pch.h"

#include "timer.h"

auto XKLib::Timer::start() -> void
{
    _start = view_as<uint64_t>(std::chrono::high_resolution_clock::now()
                                 .time_since_epoch()
                                 .count());
}

auto XKLib::Timer::end() -> void
{
    _end = view_as<uint64_t>(std::chrono::high_resolution_clock::now()
                               .time_since_epoch()
                               .count());

    _difference = _end - _start;
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
