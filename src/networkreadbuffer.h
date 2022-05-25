#ifndef XKLIB_NETWORKREADBUFFER_H
#define XKLIB_NETWORKREADBUFFER_H

#include "readbuffer.h"

namespace XKLib
{
    class NetworkReadBuffer : public Buffer
    {
      public:
        NetworkReadBuffer(data_t data          = nullptr,
                          std::size_t maxSize  = 0,
                          std::size_t readBits = 0);

        bool readBit();
        void pos(std::size_t toBit = 0);

        template <TypeSize T = type_array>
        auto readVar()
        {
            if constexpr (T == type_array)
            {
                static_assert(T != type_array,
                              "Can't read as type_array");
            }
            else
            {
                g_v_t<T> var {};

                for (std::size_t i = 0; i < sizeof(g_v_t<T>) * CHAR_BIT;
                     i++)
                {
                    if (readBit())
                    {
                        var += view_as<g_v_t<T>>(1)
                               << view_as<g_v_t<T>>(i);
                    }
                }

                return var;
            }
        }

      private:
        std::size_t _read_bits {};
    };
}

#endif
