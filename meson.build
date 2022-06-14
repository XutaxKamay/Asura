project('xklib', 'cpp')

xklib_srcs = [
    'src/bits.cpp',
    'src/buffer.cpp',
    'src/circularbuffer.cpp',
    'src/decryptrsablocks.cpp',
    'src/detourx86.cpp',
    'src/elf.cpp',
    'src/encryptrsablocks.cpp',
    'src/exception.cpp',
    'src/hybridcrypto.cpp',
    'src/kokabiel.cpp',
    'src/memoryarea.cpp',
    'src/memorymap.cpp',
    'src/memoryutils.cpp',
    'src/networkreadbuffer.cpp',
    'src/networkwritebuffer.cpp',
    'src/offset.cpp',
    'src/osutils.cpp',
    'src/patternbyte.cpp',
    'src/patternscanning.cpp',
    'src/pe.cpp',
    'src/processbase.cpp',
    'src/process.cpp',
    'src/processmemoryarea.cpp',
    'src/processmemorymap.cpp',
    'src/readbuffer.cpp',
    'src/rsablocks.cpp',
    'src/runnabletask.cpp',
    'src/simd.cpp',
    'src/task.cpp',
    'src/timer.cpp',
    'src/types.cpp',
    'src/writebuffer.cpp',
    'src/xkc.cpp',
    'src/xklib.cpp'
]

test_srcs = [
    'src/main.cpp',
    'src/test.cpp'
]

srcs = xklib_srcs + test_srcs

include_dirs = [
    'src/vendor/ELFIO',
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

library('xklib', sources : xklib_srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : common_args, dependencies : deps)

executable('test.out', sources : srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : common_args, dependencies : deps)
