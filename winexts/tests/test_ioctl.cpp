#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
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

int main()
{
    int fd = open("/dev/winexts_dev", 0);

    if (fd < 0)
    {
        printf("couldn't open /dev/winexts_dev\n");
        return fd;
    }

    communicate_write_t write;

    int value               = 1337;
    write.pid_target        = pid_name("target");
    write.vm_local_address  = (uintptr_t)&value;
    write.vm_size           = sizeof(uintptr_t);
    write.vm_remote_address = 0x552acab014;

    communicate_error_t error
      = (communicate_error_t)ioctl(fd, COMMUNICATE_CMD_WRITE, &write);

    close(fd);

    return 0;
}
