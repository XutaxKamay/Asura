#ifndef NETWORKWRITEDBUFFER_H
#define NETWORKWRITEDBUFFER_H

#include "writebuffer.h"

namespace XLib
{
    class NetworkWriteBuffer : public Buffer
    {
      public:
        NetworkWriteBuffer(data_t data            = nullptr,
                           safesize_t maxSize     = 0,
                           safesize_t writtenBits = 0);

        void writeBit(bool value);
        void pos(safesize_t toBit = 0);

        template <typesize_t typesize = type_array>
        auto writeVar(g_v_t<typesize> var)
        {
            if constexpr (typesize == type_array)
            {
                static_assert(typesize != type_array, "Can't write as type_array");
            }
            else
            {
                for (size_t i = 0; i < sizeof(g_v_t<typesize>) * CHAR_BIT;
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
        safesize_t _written_bits {};
    };
};

#endif
