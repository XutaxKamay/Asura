#include "pch.h"

#include "timer.h"

auto XKLib::Timer::nanos() const -> std::uint16_t
{
    return _difference % 1000;
}

auto XKLib::Timer::micros() const -> std::uint16_t
{
    return (_difference / 1000) % 1000;
}

auto XKLib::Timer::millis() const -> std::uint16_t
{
    return (_difference / 1000000) % 1000;
}

auto XKLib::Timer::seconds() const -> std::uint64_t
{
    return (_difference / 1000000000);
}

auto XKLib::Timer::difference() const -> std::uint64_t
{
    return _difference;
}

auto XKLib::Timer::start() -> void
{
    _start = std::chrono::high_resolution_clock::now();
}

auto XKLib::Timer::end() -> void
{
    _end = std::chrono::high_resolution_clock::now();

    _difference = std::chrono::nanoseconds(_end - _start).count();
}
