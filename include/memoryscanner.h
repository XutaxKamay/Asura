#ifndef MEMORYSCANNER_H
#define MEMORYSCANNER_H

#include "types.h"
#include "memorymap.h"

namespace XLib
{
    /**
     * @brief MemoryScanner
     * Permits to scan the memory and retrieve offsets, memory pointers etc.
     */
    class MemoryScanner
    {
        /* Static methods */
      public:
        /**
         * @brief Get
         *
         * @return XLib::MemoryScanner*
         */
        static auto Get();

        /* Public methods */
      public:
        /**
         * @brief fetchMaps
         * Refresh/fetch the memory maps from the process.
         */
        auto fetchMaps() -> void;
        /**
         * @brief maps
         *
         * @return auto
         */
        auto maps();
        /**
         * @brief setMaps
         *
         * @param maps Replace the maps inside the memory scanner by this
         * parameter
         * @return auto
         */
        auto setMaps( const std::vector< map_t >& maps );

      private:
        /**
         * @brief _maps
         * Contains vector of the memory maps inside the process
         */
        std::vector< map_t > _maps {};
        /**
         * @brief _pid
         * Process ID
         */
        uint32_t _pid {};

    } extern* g_pMemoryScanner;
}

class Car
{
  public:
    std::string get_color() const;
};

#endif // MEMORYSCANNER_H
