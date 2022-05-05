#ifndef TIMER_H
#define TIMER_H

#include "types.h"

namespace XKLib
{
    template <typename T>
    concept floating_point = std::is_same<float, T>::value || std::
      is_same<double, T>::value;

    class Timer
    {
      public:
        auto nanos() const -> std::uint16_t;
        auto micros() const -> std::uint16_t;
        auto millis() const -> std::uint16_t;
        auto seconds() const -> std::uint64_t;
        auto difference() const -> std::uint64_t;

      public:
        auto start() -> void;
        auto end() -> void;

      private:
        std::chrono::high_resolution_clock::time_point _start;
        std::chrono::high_resolution_clock::time_point _end;
        std::uint64_t _difference;
    };
};

#endif
