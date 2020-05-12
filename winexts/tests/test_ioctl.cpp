#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "../communicate_structs.h"

using namespace std;

// http://proswdev.blogspot.com/2012/02/get-process-id-by-name-in-linux-using-c.html
int pid_name(string procName)
{
    int pid = -1;

    // Open the /proc directory
    DIR* dp = opendir("/proc");
    if (dp != NULL)
    {
        // Enumerate all entries in directory until process found
        struct dirent* dirp;
        while (pid < 0 && (dirp = readdir(dp)))
        {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
            if (id > 0)
            {
                // Read contents of virtual /proc/{pid}/cmdline file
                string cmdPath = string("/proc/") + dirp->d_name
                                 + "/cmdline";
                ifstream cmdFile(cmdPath.c_str());
                string cmdLine;
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty())
                {
                    // Keep first cmdline item which contains the program
                    // path
                    size_t pos = cmdLine.find('\0');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != string::npos)
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

static unsigned char values[0x3000];
static unsigned char read_values[0x3000];
static auto g_alloc_addr = (uint64_t)0x13370000;

int main()
{
    int fd = open("/dev/winexts", 0);

    if (fd < 0)
    {
        printf("couldn't open /dev/winexts\n");
        return fd;
    }

    communicate_remote_mmap_t remote_mmap;
    remote_mmap.fd                = -1;
    remote_mmap.prot              = PROT_EXEC | PROT_WRITE | PROT_READ;
    remote_mmap.flags             = MAP_PRIVATE | MAP_ANONYMOUS;
    remote_mmap.offset            = 0;
    remote_mmap.vm_remote_address = g_alloc_addr;
    remote_mmap.vm_size           = 0x3000;
    remote_mmap.pid_target        = pid_name("target");

    auto error = (communicate_error_t)ioctl(fd,
                                            COMMUNICATE_CMD_REMOTE_MMAP,
                                            &remote_mmap);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        printf("ouch mmap %i\n", error);
    }

    printf("mmap'd at %lX\n", remote_mmap.ret);

    getchar();

    communicate_write_t write;

    memset(values, 0x33, sizeof(values));

    write.pid_target        = pid_name("target");
    write.vm_local_address  = (uintptr_t)values;
    write.vm_size           = sizeof(values);
    write.vm_remote_address = 0x552ACAB040;

    error = (communicate_error_t)ioctl(fd, COMMUNICATE_CMD_WRITE, &write);

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

    read.pid_target        = pid_name("target");
    read.vm_local_address  = (uintptr_t)read_values;
    read.vm_size           = sizeof(read_values);
    read.vm_remote_address = 0x552ACAB040;

    error = (communicate_error_t)ioctl(fd, COMMUNICATE_CMD_READ, &read);

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
        printf("success\n");
    }

    getchar();

    communicate_write_t write_shellcode;
    write_shellcode.pid_target = pid_name("target");

    uint8_t shellcode[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xBB,
                            0x02, 0x00, 0x00, 0x00, 0xCD, 0x80 };

    write_shellcode.vm_local_address  = (uintptr_t)shellcode;
    write_shellcode.vm_remote_address = remote_mmap.ret;
    write_shellcode.vm_size           = sizeof(shellcode);

    error = (communicate_error_t)ioctl(fd,
                                       COMMUNICATE_CMD_WRITE,
                                       &write_shellcode);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        printf("ouch write %i\n", error);
    }
    else
    {
        printf("wrote shellcode\n");
    }

    //     sleep(30);

    getchar();

    write.vm_local_address  = (uint64_t)&g_alloc_addr;
    write.vm_size           = sizeof(g_alloc_addr);
    write.pid_target        = pid_name("target");
    write.vm_remote_address = g_alloc_addr + 0x2000 + 0x28;
    error = (communicate_error_t)ioctl(fd, COMMUNICATE_CMD_WRITE, &write);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        printf("ouch write %i\n", error);
    }
    else
    {
        printf("test write\n");
    }

    getchar();

    communicate_remote_clone_t remote_clone;

    remote_clone.flags        = CLONE_VM | CLONE_FS | CLONE_FILES;
    remote_clone.stack        = remote_mmap.ret + 0x2000;
    remote_clone.stack_size   = 4096;
    remote_clone.child_tid    = 0;
    remote_clone.pidfd        = 0;
    remote_clone.parent_tid   = 0;
    remote_clone.set_tid_size = 0;
    remote_clone.set_tid      = 0;
    remote_clone.tls          = 0;
    remote_clone.exit_signal  = 0;
    remote_clone.pid_target   = pid_name("target");

    error = (communicate_error_t)ioctl(fd,
                                       COMMUNICATE_CMD_REMOTE_CLONE,
                                       &remote_clone);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        printf("ouch clone %i\n", error);
    }
    else
    {
        printf("clone %i\n", remote_clone.ret);
    }

    getchar();

    communicate_remote_munmap_t remote_munmap;
    remote_munmap.pid_target        = pid_name("target");
    remote_munmap.vm_remote_address = g_alloc_addr;
    remote_munmap.vm_size           = 0x3000;

    error = (communicate_error_t)ioctl(fd,
                                       COMMUNICATE_CMD_REMOTE_MUNMAP,
                                       &remote_munmap);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        printf("ouch munmap %i\n", error);
    }

    close(fd);

    return 0;
}
