#ifndef KOKABIEL_H
#define KOKABIEL_H

#include "process.h"
#include "processmemoryarea.h"

namespace XKLib
{
    template <unsigned char ELFClass>
    concept ELFClassSupported = ELFClass == ELFCLASS32
#ifndef ENVIRONMENT32
                                || ELFClass == ELFCLASS64
#endif
      ;

    /**
     * @brief Manual maps a statically link ELF into a process.
     * TODO:
     * Support dynamic libs.
     */
    class Kokabiel
    {
      private:
        template <typename T>
        struct Elf_auxv_t
        {
            T a_type; /* Entry type */
            union
            {
                T a_val; /* Integer value */
            } a_un;
        };

        struct loadable_segment_t
        {
            std::vector<byte_t> data;
            mapf_t flags;
            ptr_t start;
        };

      public:
        struct injection_info_t
        {
            struct
            {
                ptr_t base_image;
                ptr_t shellcode_address;
            } allocated_mem;

            uintptr_t offset_image;
            uintptr_t entry_point;
            uintptr_t stack_start;
            uintptr_t env_data;
            std::vector<loadable_segment_t> ready_segments;
        };

        Kokabiel(const std::string& fileName);

        template <std::size_t stack_size_T>
        auto inject(ProcessMemoryMap& processMemoryMap,
                    const std::vector<std::string>& cmdLine,
                    const std::vector<std::string>& env,
                    RunnableTask<stack_size_T>& runnableTask,
                    injection_info_t& injectionInfo) -> void;

      private:
        auto loadSegments() -> void;

        template <unsigned char ELFClass_T>
        requires(ELFClassSupported<ELFClass_T>) auto relocateSegments(
          ProcessMemoryMap& processMemoryMap,
          injection_info_t& injectionInfo) -> void;

        template <unsigned char ELFClass_T, std::size_t stack_size_T>
        requires(ELFClassSupported<ELFClass_T>) auto createEnv(
          ProcessMemoryMap& processMemoryMap,
          const std::vector<std::string>& cmdLine,
          const std::vector<std::string>& env,
          RunnableTask<stack_size_T>& runnableTask,
          injection_info_t& injectionInfo);

        template <unsigned char ELFClass_T, std::size_t stack_size_T>
        requires(ELFClassSupported<ELFClass_T>) auto createShellCode(
          ProcessMemoryMap& processMemoryMap,
          const std::vector<std::string>& cmdLine,
          RunnableTask<stack_size_T>& runnableTask,
          injection_info_t& injectionInfo) -> void;

      private:
        ELFIO::elfio _elf;
        std::vector<loadable_segment_t> _loaded_segments;
    };

    template <std::size_t stack_size_T>
    auto Kokabiel::inject(ProcessMemoryMap& processMemoryMap,
                          const std::vector<std::string>& cmdLine,
                          const std::vector<std::string>& env,
                          RunnableTask<stack_size_T>& runnableTask,
                          injection_info_t& injectionInfo) -> void
    {
        if (_elf.get_type() != ET_DYN && _elf.get_type() != ET_EXEC)
        {
            XKLIB_EXCEPTION("Elf must be dynamic library or "
                            "executable");
        }

        loadSegments();

        if (_elf.get_class() == ELFCLASS32)
        {
            relocateSegments<ELFCLASS32>(processMemoryMap, injectionInfo);

            createEnv<ELFCLASS32, stack_size_T>(processMemoryMap,
                                                cmdLine,
                                                env,
                                                runnableTask,
                                                injectionInfo);

            createShellCode<ELFCLASS32, stack_size_T>(processMemoryMap,
                                                      cmdLine,
                                                      runnableTask,
                                                      injectionInfo);
        }
#ifndef ENVIRONMENT32
        else
        {
            relocateSegments<ELFCLASS64>(processMemoryMap, injectionInfo);

            createEnv<ELFCLASS64, stack_size_T>(processMemoryMap,
                                                cmdLine,
                                                env,
                                                runnableTask,
                                                injectionInfo);

            createShellCode<ELFCLASS64, stack_size_T>(processMemoryMap,
                                                      cmdLine,
                                                      runnableTask,
                                                      injectionInfo);
        }
#endif
    }

    template <unsigned char ELFClass_T>
    requires(ELFClassSupported<ELFClass_T>) auto Kokabiel::relocateSegments(
      ProcessMemoryMap& processMemoryMap,
      injection_info_t& injectionInfo) -> void
    {
        constexpr auto _reloc_ptr = []()
        {
            if constexpr (ELFClass_T == ELFCLASS32)
            {
                return type_wrapper<uint32_t>;
            }
            else if constexpr (ELFClass_T == ELFCLASS64)
            {
                return type_wrapper<uint64_t>;
            }
        }();

        using reloc_ptr_t = typename decltype(_reloc_ptr)::type;

        /* Only copy previous segments for multi-threading */
        injectionInfo.ready_segments = _loaded_segments;

        /* first allocate a new base image */
        auto total_image_size = view_as<uintptr_t>(
                                  (injectionInfo.ready_segments.end() - 1)
                                    ->start)
                                + (injectionInfo.ready_segments.end() - 1)
                                    ->data.size()
                                - view_as<uintptr_t>(
                                  injectionInfo.ready_segments.begin()
                                    ->start);

        injectionInfo.allocated_mem.base_image = view_as<ptr_t>(0l);

        if (_elf.get_type() == ET_EXEC)
        {
            injectionInfo.allocated_mem.base_image = processMemoryMap
                                                       .allocArea(
                                                         injectionInfo
                                                           .ready_segments
                                                           .begin()
                                                           ->start,
                                                         total_image_size,
                                                         0);

            if (injectionInfo.allocated_mem.base_image
                != injectionInfo.ready_segments.begin()->start)
            {
                XKLIB_EXCEPTION("Could not allocate base image");
            }
        }
        else
        {
            injectionInfo.allocated_mem.base_image = processMemoryMap
                                                       .allocArea(
                                                         0,
                                                         total_image_size,
                                                         0);

            if (injectionInfo.allocated_mem.base_image == nullptr)
            {
                XKLIB_EXCEPTION("Could not allocate base image");
            }
        }

        /* Calculate offset between base image and new image */
        injectionInfo.offset_image = view_as<uintptr_t>(
                                       injectionInfo.allocated_mem
                                         .base_image)
                                     - view_as<uintptr_t>(
                                       injectionInfo.ready_segments
                                         .begin()
                                         ->start);

        /* Setup entry point */
        injectionInfo.entry_point = _elf.get_entry()
                                    + injectionInfo.offset_image;

        /* Relocate where the segments start */
        for (auto it = injectionInfo.ready_segments.begin();
             it != injectionInfo.ready_segments.end();
             it++)
        {
            *view_as<uintptr_t*>(&it->start) += injectionInfo.offset_image;
            processMemoryMap.forceWrite(it->start, it->data);
        }

        /* Relocate image if possible */
        for (auto&& section : _elf.sections)
        {
            auto sec_type = section->get_type();

            /* We care only about dynamic relocs */
            if (sec_type == SHT_RELA || sec_type == SHT_REL)
            {
                ELFIO::const_relocation_section_accessor rsa(_elf,
                                                             section);

                auto entry_count = rsa.get_entries_num();

                for (decltype(entry_count) i = 0; i < entry_count; i++)
                {
                    ELFIO::Elf64_Addr offset;
                    ELFIO::Elf64_Addr sym_val;
                    std::string sym_name;
                    unsigned char type;
                    ELFIO::Elf_Sxword addend;
                    ELFIO::Elf_Sxword calc_value;

                    rsa.get_entry(i,
                                  offset,
                                  sym_val,
                                  sym_name,
                                  type,
                                  addend,
                                  calc_value);

                    /* no reloc or run-time reloc */
                    if (calc_value == 0)
                    {
                        continue;
                    }

                    for (auto ready_seg : injectionInfo.ready_segments)
                    {
                        if (offset >= view_as<uintptr_t>(ready_seg.start)
                            && offset
                                 < (view_as<uintptr_t>(ready_seg.start)
                                    + ready_seg.data.size()))
                        {
                            auto rel_off = offset
                                           - view_as<uintptr_t>(
                                             ready_seg.start);

                            *view_as<reloc_ptr_t*>(&ready_seg.data[rel_off]) = calc_value
                                                                               + injectionInfo
                                                                                   .offset_image;
                        }
                    }
                }
            }
            /**
             * TODO: ?
             * Dynamic executables are not supported due to the fact
             * that a process could not have ld.so loaded. We might
             * end up loading ourselves at the end, though it is a
             * quite long task.
             */
            else if (sec_type == SHT_DYNSYM)
            {
                /**
                 * I don't know why (yet), but it according to ELF,
                 * there can be one useless smybol and section
                 * SHT_DYNSYM always exists
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

        for (auto ready_seg : injectionInfo.ready_segments)
        {
            processMemoryMap.protectMemoryArea(ready_seg.start,
                                               ready_seg.data.size(),
                                               ready_seg.flags);

            processMemoryMap.forceWrite(view_as<ptr_t>(ready_seg.start),
                                        ready_seg.data);
        }
    }

    template <unsigned char ELFClass_T, std::size_t stack_size_T>
    requires(ELFClassSupported<ELFClass_T>) auto Kokabiel::createEnv(
      ProcessMemoryMap& processMemoryMap,
      const std::vector<std::string>& cmdLine,
      const std::vector<std::string>& env,
      RunnableTask<stack_size_T>& runnableTask,
      injection_info_t& injectionInfo)
    {
        constexpr auto _reloc_ptr = []()
        {
            if constexpr (ELFClass_T == ELFCLASS32)
            {
                return type_wrapper<uint32_t>;
            }
            else if constexpr (ELFClass_T == ELFCLASS64)
            {
                return type_wrapper<uint64_t>;
            }
        }();

        using reloc_ptr_t = typename decltype(_reloc_ptr)::type;

        std::vector<reloc_ptr_t> cmds_offsets;
        std::vector<reloc_ptr_t> envs_offsets;
        bytes_t env_data;
        reloc_ptr_t total_offset = 0;

        /* traverse in reversed way */
        for (auto cmd = cmdLine.rbegin(); cmd != cmdLine.rend(); cmd++)
        {
            reloc_ptr_t offset;
            offset = total_offset;
            total_offset += cmd->size() + 1;

            env_data.insert(env_data.end(), cmd->begin(), cmd->end());
            env_data.push_back(0);

            cmds_offsets.push_back(offset);
        }

        /* cmds ended, now add env */
        for (auto e = env.rbegin(); e != env.rend(); e++)
        {
            reloc_ptr_t offset;
            offset = total_offset;
            total_offset += e->size() + 1;

            env_data.insert(env_data.end(), e->begin(), e->end());
            env_data.push_back(0);

            envs_offsets.push_back(offset);
        }

        injectionInfo.stack_start = view_as<uintptr_t>(
                                      runnableTask.baseStack())
                                    + stack_size_T;

        injectionInfo.env_data = view_as<uintptr_t>(
          processMemoryMap.allocArea(
            0,
            env_data.size() + MemoryUtils::GetPageSize(), /* env + aux
                                                             vecs */
            MemoryArea::ProtectionFlags::RW));

        if (injectionInfo.env_data == 0)
        {
            XKLIB_EXCEPTION("Couldn't allocate memory for cmd line "
                            "data");
        }

        /* write argv + envp */
        processMemoryMap.write(view_as<ptr_t>(injectionInfo.env_data),
                               env_data);

        auto at_random = injectionInfo.env_data + total_offset;

        /* let's generate some 16 random bytes for AT_RANDOM */
        static std::vector<byte_t> random_bytes = []
        {
            using random_bytes_engine = std::independent_bits_engine<
              std::default_random_engine,
              CHAR_BIT,
              byte_t>;

            random_bytes_engine rbe;
            std::vector<byte_t> data(16);
            std::generate(begin(data), end(data), std::ref(rbe));

            return data;
        }();

        processMemoryMap.write(*view_as<ptr_t*>(&at_random),
                               random_bytes);

        /* Setup auxiliary vectors */
        Elf_auxv_t<reloc_ptr_t> elf_aux[2] {
            {  AT_NULL,                                  { 0 }}, /* first because last in the stack */
            {AT_RANDOM, { *view_as<reloc_ptr_t*>(&at_random) }}
        };

        /* write aux vecs */
        for (std::size_t i = 0; i < 2; i++)
        {
            injectionInfo.stack_start -= sizeof(Elf_auxv_t<reloc_ptr_t>);
            processMemoryMap.write(view_as<ptr_t>(
                                     injectionInfo.stack_start),
                                   &elf_aux[i],
                                   sizeof(Elf_auxv_t<reloc_ptr_t>));
        }

        static reloc_ptr_t null_address = 0;
        /* write null address for limiting enp */
        injectionInfo.stack_start -= sizeof(reloc_ptr_t);
        processMemoryMap.write(view_as<ptr_t>(injectionInfo.stack_start),
                               &null_address,
                               sizeof(null_address));

        /**
         * Env exists ? if yes we write env addresss to stack after
         * the null addr
         */
        for (auto&& env_offset : envs_offsets)
        {
            injectionInfo.stack_start -= sizeof(reloc_ptr_t);

            auto address_of_string = view_as<reloc_ptr_t>(
              injectionInfo.env_data + env_offset);

            processMemoryMap.write(view_as<ptr_t>(
                                     injectionInfo.stack_start),
                                   &address_of_string,
                                   sizeof(address_of_string));
        }

        /* write null address for limiting argv */
        injectionInfo.stack_start -= sizeof(reloc_ptr_t);
        processMemoryMap.write(view_as<ptr_t>(injectionInfo.stack_start),
                               &null_address,
                               sizeof(null_address));

        for (auto&& cmd_offset : cmds_offsets)
        {
            injectionInfo.stack_start -= sizeof(reloc_ptr_t);

            auto address_of_string = view_as<reloc_ptr_t>(
              injectionInfo.env_data + cmd_offset);

            processMemoryMap.write(view_as<ptr_t>(
                                     injectionInfo.stack_start),
                                   &address_of_string,
                                   sizeof(address_of_string));
        }
    }

    template <unsigned char ELFClass_T, std::size_t stack_size_T>
    requires(ELFClassSupported<ELFClass_T>) auto Kokabiel::createShellCode(
      ProcessMemoryMap& processMemoryMap,
      const std::vector<std::string>& cmdLine,
      RunnableTask<stack_size_T>& runnableTask,
      injection_info_t& injectionInfo) -> void
    {
        constexpr auto _reloc_ptr = []()
        {
            if constexpr (ELFClass_T == ELFCLASS32)
            {
                return type_wrapper<uint32_t>;
            }
            else if constexpr (ELFClass_T == ELFCLASS64)
            {
                return type_wrapper<uint64_t>;
            }
        }();

        using reloc_ptr_t = typename decltype(_reloc_ptr)::type;
        std::vector<byte_t> shellcode;

#if defined(__x86_64__) || defined(__i386__)
        if constexpr (ELFClass_T == ELFCLASS64)
        {
            /**
             * "movabs rax, 0; mov rsp, rax; movabs rax, 0; push rax;
             * movabs rax, 0; jmp rax"
             */
            shellcode = { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x48, 0x89, 0xc4, 0x48, 0xb8, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50,
                          0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0xff, 0xe0 };

            /* inject shellcode for properly calling _start */
            *view_as<reloc_ptr_t*>(shellcode.data() + 26) = injectionInfo
                                                              .entry_point;

            /* prepare cmd line */
            *view_as<reloc_ptr_t*>(shellcode.data() + 15) = cmdLine.size();

            /* setup stack */
            *view_as<reloc_ptr_t*>(shellcode.data() + 2) = injectionInfo
                                                             .stack_start;
        }
        else if constexpr (ELFClass_T == ELFCLASS32)
        {
            /**
             * ""mov eax, 0; mov esp, eax; mov eax, 0; push eax; mov eax,
             * 0; jmp eax"
             */
            shellcode = { 0xb8, 0x00, 0x00, 0x00, 0x00, 0x89, 0xc4,
                          0xb8, 0x00, 0x00, 0x00, 0x00, 0x50, 0xb8,
                          0x00, 0x00, 0x00, 0x00, 0xff, 0xe0 };

            /* inject shellcode for properly calling _start */
            *view_as<reloc_ptr_t*>(shellcode.data() + 14) = injectionInfo
                                                              .entry_point;

            /* prepare cmd line */
            *view_as<reloc_ptr_t*>(shellcode.data() + 8) = cmdLine.size();

            /* setup stack */
            *view_as<reloc_ptr_t*>(shellcode.data() + 1) = injectionInfo
                                                             .stack_start;
        }
#else
    #error "Architecture not supported"
#endif

        /**
         * NOTE:
         * Well 32 bits injectors won't be able to inject in 64 bits
         * processes. process, due to syscalls on linux kernel, but this
         * is not a real limitation on Windows. Technically speaking, we
         * could use uint64_t for addresses as Windows supports it
         * (ReadProcessMemory64 etc.), but that would require some extra
         * code. On GNU/Linux it is a problem though and it doesn't seem
         * very compatible. To be honest Windows is much more nicely
         * designed in the kernel side than Linux, even if it's bloated
         * everywhere with user programs. That means that rmmap, rmunmap,
         * rclone, rmprotect & can't work for 64 bits processes, if our
         * own process is 32 bits.
         */

        injectionInfo.allocated_mem.shellcode_address = processMemoryMap.allocArea(
          0,
          shellcode.size(),
          MemoryArea::ProtectionFlags::RW);

        processMemoryMap.write(
          view_as<ptr_t>(injectionInfo.allocated_mem.shellcode_address),
          shellcode);

        processMemoryMap.protectMemoryArea(
          injectionInfo.allocated_mem.shellcode_address,
          shellcode.size(),
          MemoryArea::ProtectionFlags::RX);

        runnableTask.routineAddress() = injectionInfo.allocated_mem
                                          .shellcode_address;
    }
};

#endif
