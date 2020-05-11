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

unsigned char values[0x3000];
unsigned char read_values[0x3000];

int main()
{
    int fd = open("/dev/winexts_dev", 0);

    if (fd < 0)
    {
        printf("couldn't open /dev/winexts_dev\n");
        return fd;
    }

    communicate_remote_mmap_t remote_mmap;
    remote_mmap.fd                = -1;
    remote_mmap.prot              = PROT_EXEC | PROT_WRITE | PROT_READ;
    remote_mmap.flags             = MAP_PRIVATE | MAP_ANONYMOUS;
    remote_mmap.offset            = 0;
    remote_mmap.vm_remote_address = 0;
    remote_mmap.vm_size           = 4096;
    remote_mmap.pid_target        = pid_name("target");

    auto error = (communicate_error_t)ioctl(fd,
                                            COMMUNICATE_CMD_REMOTE_MMAP,
                                            &remote_mmap);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        printf("ouch mmap %i\n", error);
    }

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

    if (memcmp(values, read_values, sizeof(read_values)) == 0)
    {
        printf("success\n");
    }

    communicate_write_t write_shellcode;
    write_shellcode.pid_target = pid_name("target");

    uint8_t shellcode[]
      = { 0xCC, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x05 };

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

    //     sleep(30);

    /*communicate_remote_clone_t remote_clone;

    error = (communicate_error_t)ioctl(fd,
                                       COMMUNICATE_CMD_CLONE,
                                       &write_shellcode);

    if (error != COMMUNICATE_ERROR_NONE)
    {
        printf("ouch clone %i\n", error);
    }*/

    communicate_remote_munmap_t remote_munmap;
    remote_munmap.pid_target        = pid_name("target");
    remote_munmap.vm_remote_address = remote_mmap.ret;
    remote_munmap.vm_size           = 4096;

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
