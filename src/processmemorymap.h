#ifndef PROCESSMEMORYMAP_H
#define PROCESSMEMORYMAP_H

#include "memorymap.h"
#include "memoryutils.h"
#include "processbase.h"
#include "processmemoryarea.h"

namespace XLib
{
    class ProcessMemoryArea;
    class ProcessMemoryMap : public MemoryMap<ProcessMemoryArea>
    {
      public:
        ProcessMemoryMap(ProcessBase process);

      public:
        auto refresh() -> void;

      public:
        template <typename T = uintptr_t>
        auto searchNearestEmptyArea(T address)
        {
            if (_areas.size() == 0)
            {
                return address;
            }

            struct simplified_area_t
            {
                uintptr_t begin;
                uintptr_t end;
            };

            std::vector<simplified_area_t> simplified_areas;

            /**
             * Merge areas, even if they're not the same memory
             * protections.
             * Thanks to the operating system, the areas are already in
             * order.
             */
            size_t area_index = 0;

            while (area_index < _areas.size())
            {
                simplified_area_t simplified_area;

                auto first_area = _areas[area_index];
                auto begin_ptr  = first_area->begin();
                auto end_ptr    = first_area->end();

                for (area_index = area_index + 1;
                     area_index < _areas.size();
                     area_index++)
                {
                    auto next_area = _areas[area_index];

                    /**
                     * If begin ptr is the same as the previous end
                     * then affect the new end ptr and go on the next area
                     */
                    if (next_area->begin() == end_ptr)
                    {
                        end_ptr = next_area->end();
                    }
                    /**
                     * Else we break and find our newest non-merged area
                     */
                    else
                    {
                        break;
                    }
                }

                simplified_area.begin = begin_ptr;
                simplified_area.end   = end_ptr;
                simplified_areas.push(simplified_area);
            }

            /**
             * Now find the closest area we can get
             */
            uintptr_t start_ptr, end_ptr;

            for (auto&& simplified_area : simplified_areas)
            {
                start_ptr = simplified_area.begin;
                end_ptr   = simplified_area.end;

                if (view_as<uintptr_t>(address) >= start_ptr
                    && view_as<uintptr_t>(address) < end_ptr)
                {
                    break;
                }
            }

            auto relative_address = view_as<uintptr_t>(address)
                                    - start_ptr;

            auto area_size = end_ptr - start_ptr;

            uintptr_t closest_area = 0x0;

            if (relative_address > (area_size / 2))
            {
                closest_area = end_ptr;
            }
            else
            {
                closest_area = start_ptr - MemoryUtils::GetPageSize();
            }

            return closest_area;
        }

        template <typename T = uintptr_t>
        auto search(T address) -> std::shared_ptr<ProcessMemoryArea>
        {
            for (auto&& area : _areas)
            {
                auto start_ptr = area->begin();
                auto end_ptr   = area->end();

                if (view_as<uintptr_t>(address) >= start_ptr
                    && view_as<uintptr_t>(address) < end_ptr)
                {
                    return area;
                }
            }

            return nullptr;
        }

        template <typename T = uintptr_t>
        auto allocArea(T address, size_t size, mapf_t flags) -> ptr_t
        {
            auto ret = MemoryUtils::AllocArea(_process_base.id(),
                                              address,
                                              size,
                                              flags);

            refresh();

            return ret;
        }

        template <typename T = uintptr_t>
        auto freeArea(T address, size_t size) -> void
        {
            MemoryUtils::FreeArea(_process_base.id(), address, size);

            refresh();
        }

        template <typename T = uintptr_t>
        auto protectMemoryArea(T address, size_t size, mapf_t flags)
          -> void
        {
            MemoryUtils::ProtectMemoryArea(_process_base.id(),
                                           address,
                                           size,
                                           flags);

            refresh();
        }

        template <typename T = uintptr_t>
        auto read(T address, size_t size) -> bytes_t
        {
            return MemoryUtils::ReadProcessMemoryArea(_process_base.id(),
                                                      address,
                                                      size);
        }

        template <typename T = uintptr_t>
        auto write(T address, bytes_t bytes) -> void
        {
            MemoryUtils::WriteProcessMemoryArea(_process_base.id(),
                                                bytes,
                                                address);
        }

        template <typename T = uintptr_t>
        auto forceWrite(T address, bytes_t bytes) -> void
        {
            refresh();

            auto area = search(address);

            if (!area)
            {
                XLIB_EXCEPTION("Could not find area");
            }

            area->protectionFlags() |= MemoryArea::ProtectionFlags::W;

            MemoryUtils::WriteProcessMemoryArea(_process_base.id(),
                                                address,
                                                bytes);

            area->resetToDefaultFlags();
        }

      private:
        ProcessBase _process_base;
    };
};

#endif
