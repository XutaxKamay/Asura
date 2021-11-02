## Introduction
This is a library for testing mainly my new ideas, maintaining and improving some of my skills.

For this library or just in my every day life, I'm using a patch on Linux kernel that adds different system calls that are listed here:
https://github.com/XutaxKamay/mylib/blob/master/src/custom_linux_syscalls.h

## Requirements
- GNU/Linux:
    - make
    - g++ / clang++

- For Windows you need MinGW with g++

## How to build
Clone the repo `git clone --recurse-submodules https://github.com/XutaxKamay/XKLib`.

Type `make -j$(nproc)` inside the root directory of the repository.

## How to use
- If you want to use the library, for GNU/Linux you'll likely need this Linux kernel patch and recompile your kernel:

### Warning
You might need to fix the patch to suit your kernel version (changing system calls numbers).
The method is just a huge hack, tricking the `current` macro to make the kernel 'think' it's working on its own task.
Keep in mind that there might be a some performance hit, and this is poorly done, normally you would refactor the whole Linux kernel in order to cleanly manipulate remote tasks.
```
https://xutaxkamay.com/syscalls.patch.gpg
gpg syscalls.patch.gpg
pass: yu9Ooya7
```

- Just include \<repo\>/src in your project and link the a library. (xklib.rel.a or xklib.dbg.a)

## Testing & Examples
There is tests and examples being done on my code at test/src and being compiled as xklib_test.rel / xklib_test.dbg
