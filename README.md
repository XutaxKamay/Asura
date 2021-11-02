## Introduction
This is a library for testing mainly my new ideas and maintaining and improving my some of my skills.

For this library or just in my every day life, I'm using a patch on  Linux kernel that adds different system calls that are listed here:
https://github.com/XutaxKamay/mylib/blob/master/src/custom_linux_syscalls.h

## Requirements
- GNU/Linux:
    - make
    - g++ / clang++

- For Windows you need MinGW with g++

## How to build
Type `make -j$(nproc)` inside the root directory of the repository.

## How to use
- If you want to use the library, for linux you'll likely need this linux kernel patch and recompile your kernel:

```
https://xutaxkamay.com/syscalls.patch.gpg
gpg syscalls.patch.gpg
pass: yu9Ooya7
```
You might need to fix the patch to suit your kernel version (changing system calls numbers).

- Just include <repo>/src in your project and link the a library. (xklib.rel.a or xklib.dbg.a)

