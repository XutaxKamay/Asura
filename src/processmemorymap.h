#ifndef XKLIB_PROCESSMEMORYMAP_H
#define XKLIB_PROCESSMEMORYMAP_H

#include "memorymap.h"
#include "memoryutils.h"
#include "processbase.h"
#include "processmemoryarea.h"

namespace XKLib
{
    class ProcessMemoryArea;

    class ProcessMemoryMap : public MemoryMap<ProcessMemoryArea>
    {
      public:
        ProcessMemoryMap();
        explicit ProcessMemoryMap(ProcessBase process);

      public:
        auto refresh() -> void;

      public:
        auto read(const auto address, const std::size_t size) const
          -> bytes_t
        {
            return MemoryUtils::ReadProcessMemoryArea(_process_base.id(),
                                                      address,
                                                      size);
        }

        auto write(const auto address, const bytes_t& bytes) const -> void
        {
            MemoryUtils::WriteProcessMemoryArea(_process_base.id(),
                                                bytes,
                                                address);
        }

        auto write(const auto address,
                   const auto ptr,
                   const std::size_t size) const -> void
        {
            bytes_t data(view_as<byte_t*>(ptr),
                         view_as<byte_t*>(ptr) + size);

            write<decltype(address)>(address, data);
        }

        auto searchNearestEmptyArea(const auto address) const
        {
            if (_areas.size() == 0)
            {
                return address;
            }

            struct SimplifiedArea
            {
                std::uintptr_t begin;
                std::uintptr_t end;
            };

            std::vector<SimplifiedArea> simplified_areas;

            /**
             * Merge areas, even if they're not the same memory
             * protections.
             * Thanks to the operating system, the areas are already
             * in order.
             */
            std::size_t area_index = 0;

            while (area_index < _areas.size())
            {
                SimplifiedArea simplified_area;

                const auto first_area = _areas[area_index];
                const auto begin_ptr  = first_area->begin();
                auto end_ptr          = first_area->end();

                for (area_index = area_index + 1;
                     area_index < _areas.size();
                     area_index++)
                {
                    const auto next_area = _areas[area_index];

                    /**
                     * If begin ptr is the same as the previous end
                     * then affect the new end ptr and go on the next
                     * area
                     */
                    if (next_area->begin() == end_ptr)
                    {
                        end_ptr = next_area->end();
                    }
                    /**
                     * Else we break and find our newest non-merged
                     * area
                     */
                    else
                    {
                        break;
                    }
                }

                simplified_area.begin = begin_ptr;
                simplified_area.end   = end_ptr;
                simplified_areas.push_back(simplified_area);
            }

            /**
             * Now find the closest area we can get
             */
            std::uintptr_t start_ptr, end_ptr;

            for (const auto& simplified_area : simplified_areas)
            {
                start_ptr = simplified_area.begin;
                end_ptr   = simplified_area.end;

                if (view_as<std::uintptr_t>(address) >= start_ptr
                    and view_as<std::uintptr_t>(address) < end_ptr)
                {
                    break;
                }
            }

            const auto relative_address = view_as<std::uintptr_t>(address)
                                          - start_ptr;

            const auto area_size = end_ptr - start_ptr;

            std::uintptr_t closest_area = 0x0;

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

        auto search(const auto address) const
          -> std::shared_ptr<ProcessMemoryArea>
        {
            for (const auto& area : _areas)
            {
                const auto start_ptr = area->begin();
                const auto end_ptr   = area->end();

                if (view_as<std::uintptr_t>(address) >= start_ptr
                    and view_as<std::uintptr_t>(address) < end_ptr)
                {
                    return area;
                }
            }

            return nullptr;
        }

      public:
        auto allocArea(const auto address,
                       const std::size_t size,
                       const mapf_t flags) -> ptr_t
        {
            const auto ret = MemoryUtils::AllocArea(_process_base.id(),
                                                    address,
                                                    size,
                                                    flags);

            refresh();

            return ret;
        }

        auto freeArea(const auto address, std::size_t size) -> void
        {
            MemoryUtils::FreeArea(_process_base.id(), address, size);

            refresh();
        }

        auto protectMemoryArea(const auto address,
                               const std::size_t size,
                               const mapf_t flags) -> void
        {
            MemoryUtils::ProtectMemoryArea(_process_base.id(),
                                           address,
                                           size,
                                           flags);

            refresh();
        }

        auto forceWrite(const auto address, const bytes_t& bytes) -> void
        {
            refresh();

            const auto area = search(address);

            if (not area)
            {
                XKLIB_EXCEPTION("Could not find area");
            }

            const auto flags = area->protectionFlags().cachedValue();

            area->protectionFlags() = flags
                                      | MemoryArea::ProtectionFlags::W;

            write(address, bytes);

            area->protectionFlags() = flags;
        }

        auto forceWrite(const auto address,
                        const auto ptr,
                        std::size_t size) -> void
        {
            bytes_t data(view_as<byte_t*>(ptr),
                         view_as<byte_t*>(ptr) + size);

            forceWrite(address, data);
        }

      private:
        ProcessBase _process_base;
    };
}

#endif
