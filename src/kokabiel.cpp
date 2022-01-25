#include "pch.h"

#include "exception.h"

#include "kokabiel.h"

XKLib::Kokabiel::Kokabiel(const std::string& fileName)
{
    if (!_elf.load(fileName))
    {
        XKLIB_EXCEPTION("Couldn't load " + fileName);
    }
}

auto XKLib::Kokabiel::inject(XKLib::Process& process,
                             const std::vector<std::string>& cmdLine,
                             const std::vector<std::string>& env) -> void
{
    auto elf_type = _elf.get_type();

    if (elf_type != ET_DYN && elf_type != ET_EXEC)
    {
        XKLIB_EXCEPTION("Elf must be dynamic library or executable");
    }

    std::vector<ready_segment_t> ready_segments;

    /* copy segments */
    for (auto&& segment : _elf.segments)
    {
        auto seg_type = segment->get_type();

        if (seg_type == PT_LOAD)
        {
            ready_segment_t ready_segment;

            ready_segment.start = view_as<ptr_t>(
              MemoryUtils::align(segment->get_virtual_address(),
                                 MemoryUtils::GetPageSize()));

            auto left_over = segment->get_virtual_address()
                             - view_as<uintptr_t>(ready_segment.start);

            auto memory_aligned_size = MemoryUtils::align_to_page_size(
              segment->get_memory_size() + left_over,
              MemoryUtils::GetPageSize());

            ready_segment.data.resize(memory_aligned_size);

            std::copy(segment->get_data(),
                      segment->get_data() + segment->get_file_size(),
                      ready_segment.data.begin() + left_over);

            auto seg_flags = segment->get_flags();

            ready_segment.flags = ((seg_flags & PF_R) ?
                                     MemoryArea::ProtectionFlags::R :
                                     0)
                                  | ((seg_flags & PF_W) ?
                                       MemoryArea::ProtectionFlags::W :
                                       0)
                                  | ((seg_flags & PF_X) ?
                                       MemoryArea::ProtectionFlags::X :
                                       0);

            ready_segments.push_back(ready_segment);
        }
    }

    if (ready_segments.empty())
    {
        XKLIB_EXCEPTION("No loadable segments inside the elf file");
    }

    /* sort segments */
    std::sort(ready_segments.begin(),
              ready_segments.end(),
              [](const ready_segment_t& rs1, const ready_segment_t& rs2)
              {
                  return view_as<uintptr_t>(rs1.start)
                         < view_as<uintptr_t>(rs2.start);
              });

    auto mmap = process.mmap();

    auto total_image_size = view_as<uintptr_t>(
                              (ready_segments.end() - 1)->start)
                            + (ready_segments.end() - 1)->data.size()
                            - view_as<uintptr_t>(
                              ready_segments.begin()->start);

    auto base_image = view_as<ptr_t>(0l);

    if (elf_type == ET_EXEC)
    {
        base_image = mmap.allocArea(ready_segments[0].start,
                                    total_image_size,
                                    0);

        if (base_image != ready_segments[0].start)
        {
            XKLIB_EXCEPTION("Could not allocate base image");
        }
    }
    else
    {
        base_image = mmap.allocArea(0, total_image_size, 0);

        if (base_image == nullptr)
        {
            XKLIB_EXCEPTION("Could not allocate base image");
        }
    }

    auto offset_image = view_as<intptr_t>(base_image)
                        - view_as<intptr_t>(ready_segments[0].start);

    auto entry_point = _elf.get_entry() + offset_image;

    for (auto it = ready_segments.begin(); it != ready_segments.end();
         it++)
    {
        std::cout << "b seg: " << std::hex << it->start << " "
                  << view_as<uintptr_t>(it->start) + it->data.size()
                  << std::endl;

        *view_as<intptr_t*>(&it->start) += offset_image;

        std::cout << "a seg: " << std::hex << it->start << " "
                  << view_as<uintptr_t>(it->start) + it->data.size()
                  << std::endl;

        mmap.forceWrite(it->start, it->data);
    }

    /* Relocate image if possible */
    for (auto&& section : _elf.sections)
    {
        auto sec_type = section->get_type();

        /* We care only about dynamic relocs */
        if (sec_type == SHT_RELA || sec_type == SHT_REL)
        {
            ELFIO::const_relocation_section_accessor rsa(_elf, section);

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

                std::cout << std::dec << view_as<int>(i) << std::hex
                          << " " << offset << " " << addend << " "
                          << calc_value << std::endl;

                /* no reloc or run-time reloc */
                if (calc_value == 0)
                {
                    continue;
                }

                for (auto ready_seg : ready_segments)
                {
                    if (offset >= view_as<uintptr_t>(ready_seg.start)
                        && offset < (view_as<uintptr_t>(ready_seg.start)
                                     + ready_seg.data.size()))
                    {
                        auto rel_off = offset
                                       - view_as<uintptr_t>(
                                         ready_seg.start);

                        if (_elf.get_class() == ELFCLASS32)
                        {
                            *view_as<uint32_t*>(&ready_seg.data[rel_off]) = calc_value
                                                                            + offset_image;
                        }
                        else
                        {
                            *view_as<uint64_t*>(&ready_seg.data[rel_off]) = calc_value
                                                                            + offset_image;
                        }
                    }
                }
            }
        }
        /**
         * TODO: ?
         * Dynamic executables are not supported due to the fact that
         * a process could not have ld.so loaded.
         * We might end up loading ourselves at the end, though it is a
         * quite long task.
         */
        else if (sec_type == SHT_DYNSYM)
        {
            /**
             * I don't know why (yet), but it according to ELF, there can
             * be one useless smybol and section SHT_DYNSYM always exists
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

    for (auto ready_seg : ready_segments)
    {
        std::cout << "protecting seg: " << std::hex << ready_seg.start
                  << std::endl;

        mmap.protectMemoryArea(ready_seg.start,
                               ready_seg.data.size(),
                               ready_seg.flags);
    }

    /**
     * "movabs rax, 0; mov rsp, rax; movabs rax, 0; push rax; movabs rax,
     * 0; jmp rax"
     */
    std::vector<byte_t> shellcode = {
        0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x48, 0x89, 0xc4, 0x48, 0xb8, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x48, 0xb8, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0
    };

    auto shellcode_address = mmap.allocArea(
      0,
      sizeof(shellcode),
      MemoryArea::ProtectionFlags::RWX);

    /* inject shellcode for properly calling _start */
    *view_as<uintptr_t*>(shellcode.data() + 26) = entry_point;

    /* prepare cmd line */
    *view_as<uintptr_t*>(shellcode.data() + 15) = cmdLine.size();

    mmap.protectMemoryArea(shellcode_address,
                           sizeof(shellcode),
                           MemoryArea::ProtectionFlags::RX);

    auto task = process.createTask(shellcode_address);

    struct cmd_offset_t
    {
        std::size_t offset;
    };

    struct env_offset_t
    {
        std::size_t offset;
    };

    std::vector<cmd_offset_t> cmds_offsets;
    std::vector<env_offset_t> envs_offsets;
    bytes_t env_data;
    std::size_t total_offset = 0;

    /* traverse in reversed way */
    for (auto cmd = cmdLine.rbegin(); cmd != cmdLine.rend(); cmd++)
    {
        cmd_offset_t co;
        co.offset = total_offset;
        total_offset += cmd->size() + 1;

        env_data.insert(env_data.end(), cmd->begin(), cmd->end());
        env_data.push_back(0);

        cmds_offsets.push_back(co);
    }

    /* cmds ended, now add env */
    for (auto e = env.rbegin(); e != env.rend(); e++)
    {
        env_offset_t eo;
        eo.offset = total_offset;
        total_offset += e->size() + 1;

        env_data.insert(env_data.end(), e->begin(), e->end());
        env_data.push_back(0);

        envs_offsets.push_back(eo);
    }

    auto stack_start = view_as<uintptr_t>(task.base_stack)
                       + Process::TASK_STACK_SIZE;

    auto allocated_env_data = mmap.allocArea(
      0,
      env_data.size() + MemoryUtils::GetPageSize(), /* env + fun */
      MemoryArea::ProtectionFlags::RW);

    if (allocated_env_data == nullptr)
    {
        XKLIB_EXCEPTION("Couldn't allocate memory for cmd line data");
    }

    /* write argv + envp */
    mmap.write(allocated_env_data, env_data);

    auto at_random = view_as<uintptr_t>(allocated_env_data)
                     + total_offset;

    /* let's generate some 16 random bytes for AT_RANDOM */
    static std::vector<byte_t> random_bytes = []{

        using random_bytes_engine = std::independent_bits_engine<
        std::default_random_engine, CHAR_BIT, byte_t>;

        random_bytes_engine rbe;
        std::vector<byte_t> data(16);
        std::generate(begin(data), end(data), std::ref(rbe));

        return data;
    }();

    mmap.write(at_random, random_bytes);

    /* Setup auxiliary vectors */
    Elf_auxv_t<uintptr_t> elf_aux[2] {
        {  AT_NULL,         { 0 }}, /* first because last in the stack */
        {AT_RANDOM, { at_random }}
    };

    /* write aux vecs */
    for (std::size_t i = 0; i < 2; i++)
    {
        stack_start -= sizeof(Elf_auxv_t<uintptr_t>);
        mmap.write(stack_start,
                   &elf_aux[i],
                   sizeof(Elf_auxv_t<uintptr_t>));
    }

    static uintptr_t null_address = 0;
    /* write null address for limiting enp */
    stack_start -= sizeof(ptr_t);
    mmap.write(stack_start, &null_address, sizeof(null_address));

    /**
     * Env exists ? if yes we write env addresss to stack after the null
     * addr
     */
    for (auto&& env_offset : envs_offsets)
    {
        stack_start -= sizeof(ptr_t);

        auto address_of_string = view_as<uintptr_t>(allocated_env_data)
                                 + env_offset.offset;

        mmap.write(stack_start,
                   &address_of_string,
                   sizeof(address_of_string));
    }

    /* write null address for limiting argv */
    stack_start -= sizeof(ptr_t);
    mmap.write(stack_start, &null_address, sizeof(null_address));

    for (auto&& cmd_offset : cmds_offsets)
    {
        stack_start -= sizeof(ptr_t);

        auto address_of_string = view_as<uintptr_t>(allocated_env_data)
                                 + cmd_offset.offset;

        mmap.write(stack_start,
                   &address_of_string,
                   sizeof(address_of_string));
    }

    *view_as<uintptr_t*>(shellcode.data() + 2) = stack_start;

    mmap.forceWrite(shellcode_address, shellcode);

    std::cout << "running" << std::endl;
    task.run();

    std::cout << "waiting" << std::endl;

    task.wait();
    std::cout << "ended" << std::endl;
}
