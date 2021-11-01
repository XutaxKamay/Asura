#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include "types.h"

namespace XLib
{
    template <typename T, size_t max_history_T>
    /**
     * @brief The CircularBuffer class
     * Simple circular buffer with having in mind the best performance.
     */
    class CircularBuffer
    {
      public:
        using pT = T*;

        enum push_type_t
        {
            Filling,
            Updating
        };

      public:
        /**
         * @brief push
         * @param element
         * @return push_type_t
         * Pushes your element as the first element into the history
         * buffer.
         */
        auto push(T element);
        /**
         * @brief get
         * @param wantedSlot
         * @return int
         * Get the desired element from the history buffer from a
         * index/slot.
         */
        auto get(int wantedSlot = 0);

      private:
        /**
         * @brief _slot
         * @param wantedSlot
         * @return int
         */
        auto _slot(int wantedSlot = 0);

      private:
        /**
         * @brief _buffer
         * Buffer to the elements that will construct history.
         */
        T _buffer[max_history_T] {};
        /**
         * @brief _filled_history
         * Counts the elements filled into history.
         */
        int _filled_history {};
        /**
         * @brief _index
         * The index/slot of the first element into history.
         */
        int _index {};
    };

    template <typename T, size_t max_history_T>
    auto CircularBuffer<T, max_history_T>::push(T element)
    {
        /**
         * If it has been filled,
         * we update the index/slot of the first element.
         */
        if (_filled_history >= max_history_T)
        {
            _buffer[_index] = element;

            if (_index >= (max_history_T - 1))
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

    template <typename T, size_t max_history_T>
    auto CircularBuffer<T, max_history_T>::get(int wantedSlot)
    {
        auto real_slot = _slot(wantedSlot);

        return (real_slot == -1) ? nullptr : &_buffer[real_slot];
    }

    template <typename T, size_t max_history_T>
    auto CircularBuffer<T, max_history_T>::_slot(int wantedSlot)
    {
        if (wantedSlot >= _filled_history || wantedSlot < 0
            || _filled_history <= 0)
        {
            return -1;
        }

        /* Check if the history has been filled yet */
        if (_filled_history >= max_history_T)
        {
            int calc_slot = (_index - wantedSlot);

            /**
             * If it's under 0 it means that we need to rollback
             * in order to get the real slot
             */
            return (calc_slot < 0) ? calc_slot + max_history_T :
                                     calc_slot;
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
} // namespace XLib

#endif // CIRCULARBUFFER_H
