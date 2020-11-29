#include "buffer.h"

using namespace XLib;

std::string XLib::get_variable_type_str(typesize_t typeSize)
{
    switch (typeSize)
    {
        case (type_safesize):
            return "safesize (32 bits)";
        case (type_8s):
            return "8 bits signed";
        case (type_16s):
            return "16 bits signed";
        case (type_32s):
            return "32 bits signed";
        case (type_64s):
            return "64 bits signed";
        case (type_8us):
            return "8 bits unsigned";
        case (type_16us):
            return "16 bits unsigned";
        case (type_32us):
            return "32 bits unsigned";
        case (type_64us):
            return "64 bits unsigned";
        case (type_array):
            return "array";
        case (type_float):
            return "float";
        case (type_double):
            return "double";
        default:
            return "unknown";
    }
}
