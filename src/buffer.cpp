#include "buffer.h"

std::string XLib::GetVariableTypeStr( typesize_t typeSize )
{
    if ( typeSize == type_safesize )
        return "safesize (32 bits)";
    else if ( typeSize == type_8 )
        return "8 bits";
    else if ( typeSize == type_16 )
        return "16 bits";
    else if ( typeSize == type_32 )
        return "32 bits";
    else if ( typeSize == type_64 )
        return "64 bits";
    else if ( typeSize == type_array )
        return "array";
    else if ( typeSize == type_float )
        return "float";
    else if ( typeSize == type_double )
        return "double";
    else
        return "unknown";
}
