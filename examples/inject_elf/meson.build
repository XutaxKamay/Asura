project('inject_elf', 'cpp')

xklib_srcs = [
    'src/XKLib/src/bits.cpp',
    'src/XKLib/src/buffer.cpp',
    'src/XKLib/src/circularbuffer.cpp',
    'src/XKLib/src/decryptrsablocks.cpp',
    'src/XKLib/src/detourx86.cpp',
    'src/XKLib/src/elf.cpp',
    'src/XKLib/src/encryptrsablocks.cpp',
    'src/XKLib/src/exception.cpp',
    'src/XKLib/src/hybridcrypto.cpp',
    'src/XKLib/src/kokabiel.cpp',
    'src/XKLib/src/memoryarea.cpp',
    'src/XKLib/src/memorymap.cpp',
    'src/XKLib/src/memoryutils.cpp',
    'src/XKLib/src/networkreadbuffer.cpp',
    'src/XKLib/src/networkwritebuffer.cpp',
    'src/XKLib/src/offset.cpp',
    'src/XKLib/src/osutils.cpp',
    'src/XKLib/src/patternbyte.cpp',
    'src/XKLib/src/patternscanning.cpp',
    'src/XKLib/src/pe.cpp',
    'src/XKLib/src/processbase.cpp',
    'src/XKLib/src/process.cpp',
    'src/XKLib/src/processmemoryarea.cpp',
    'src/XKLib/src/processmemorymap.cpp',
    'src/XKLib/src/readbuffer.cpp',
    'src/XKLib/src/rsablocks.cpp',
    'src/XKLib/src/runnabletask.cpp',
    'src/XKLib/src/simd.cpp',
    'src/XKLib/src/task.cpp',
    'src/XKLib/src/timer.cpp',
    'src/XKLib/src/types.cpp',
    'src/XKLib/src/writebuffer.cpp',
    'src/XKLib/src/xkc.cpp',
    'src/XKLib/src/xklib.cpp'
]

inject_elf_srcs = [
    'src/inject_elf.cpp'
]

srcs = xklib_srcs + inject_elf_srcs

incdirs = [
    'src/XKLib/src/vendor/ELFIO',
    'src/XKLib/src',
    'src'
]

common_args = [
    '-std=c++20',
    '-Wextra',
    '-W',
    '-Wall',
    '-Werror',
    '-Wno-unused-command-line-argument',
    '-Wno-ignored-optimization-argument'
]

deps = [
  dependency('libcryptopp')
]

if get_option('native')
    common_args += ['-march=native', '-mtune=native']
endif

executable('inject_elf.out', sources : srcs, cpp_pch : 'src/pch.h', include_directories : incdirs, cpp_args : common_args, dependencies : deps)
