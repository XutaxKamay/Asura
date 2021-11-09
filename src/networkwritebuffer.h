#ifndef NETWORKWRITEDBUFFER_H
#define NETWORKWRITEDBUFFER_H

#include "writebuffer.h"

namespace XKLib
{
    class NetworkWriteBuffer : public Buffer
    {
        public:
            NetworkWriteBuffer(data_t data             = nullptr,
                               std::size_t maxSize     = 0,
                               std::size_t writtenBits = 0);

            void writeBit(bool value);
            void pos(std::size_t toBit = 0);

            template <typesize_t typesize = type_array>
            auto writeVar(g_v_t<typesize> var)
            {
                if constexpr (typesize == type_array)
                {
                    static_assert(typesize != type_array,
                                  "Can't write as type_array");
                }
                else
                {
                    for (std::size_t i = 0;
                         i < sizeof(g_v_t<typesize>) * CHAR_BIT;
                         i++)
                    {
                        writeBit(var
                                     & (view_as<g_v_t<typesize>>(1)
                                        << view_as<g_v_t<typesize>>(i)) ?
                                   true :
                                   false);
                    }
                }
            }

        private:
            std::size_t _written_bits {};
    };
};

#endif
