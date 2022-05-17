## Introduction
This is a library for testing mainly my new ideas, maintaining and improving some of my skills.

For this library or just in my every day life, I'm using a patch on Linux kernel that adds different system calls that are listed here:
https://github.com/XutaxKamay/mylib/blob/master/src/custom_linux_syscalls.h

## Requirements
- GNU/Linux:
    - meson
    - g++ / clang++
    - CryptoPP

- For Windows you need MinGW with g++

## How to build
Clone the repo `git clone --recurse-submodules https://github.com/XutaxKamay/XKLib`.

Type `meson build;cd build:meson compile` inside the root directory of the repository.

## How to use

- Just include \<repo\>/src in your project and link the a library. (xklib.rel.a or xklib.dbg.a)

### Warning!!!
If you want to use the library, for GNU/Linux you'll likely need this Linux kernel patch and recompile your kernel:
```
https://xutaxkamay.com/syscalls.patch.gpg
gpg syscalls.patch.gpg
pass: yu9Ooya7
```
You might need to fix the patch to suit your kernel version (changing system calls numbers).
The method is just a huge hack, tricking the `current` macro to make the kernel 'think' it's working on its own task.

Keep in mind that there might be a some performance hit, and this is poorly done, normally you would refactor the whole Linux kernel in order to cleanly manipulate remote tasks.

## What about using ptrace?
ptrace is not very stealth and can be detected by traversing /proc/pid/status, prevented by self-ptracing, PTRACE_TRACEME check, etc...
Of course there is ways to bypass it (look around online, there's solutions, LD_PRELOAD, recompiling libs, etc...)

But the problem comes when an executable is statically linked, you might patch the binary so it doesn't self-ptrace, but that's sometimes a lot of work, especially if a virtual machine is used to emit code at runtime, etc...
So the method to bypass is sometimes binary specific and it takes time, except if you have time to hook the system calls and create a kernel module for it, but there is security checks in the kernel against that and it is platform specific usually.

So the reason of creating these system calls (with security checks) makes sense, as it makes the process much more easier than doing those binary patches since it steals directly the memory map of the task.

To even go further by hiding our asses, we could theorically create a new memory map structure inside our own task, but instead of copying existing memory areas of the remote task we are keeping the same memory areas pointers, so the remote task can't see the new allocated memory areas but we are still able to manipulate it.

The other easier method is to add a special dirty flag to the memory area so it hides a bit the new allocated memory area from the remote task (/proc/pid/maps), but it is clearly not a proper solution.

## Testing & Examples
There is tests and examples being done on my code at test/src and being compiled as xklib_test.rel / xklib_test.dbg
