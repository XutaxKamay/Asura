#ifndef XKLIB_KOKABIEL_H
#define XKLIB_KOKABIEL_H

#include "memoryarea.h"
#include "process.h"
#include "processmemoryarea.h"

/* Expand ELFIO */
namespace ELFIO
{
    constexpr auto AT_NULL   = 0;
    constexpr auto AT_RANDOM = 25;

    template <typename T>
    struct Elf_auxv
    {
        T a_type; /* Entry type */

        union
        {
            T a_val; /* Integer value */
        } a_un;
    };

};

namespace XKLib
{
    template <unsigned char E>
    concept ELFClassSupported = E == ELFIO::ELFCLASS32
#ifndef ENVIRONMENT32
                                or E == ELFIO::ELFCLASS64
#endif
      ;

    /**
     * Manual maps a statically link ELF into a process.
     * TODO:
     * Support dynamic libs.
     */
    class Kokabiel
    {
      private:
        struct MemoryArea
        {
            bytes_t bytes;
            mapf_t flags;
            std::uintptr_t start;
        };

      public:
        struct InjectionInfo
        {
            struct
            {
                MemoryArea shellcode;
                MemoryArea env_data;
            } allocated_mem;

            std::uintptr_t offset_image;
            std::uintptr_t entry_point;
            std::uintptr_t stack_start;
            std::vector<MemoryArea> loaded_segments;
            ProcessMemoryMap process_memory_map;
        };

        enum class arch
        {
            X86
        };

        Kokabiel(const std::string& fileName);

        template <std::size_t N, arch A>
        auto inject(ProcessMemoryMap& processMemoryMap,
                    const std::vector<std::string>& cmdLine,
                    const std::vector<std::string>& env,
                    RunnableTask<N>& runnableTask,
                    InjectionInfo& injectionInfo) const -> void;

        auto freeInjection(InjectionInfo& injectionInfo) const -> void;

      private:
        auto loadSegments() -> void;

        template <unsigned char E>
        requires(ELFClassSupported<E>) auto relocateSegments(
          InjectionInfo& injectionInfo) const -> void;

        template <unsigned char E, std::size_t N>
        requires(ELFClassSupported<E>) auto createEnv(
          const std::vector<std::string>& cmdLine,
          const std::vector<std::string>& env,
          RunnableTask<N>& runnableTask,
          InjectionInfo& injectionInfo) const;

        template <unsigned char E, std::size_t N, arch A>
        requires(ELFClassSupported<E>) auto createShellCode(
          const std::vector<std::string>& cmdLine,
          RunnableTask<N>& runnableTask,
          InjectionInfo& injectionInfo) const -> void;

      private:
        ELFIO::elfio _elf;
        std::vector<MemoryArea> _loadable_segments;
        std::size_t _image_size;
    };

    template <std::size_t N, Kokabiel::arch A>
    auto Kokabiel::inject(ProcessMemoryMap& processMemoryMap,
                          const std::vector<std::string>& cmdLine,
                          const std::vector<std::string>& env,
                          RunnableTask<N>& runnableTask,
                          InjectionInfo& injectionInfo) const -> void
    {
        if (_elf.get_type() != ELFIO::ET_DYN
            and _elf.get_type() != ELFIO::ET_EXEC)
        {
            XKLIB_EXCEPTION("Elf must be dynamic library or "
                            "executable");
        }

        injectionInfo.process_memory_map = processMemoryMap;

        if (_elf.get_class() == ELFIO::ELFCLASS32)
        {
            relocateSegments<ELFIO::ELFCLASS32>(injectionInfo);

            createEnv<ELFIO::ELFCLASS32, N>(cmdLine,
                                            env,
                                            runnableTask,
                                            injectionInfo);

            createShellCode<ELFIO::ELFCLASS32, N, A>(cmdLine,
                                                     runnableTask,
                                                     injectionInfo);
        }
#ifndef ENVIRONMENT32
        else
        {
            relocateSegments<ELFIO::ELFCLASS64>(injectionInfo);

            createEnv<ELFIO::ELFCLASS64, N>(cmdLine,
                                            env,
                                            runnableTask,
                                            injectionInfo);

            createShellCode<ELFIO::ELFCLASS64, N, A>(cmdLine,
                                                     runnableTask,
                                                     injectionInfo);
        }
#endif
    }

    template <unsigned char E>

    requires(ELFClassSupported<E>) auto Kokabiel::relocateSegments(
      InjectionInfo& injectionInfo) const -> void
    {
        /* Only copy previous segments for multi-threading */
        injectionInfo.loaded_segments = _loadable_segments;

        std::uintptr_t image_base = 0;

        if (_elf.get_type() == ELFIO::ET_EXEC)
        {
            image_base = view_as<std::uintptr_t>(
              injectionInfo.process_memory_map.allocArea(
                injectionInfo.loaded_segments.begin()->start,
                _image_size,
                XKLib::MemoryArea::ProtectionFlags::RW));

            if (image_base == 0
                or (image_base
                    != injectionInfo.loaded_segments.begin()->start))
            {
                XKLIB_EXCEPTION("Could not allocate image");
            }
        }
        else
        {
            image_base = view_as<std::uintptr_t>(
              injectionInfo.process_memory_map.allocArea(
                0,
                _image_size,
                XKLib::MemoryArea::ProtectionFlags::RW));

            if (image_base == 0)
            {
                XKLIB_EXCEPTION("Could not allocate image");
            }
        }

        /* Calculate offset between base image and new image */
        injectionInfo.offset_image = image_base
                                     - _loadable_segments.begin()->start;

        for (auto&& segment : injectionInfo.loaded_segments)
        {
            segment.start += injectionInfo.offset_image;
        }

        /* Setup entry point */
        injectionInfo.entry_point = _elf.get_entry()
                                    + injectionInfo.offset_image;

        /* Relocate image if possible */
        for (const auto& section : _elf.sections)
        {
            const auto sec_type = section->get_type();

            /**
             * TODO: ?
             * Dynamic executables are not supported due to the fact
             * that a process could not have ld.so loaded. We might
             * end up loading ourselves at the end, though it is a
             * quite long task.
             */
            if (sec_type == ELFIO::SHT_DYNSYM)
            {
                /**
                 * I don't know why (yet), but it according to ELF,
                 * there can be one useless smybol and section
                 * ELFIO::SHT_DYNSYM always exists
                 * ... Does not happen with static executables though.
                 */
                if (ELFIO::const_symbol_section_accessor(_elf, section)
                      .get_symbols_num()
                    > 1)
                {
                    XKLIB_EXCEPTION("Should not get any dynamic symbols "
                                    "inside the elf, it is not supported "
                                    "yet.");
                }
            }
        }

        for (const auto& segment : injectionInfo.loaded_segments)
        {
            injectionInfo.process_memory_map.write(segment.start,
                                                   segment.bytes);
            injectionInfo.process_memory_map.protectMemoryArea(
              segment.start,
              segment.bytes.size(),
              segment.flags);
        }
    }

    template <unsigned char E, std::size_t N>

    requires(ELFClassSupported<E>) auto Kokabiel::createEnv(
      const std::vector<std::string>& cmdLine,
      const std::vector<std::string>& env,
      RunnableTask<N>& runnableTask,
      InjectionInfo& injectionInfo) const
    {
        constexpr auto _reloc_ptr = []()
        {
            if constexpr (E == ELFIO::ELFCLASS32)
            {
                return type_wrapper<std::uint32_t>;
            }
            else if constexpr (E == ELFIO::ELFCLASS64)
            {
                return type_wrapper<std::uint64_t>;
            }
        }();

        using reloc_ptr_t = typename decltype(_reloc_ptr)::type;

        std::vector<reloc_ptr_t> cmds_offsets;
        std::vector<reloc_ptr_t> envs_offsets;
        reloc_ptr_t total_offset = 0;

        /* traverse in reversed way */
        for (auto cmd = cmdLine.rbegin(); cmd != cmdLine.rend(); cmd++)
        {
            reloc_ptr_t offset = total_offset;
            total_offset += cmd->size() + 1;

            injectionInfo.allocated_mem.env_data.bytes.insert(
              injectionInfo.allocated_mem.env_data.bytes.end(),
              cmd->begin(),
              cmd->end());
            injectionInfo.allocated_mem.env_data.bytes.push_back(0);

            cmds_offsets.push_back(offset);
        }

        /* cmds ended, now add env */
        for (auto e = env.rbegin(); e != env.rend(); e++)
        {
            reloc_ptr_t offset = total_offset;
            total_offset += e->size() + 1;

            injectionInfo.allocated_mem.env_data.bytes.insert(
              injectionInfo.allocated_mem.env_data.bytes.end(),
              e->begin(),
              e->end());
            injectionInfo.allocated_mem.env_data.bytes.push_back(0);

            envs_offsets.push_back(offset);
        }

        injectionInfo.stack_start = view_as<std::uintptr_t>(
                                      runnableTask.baseStack())
                                    + N;

        injectionInfo.allocated_mem.env_data.start = view_as<
          std::uintptr_t>(injectionInfo.process_memory_map.allocArea(
          nullptr,
          injectionInfo.allocated_mem.env_data.bytes.size()
            + MemoryUtils::GetPageSize(),
          XKLib::MemoryArea::ProtectionFlags::RW));

        if (injectionInfo.allocated_mem.env_data.start == 0)
        {
            XKLIB_EXCEPTION("Couldn't allocate memory for cmd line "
                            "data");
        }

        /* write argv + envp */
        injectionInfo.process_memory_map.write(
          view_as<ptr_t>(injectionInfo.allocated_mem.env_data.start),
          injectionInfo.allocated_mem.env_data.bytes);

        const auto at_random = injectionInfo.allocated_mem.env_data.start
                               + total_offset;

        /* let's generate some 16 random bytes for AT_RANDOM */
        const std::vector<byte_t> random_bytes = []
        {
            using random_bytes_engine = std::independent_bits_engine<
              std::default_random_engine,
              CHAR_BIT,
              byte_t>;

            random_bytes_engine rbe;
            std::vector<byte_t> data(16);
            std::generate(data.begin(), data.end(), std::ref(rbe));

            return data;
        }();

        injectionInfo.process_memory_map.write(
          *view_as<ptr_t*>(&at_random),
          random_bytes);

        /* Setup auxiliary vectors */
        const ELFIO::Elf_auxv<reloc_ptr_t> elf_aux[2] {
            {  ELFIO::AT_NULL,                                  { 0 }},
            {ELFIO::AT_RANDOM, { *view_as<reloc_ptr_t*>(&at_random) }}
        };

        /* write aux vecs */
        for (std::size_t i = 0; i < 2; i++)
        {
            injectionInfo.stack_start -= sizeof(
              ELFIO::Elf_auxv<reloc_ptr_t>);

            injectionInfo.process_memory_map.write(
              view_as<ptr_t>(injectionInfo.stack_start),
              &elf_aux[i],
              sizeof(ELFIO::Elf_auxv<reloc_ptr_t>));
        }

        const static reloc_ptr_t null_address = 0;

        /* write null address for limiting enp */
        injectionInfo.stack_start -= sizeof(reloc_ptr_t);

        injectionInfo.process_memory_map.write(
          view_as<ptr_t>(injectionInfo.stack_start),
          &null_address,
          sizeof(null_address));

        /**
         * Env exists ? if yes we write env addresss to stack after
         * the null addr
         */
        for (const auto& env_offset : envs_offsets)
        {
            injectionInfo.stack_start -= sizeof(reloc_ptr_t);

            const auto address_of_string = view_as<reloc_ptr_t>(
              injectionInfo.allocated_mem.env_data.start + env_offset);

            injectionInfo.process_memory_map.write(
              view_as<ptr_t>(injectionInfo.stack_start),
              &address_of_string,
              sizeof(address_of_string));
        }

        /* write null address for limiting argv */
        injectionInfo.stack_start -= sizeof(reloc_ptr_t);

        injectionInfo.process_memory_map.write(
          view_as<ptr_t>(injectionInfo.stack_start),
          &null_address,
          sizeof(null_address));

        for (const auto& cmd_offset : cmds_offsets)
        {
            injectionInfo.stack_start -= sizeof(reloc_ptr_t);

            const auto address_of_string = view_as<reloc_ptr_t>(
              injectionInfo.allocated_mem.env_data.start + cmd_offset);

            injectionInfo.process_memory_map.write(
              view_as<ptr_t>(injectionInfo.stack_start),
              &address_of_string,
              sizeof(address_of_string));
        }
    }

    template <unsigned char E, std::size_t N, Kokabiel::arch A>

    requires(ELFClassSupported<E>) auto Kokabiel::createShellCode(
      const std::vector<std::string>& cmdLine,
      RunnableTask<N>& runnableTask,
      InjectionInfo& injectionInfo) const -> void
    {
        constexpr auto _reloc_ptr = []()
        {
            if constexpr (E == ELFIO::ELFCLASS32)
            {
                return type_wrapper<std::uint32_t>;
            }
            else if constexpr (E == ELFIO::ELFCLASS64)
            {
                return type_wrapper<std::uint64_t>;
            }
        }();

        using reloc_ptr_t = typename decltype(_reloc_ptr)::type;

        if constexpr (A == arch::X86)
        {
            if constexpr (E == ELFIO::ELFCLASS64)
            {
                /**
                 * "movabs rax, 0; mov rsp, rax; movabs rax, 0; push rax;
                 * movabs rax, 0; xor rdx, rdx; jmp rax"
                 */
                injectionInfo.allocated_mem.shellcode.bytes = {
                    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x48, 0x89, 0xc4, 0x48, 0xb8, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50,
                    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x48, 0x31, 0xd2, 0xff, 0xe0
                };

                /* inject shellcode for properly calling _start */
                *view_as<reloc_ptr_t*>(
                  &injectionInfo.allocated_mem.shellcode.bytes[26])
                  = injectionInfo.entry_point;

                /* prepare cmd line */
                *view_as<reloc_ptr_t*>(
                  &injectionInfo.allocated_mem.shellcode.bytes[15])
                  = cmdLine.size();

                /* setup stack */
                *view_as<reloc_ptr_t*>(
                  &injectionInfo.allocated_mem.shellcode.bytes[2])
                  = injectionInfo.stack_start;
            }
            else if constexpr (E == ELFIO::ELFCLASS32)
            {
                /**
                 * "mov eax, 0; mov esp, eax; mov eax, 0; push eax; mov
                 * eax, 0; xor edx, edx; jmp eax"
                 */
                injectionInfo.allocated_mem.shellcode.bytes = {
                    0xb8, 0x00, 0x00, 0x00, 0x00, 0x89, 0xc4, 0xb8,
                    0x00, 0x00, 0x00, 0x00, 0x50, 0xb8, 0x00, 0x00,
                    0x00, 0x00, 0x31, 0xd2, 0xff, 0xe0
                };

                /* inject shellcode for properly calling _start */
                *view_as<reloc_ptr_t*>(
                  &injectionInfo.allocated_mem.shellcode.bytes[14])
                  = injectionInfo.entry_point;

                /* prepare cmd line */
                *view_as<reloc_ptr_t*>(
                  &injectionInfo.allocated_mem.shellcode.bytes[8])
                  = cmdLine.size();

                /* setup stack */
                *view_as<reloc_ptr_t*>(
                  &injectionInfo.allocated_mem.shellcode.bytes[1])
                  = injectionInfo.stack_start;
            }
        }

        /**
         * NOTE:
         * Well 32 bits injectors won't be able to inject in 64 bits
         * processes. process, due to syscalls on linux kernel, but this
         * is not a real limitation on Windows. Technically speaking, we
         * could use std::uint64_t for addresses as Windows supports it
         * (ReadProcessMemory64 etc.), but that would require some extra
         * code. On GNU/Linux it is a problem though and it doesn't seem
         * very compatible. To be honest Windows is much more nicely
         * designed in the kernel side than Linux, even if it's bloated
         * everywhere with user programs. That means that rmmap, rmunmap,
         * rclone, rmprotect & can't work for 64 bits processes, if our
         * own process is 32 bits.
         */

        injectionInfo.allocated_mem.shellcode.start = view_as<
          std::uintptr_t>(injectionInfo.process_memory_map.allocArea(
          nullptr,
          injectionInfo.allocated_mem.shellcode.bytes.size(),
          XKLib::MemoryArea::ProtectionFlags::RW));

        injectionInfo.process_memory_map.write(
          view_as<ptr_t>(injectionInfo.allocated_mem.shellcode.start),
          injectionInfo.allocated_mem.shellcode.bytes);

        injectionInfo.process_memory_map.protectMemoryArea(
          injectionInfo.allocated_mem.shellcode.start,
          injectionInfo.allocated_mem.shellcode.bytes.size(),
          XKLib::MemoryArea::ProtectionFlags::RX);

        runnableTask.routineAddress() = view_as<ptr_t>(
          injectionInfo.allocated_mem.shellcode.start);
    }
}

#endif
