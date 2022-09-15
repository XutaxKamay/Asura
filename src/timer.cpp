#include "pch.h"

#include "timer.h"

auto Asura::Timer::nanos() const -> std::uint16_t
{
    return _difference % 1000;
}

auto Asura::Timer::micros() const -> std::uint16_t
{
    return (_difference / 1000) % 1000;
}

auto Asura::Timer::millis() const -> std::uint16_t
{
    return (_difference / 1000000) % 1000;
}

auto Asura::Timer::seconds() const -> std::uint64_t
{
    return (_difference / 1000000000);
}

auto Asura::Timer::difference() const -> std::uint64_t
{
    return _difference;
}

auto Asura::Timer::start() -> void
{
    _start = std::chrono::high_resolution_clock::now();
}

auto Asura::Timer::end() -> void
{
    _end = std::chrono::high_resolution_clock::now();

    _difference = std::chrono::nanoseconds(_end - _start).count();
}
