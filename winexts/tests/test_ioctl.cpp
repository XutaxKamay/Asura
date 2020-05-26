#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

#include <dirent.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <linux/unistd.h>

#include "../communicate_structs.h"

// http://proswdev.blogspot.com/2012/02/get-process-id-by-name-in-linux-using-c.html
int pid_name(const std::string& procName)
{
    int pid = -1;

    // Open the /proc directory
    DIR* dp = opendir("/proc");
    if (dp != NULL)
    {
        // Enumerate all entries in directory until process found
        struct dirent* dirp;
        while (pid <= 0 && (dirp = readdir(dp)))
        {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
            if (id > 0)
            {
                // Read contents of virtual /proc/{pid}/cmdline file
                std::string cmdPath = std::string("/proc/")
                                      + std::string(dirp->d_name)
                                      + std::string("/cmdline");
                std::ifstream cmdFile(cmdPath.c_str());
                std::string cmdLine;
                std::getline(cmdFile, cmdLine);

                if (!cmdLine.empty())
                {
                    // Keep first cmdline item which contains the program
                    // path
                    size_t pos = cmdLine.find('\0');
                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(0, pos);

                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');

                    if (pos != std::string::npos)
                        cmdLine = cmdLine.substr(pos + 1);
                    // Compare against requested process name
                    if (procName == cmdLine)
                        pid = id;
                }
            }
        }
    }

    closedir(dp);

    return pid;
}

// https://stackoverflow.com/questions/25704455/how-to-check-if-memory-region-is-mapped-to-file
int is_mmaped(void* ptr, size_t length, int pid)
{
    std::string fmt("/proc/" + std::to_string(pid) + "/maps");

    FILE* file = fopen(fmt.c_str(), "r");
    char line[1024];
    int result = 0;

    while (!feof(file))
    {
        if (fgets(line, sizeof(line) / sizeof(char), file) == NULL)
        {
            break;
        }
        unsigned long start, end;
        if (sscanf(line, "%lx-%lx", &start, &end) != 2)
        {
            continue; // could not parse. fail gracefully and try again on
                      // the next line.
        }
        unsigned long ptri = (long)ptr;
        if (ptri >= start && ptri + length <= end)
        {
            result = 1;
            break;
        }
    }

    fclose(file);
    return result;
}

int main()
{
    int fd = open("/dev/winexts", O_RDWR);

    if (fd < 0)
    {
        printf("couldn't open /dev/winexts\n");
        return fd;
    }

    auto pid = pid_name("target");

    if (pid == -1)
    {
        close(fd);
        return fd;
    }

    unsigned char values[0x3000];
    unsigned char read_values[0x3000];
    uint64_t g_alloc_addr = 0x13370000;
    uint64_t STACK_SIZE   = 0x3000;

    /** Should kills itself but it is not.
     * mov rax, 39
     * syscall
     * mov rsi, 20
     * mov rdi, rax
     * mov rax, 62
     * syscall

     * mov rax, 39
     * syscall
     * mov rsi, 9
     * mov rdi, rax
     * mov rax, 62
     * syscall
     */

    uint8_t shellcode[] = { 0x48, 0xC7, 0xC7, 0x00, 0x00, 0x00,
                            0x00, 0x48, 0xC7, 0xC0, 0x3C, 0x00,
                            0x00, 0x00, 0x0F, 0x05 };
    // while (true)
    {
        if (!is_mmaped(reinterpret_cast<void*>(g_alloc_addr),
                       STACK_SIZE,
                       pid))
        {
            communicate_remote_mmap_t remote_mmap;
            remote_mmap.prot = PROT_EXEC | PROT_READ | C_PROT_MAY_EXEC
                               | C_PROT_MAY_READ | C_PROT_MAY_WRITE
                               | C_PROT_MAY_SHARE;
            remote_mmap.vm_remote_address = g_alloc_addr;
            remote_mmap.vm_size           = STACK_SIZE;
            remote_mmap.pid_target        = pid;

            auto error = (communicate_error_t)
              ioctl(fd, COMMUNICATE_CMD_REMOTE_MMAP, &remote_mmap);

            if (error != COMMUNICATE_ERROR_NONE)
            {
                printf("ouch mmap %i\n", error);
            }
            else
            {
                printf("mmap'd at %lX-%lX\n",
                       remote_mmap.ret,
                       remote_mmap.vm_remote_address);
            }

            communicate_write_t write_shellcode;
            write_shellcode.pid_target = pid;

            write_shellcode.vm_local_address  = (uintptr_t)shellcode;
            write_shellcode.vm_remote_address = g_alloc_addr;
            write_shellcode.vm_size           = sizeof(shellcode);

            error = (communicate_error_t)ioctl(fd,
                                               COMMUNICATE_CMD_WRITE,
                                               &write_shellcode);

            if (error != COMMUNICATE_ERROR_NONE)
            {
                printf("ouch write shellcode %i\n", error);
            }
            else
            {
                printf("wrote shellcode\n");
            }

            communicate_remote_mprotect_t communicate_remote_mprotect;

            communicate_remote_mprotect.pid_target   = pid;
            communicate_remote_mprotect.address      = g_alloc_addr;
            communicate_remote_mprotect.wanted_flags = PROT_EXEC
                                                       | PROT_READ
                                                       | PROT_WRITE;
            communicate_remote_mprotect.pkey = -1;
            communicate_remote_mprotect.size = STACK_SIZE;

            error = (communicate_error_t)ioctl(
              fd,
              COMMUNICATE_CMD_REMOTE_PROTECT,
              &communicate_remote_mprotect);

            if (error != COMMUNICATE_ERROR_NONE)
            {
                printf("ouch remote protect %i\n", error);
            }
            else
            {
                printf("remote protect done %i\n",
                       communicate_remote_mprotect.ret);
            }

            communicate_write_t write;

            memset(values, 0x33, sizeof(values));

            write.pid_target        = pid;
            write.vm_local_address  = (uintptr_t)values;
            write.vm_size           = sizeof(values);
            write.vm_remote_address = 0x555555755040;

            error = (communicate_error_t)ioctl(fd,
                                               COMMUNICATE_CMD_WRITE,
                                               &write);

            if (error != COMMUNICATE_ERROR_NONE)
            {
                printf("ouch write %i\n", error);
            }
            else
            {
                printf("test write\n");
            }

            communicate_read_t read;

            memset(read_values, 0x11, sizeof(read_values));

            read.pid_target        = pid;
            read.vm_local_address  = (uintptr_t)read_values;
            read.vm_size           = sizeof(read_values);
            read.vm_remote_address = 0x555555755040;

            error = (communicate_error_t)ioctl(fd,
                                               COMMUNICATE_CMD_READ,
                                               &read);

            if (error != COMMUNICATE_ERROR_NONE)
            {
                printf("ouch read %i\n", error);
            }
            else
            {
                printf("test read\n");
            }

            if (memcmp(values, read_values, sizeof(read_values)) == 0)
            {
                printf("success read/write large buffer\n");
            }

            communicate_list_vmas_t communicate_list_vmas;

            communicate_list_vmas.pid_target    = pid;
            communicate_list_vmas.vma_max_count = 128;
            communicate_list_vmas.vmas = (communicate_vma_t*)malloc(
              communicate_list_vmas.vma_max_count
              * sizeof(communicate_vma_t));

            printf("write listvmas %lX\n",
                   (uintptr_t)communicate_list_vmas.vmas);

            error = (communicate_error_t)ioctl(fd,
                                               COMMUNICATE_CMD_LIST_VMAS,
                                               &communicate_list_vmas);

            printf("%li\n", sizeof(communicate_vma_t));

            if (error != COMMUNICATE_ERROR_NONE)
            {
                printf("ouch listvmas %i\n", error);
            }
            else
            {
                for (int vma = 0; vma < communicate_list_vmas.vma_count;
                     vma++)
                {
                    printf("vma: %lX-%lX %s\n",
                           communicate_list_vmas.vmas[vma].vm_start,
                           communicate_list_vmas.vmas[vma].vm_end,
                           communicate_list_vmas.vmas[vma].vm_descriptor);
                }

                printf("done listvmas\n");
            }

            free(communicate_list_vmas.vmas);
        }

        communicate_remote_clone_t remote_clone;

        memset(&remote_clone, 0, sizeof(remote_clone));

        remote_clone.pid_target  = pid;
        remote_clone.regs_set.ip = true;
        remote_clone.regs.ip     = g_alloc_addr;
        remote_clone.flags       = (CLONE_VM | CLONE_FS | CLONE_FILES
                              | CLONE_PARENT);
        remote_clone.exit_signal = 0;
        remote_clone.stack       = g_alloc_addr + 0x2000;
        remote_clone.stack_size  = 0x1000;

        // while (true)
        {
            auto error = (communicate_error_t)
              ioctl(fd, COMMUNICATE_CMD_REMOTE_CLONE, &remote_clone);

            if (error != COMMUNICATE_ERROR_NONE)
            {
                printf("ouch clone %i\n", error);
            }
            else
            {
                printf("test clone %i\n", remote_clone.ret);
            }
        }

        /*communicate_remote_munmap_t remote_munmap;
        remote_munmap.pid_target        = pid;
        remote_munmap.vm_remote_address = g_alloc_addr;

        auto error = (communicate_error_t)
          ioctl(fd, COMMUNICATE_CMD_REMOTE_MUNMAP, &remote_munmap);

        if (error != COMMUNICATE_ERROR_NONE)
        {
            printf("ouch munmap %i\n", error);
        }
        else
        {
            printf("munmap\n");
        }*/
    }

    close(fd);

    return 0;
}
