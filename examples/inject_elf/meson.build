project('inject_elf', 'cpp')

asura_srcs = [
    'src/Asura/src/bits.cpp',
    'src/Asura/src/buffer.cpp',
    'src/Asura/src/circularbuffer.cpp',
    'src/Asura/src/detourx86.cpp',
    'src/Asura/src/elf.cpp',
    'src/Asura/src/exception.cpp',
    'src/Asura/src/kokabiel.cpp',
    'src/Asura/src/memoryarea.cpp',
    'src/Asura/src/memorymap.cpp',
    'src/Asura/src/memoryutils.cpp',
    'src/Asura/src/networkreadbuffer.cpp',
    'src/Asura/src/networkwritebuffer.cpp',
    'src/Asura/src/offset.cpp',
    'src/Asura/src/osutils.cpp',
    'src/Asura/src/patternbyte.cpp',
    'src/Asura/src/patternscanning.cpp',
    'src/Asura/src/pe.cpp',
    'src/Asura/src/processbase.cpp',
    'src/Asura/src/process.cpp',
    'src/Asura/src/processmemoryarea.cpp',
    'src/Asura/src/processmemorymap.cpp',
    'src/Asura/src/readbuffer.cpp',
    'src/Asura/src/runnabletask.cpp',
    'src/Asura/src/simd.cpp',
    'src/Asura/src/task.cpp',
    'src/Asura/src/timer.cpp',
    'src/Asura/src/types.cpp',
    'src/Asura/src/writebuffer.cpp',
    'src/Asura/src/xkc.cpp',
    'src/Asura/src/asura.cpp'
]

inject_elf_srcs = [
    'src/inject_elf.cpp'
]

srcs = asura_srcs + inject_elf_srcs

incdirs = [
    'src/Asura/src/vendor/ELFIO',
    'src/Asura/src',
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

if get_option('native')
    common_args += ['-march=native', '-mtune=native']
endif

executable('inject_elf.out', sources : srcs, cpp_pch : 'src/pch.h', include_directories : incdirs, cpp_args : common_args)
