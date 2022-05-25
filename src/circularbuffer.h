#ifndef XKLIB_CIRCULARBUFFER_H
#define XKLIB_CIRCULARBUFFER_H

#include "types.h"

namespace XKLib
{
    template <typename T, std::size_t N>
    class CircularBuffer
    {
      public:
        using pT = T*;

        enum PushType
        {
            Filling,
            Updating
        };

      public:
        auto get(const int wantedSlot = 0) const;

      public:
        auto push(const T element);

      private:
        auto _slot(const int wantedSlot = 0) const;

      private:
        T _buffer[N] {};
        int _filled_history {};
        int _index {};
    };

    template <typename T, std::size_t N>
    auto CircularBuffer<T, N>::push(const T element)
    {
        /**
         * If it has been filled,
         * we update the index/slot of the first element.
         */
        if (_filled_history >= N)
        {
            _buffer[_index] = element;

            if (_index >= (N - 1))
            {
                _index = 0;
            }
            else
            {
                _index++;
            }

            return Updating;
        }

        _buffer[_filled_history] = element;

        /**
         * Don't forget to keep track
         * of the index/slot of the first element.
         */
        _index = _filled_history;
        _filled_history++;

        return Filling;
    }

    template <typename T, std::size_t N>
    auto CircularBuffer<T, N>::get(const int wantedSlot) const
    {
        const auto real_slot = _slot(wantedSlot);

        return (real_slot == -1) ? nullptr : &_buffer[real_slot];
    }

    template <typename T, std::size_t N>
    auto CircularBuffer<T, N>::_slot(const int wantedSlot) const
    {
        if (wantedSlot >= _filled_history or wantedSlot < 0
            or _filled_history <= 0)
        {
            return -1;
        }

        /* Check if the history has been filled yet */
        if (_filled_history >= N)
        {
            const int calc_slot = (_index - wantedSlot);

            /**
             * If it's under 0 it means that we need to rollback
             * in order to get the real slot
             */
            return (calc_slot < 0) ? calc_slot + N : calc_slot;
        }
        else
        {
            /**
             * If it has not been filled yet completely, we just
             * return our slot.
             */
            return (_filled_history - 1) - wantedSlot;
        }
    }
}

#endif // CIRCULARBUFFER_H
