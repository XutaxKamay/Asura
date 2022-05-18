project('xklib', 'cpp')

xklib_srcs = [
    'src/bits.cpp',
    'src/buffer.cpp',
    'src/circularbuffer.cpp',
    'src/decryptrsablocks.cpp',
    'src/detourx86.cpp',
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
    '-Wall'
]

args_dbg = [
    '-O0',
    '-g'
]
args_dbg += common_args

args_rel = [
    '-O3',
    '-fvisibility=hidden',
    '-fvisibility-inlines-hidden',
    '-frename-registers',
    '-fomit-frame-pointer',
    '-s'
]

args_rel += common_args

args_rel_native = [
    '-march=native',
    '-mtune=native'
]
args_rel_native += args_rel


deps = [
    dependency('libcryptopp')
]

library('xklib.32.dbg', sources : xklib_srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_dbg + '-m32', link_args : '-m32', dependencies : deps)
library('xklib.32.rel', sources : xklib_srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_rel + '-m32', link_args : '-m32', dependencies : deps)
library('xklib.32.rel.native', sources : xklib_srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_rel_native + '-m32', link_args : '-m32', dependencies : deps)

executable('test.32.dbg', sources : srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_dbg + '-m32', link_args : '-m32', dependencies : deps)
executable('test.32.rel', sources : srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_rel + '-m32', link_args : '-m32', dependencies : deps)
executable('test.32.rel.native', sources : srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_rel_native + '-m32', link_args : '-m32', dependencies : deps)


library('xklib.64.dbg', sources : xklib_srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_dbg, dependencies : deps)
library('xklib.64.rel', sources : xklib_srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_rel, dependencies : deps)
library('xklib.64.rel.native', sources : xklib_srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_rel_native, dependencies : deps)

executable('test.64.dbg', sources : srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_dbg, dependencies : deps)
executable('test.64.rel', sources : srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_rel, dependencies : deps)
executable('test.64.rel.native', sources : srcs, cpp_pch : 'src/pch.h', include_directories : include_dirs, cpp_args : args_rel_native, dependencies : deps)
