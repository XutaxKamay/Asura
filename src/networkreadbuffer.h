#ifndef NETWORKREADBUFFER_H
#define NETWORKREADBUFFER_H

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

            template <typesize_t typesize = type_array>
            auto readVar()
            {
                if constexpr (typesize == type_array)
                {
                    static_assert(typesize != type_array,
                                  "Can't read as type_array");
                }
                else
                {
                    g_v_t<typesize> var {};

                    for (std::size_t i = 0;
                         i < sizeof(g_v_t<typesize>) * CHAR_BIT;
                         i++)
                    {
                        if (readBit())
                        {
                            var += view_as<g_v_t<typesize>>(1)
                                   << view_as<g_v_t<typesize>>(i);
                        }
                    }

                    return var;
                }
            }

        private:
            std::size_t _read_bits {};
    };
};

#endif
