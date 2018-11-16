# Copyright 2014 The Ostrich / by Itamar O
# Copyright 2018 cmheia
# pylint: disable=bad-whitespace

"""SCons site config script"""

import os

from ext_lib import ExtLib
from site_utils import module_dirs_generator, join_path

# Directory for build process outputs (object files etc.)
_BUILD_BASE = ''
# Directory where binary programs are installed in (under $build_base/$flavor)
_BIN_SUBDIR = 'bin'

SDKBINDIR = 'bin'

# List of cached modules to save processing for second call and beyond
_CACHED_MODULES = list()

_DIR_SKIP_LIST = []
_MAX_DEPTH = 7

TOOLCHAIN_PREFIX = 'arm-none-eabi-'

CPU_CORE_NAME = 'cortex-m3'
CPU_ARM_THUMB_MODE = 'thumb'
FLOAT_ABI = 'soft'
FPU_TYPE = 'auto'

CFLAGS = [
    # Specifies the name of the target ARM processor.
    '-mcpu=' + CPU_CORE_NAME,
    # Select between generating code that executes in ARM and Thumb states.
    '-m' + CPU_ARM_THUMB_MODE,
    # Generate code that supports calling between
    # the ARM and Thumb instruction sets.
    # '-mthumb-interwork',
    # Generate code for the specified ABI.
    # Procedure Call Standard for the ARM Architecture (AAPCS)
    '-mabi=aapcs',
    # Specifies which floating-point ABI to use.
    # Permissible values are: ‘soft’,‘softfp’ and ‘hard’.
    # '-mfloat-abi=' + FLOAT_ABI,
    # This specifies what floating-point hardware (or hardware emulation) is
    # available on the target.
    # '-mfpu=' + FPU_TYPE,
    # Enables (or  disables) reading and writing of 16- and 32- bit values
    # from addresses that are not 16- or 32- bit aligned.
    # '-mno-unaligned-access',
    # Don’t recognize built-in functions that
    # do not begin with ‘__builtin_’ as prefix.
    '-fno-builtin',
    # Warn whenever a local variable or non-constant static variable
    # is unused aside from its declaration.
    # This warning is enabled by ‘-Wall’.
    # '-Wno-unused-variable',
    # Warn whenever a static function is declared but not defined
    # or a non-inline static function is unused.
    # This warning is enabled by ‘-Wall’.
    # '-Wno-unused-function',
    '-Wall',
    # Let the type char be signed, like signed char.
    '-fsigned-char',
    # Place each function or data item into its own section
    # in the output file if the target supports arbitrary sections.
    '-ffunction-sections',
    '-fdata-sections',
    # Specify the standard to which the code should conform.
    # The 2011 C standard plus GNU extensions.
    '-std=gnu11',
    # Assembler Listing. Remove to reduce build time
    '-Wa,-adhlns="${TARGET.base}.lst"',
    # Try to format error messages so that they fit on lines of
    # about n characters. The default is 72 characters for g++
    # and 0 for the rest of the front ends supported by GCC.
    # If n is zero, then no line-wrapping is done;
    # each error message appears on a single line.
    '-fmessage-length=0',
    # Produce debugging information for use by GDB.
    # This means to use the most expressive format available
    # (DWARF 2, stabs, or the native format if neither
    # of those are supported), including GDB extensions if at all possible.
    '-ggdb',
    # Level 3 includes extra information, such as all the macro definitions
    # present in the program.
    # Some debuggers support macro expansion when you use ‘-g3’.
    # '-g3',
    '-MMD',
    '-MP',
    # To use the link-time optimizer, ‘-flto’ needs to be
    # specified at compile time and during the final link.
    # For example:
    # gcc -c -O2 -flto foo.c
    # gcc -c -O2 -flto bar.c
    # gcc -o myprog -flto -O2 foo.o bar.o
    # '-flto',
]

ASFLAGS = CFLAGS + ['$_CPPDEFFLAGS']

LINKERSCRIPT = 'ld/w600.ld'

LINKFLAGS = CFLAGS + [
    '-T' + LINKERSCRIPT,
    # Enable garbage collection of unused input sections.
    '-Wl,--gc-sections',
    # '-Wl,--print-gc-sections',
    # To use the link-time optimizer, ‘-flto’ needs to be
    # specified at compile time and during the final link.
    # '-flto',
    # Do not use the standard system startup files when linking.
    '-nostartfiles',
    # Output a cross reference table.
    '-Wl,--cref',
    # Print a link map to the file mapfile.
    '-Wl,-Map,${TARGET.base}.map',
]

DEFAULT_LIBS = [
    'airkiss',
    'demo',
    'app',
    'common',
    'drivers',
    'network',
    'os',
    'sys',
    'boot',
]


def modules():
    """Generate modules to build.

    Each module is a directory with a SConscript file.
    """
    if not _CACHED_MODULES:
        # Build the cache
        build_dirs = [ENV_OVERRIDES[fla]['BUILDROOT'] for fla in flavors()]
        # print('build_dirs: {}'.format(build_dirs))

        def build_dir_skipper(dirpath):
            """Return True if `dirpath` is the build base dir."""
            return (os.path.normpath(_BUILD_BASE) == os.path.normpath(dirpath)) or \
                (os.path.normpath(dirpath) in build_dirs)

        def hidden_dir_skipper(dirpath):
            """Return True if `dirpath` last dir component begins with '.'"""
            last_dir = os.path.basename(dirpath)
            return last_dir.startswith('.')
        for module_path in module_dirs_generator(
                max_depth=_MAX_DEPTH, followlinks=False,
                dir_skip_list=[build_dir_skipper,
                               hidden_dir_skipper] + _DIR_SKIP_LIST,
                file_skip_list='.noscons'):
            _CACHED_MODULES.append(module_path)
    # Yield modules from cache
    for module in _CACHED_MODULES:
        yield module


ENV_DEFAULT_OPTIONS = {
    'TOOLS': ['as', 'gcc', 'g++', 'ar', 'gnulink'],
    'ENV': {'PATH': os.environ['PATH']},
}

# Dictionary of flavor-specific settings that should override values
#  from the base environment (using env.Replace).
# `_common` is reserved for settings that apply to the base env.
ENV_OVERRIDES = {
    '_common': dict(
        # Use gcc by default
        # TOOLS=['as', 'gcc', 'g++', 'ar', 'gnulink'],
        # ENV = {'PATH' : os.environ['PATH']},
        CC=TOOLCHAIN_PREFIX + 'gcc',
        AR=TOOLCHAIN_PREFIX + 'ar',
        RANLIB=TOOLCHAIN_PREFIX + 'ranlib',
        AS=TOOLCHAIN_PREFIX + 'gcc -c',
        CXX=TOOLCHAIN_PREFIX + 'g++',
        OBJCOPY=TOOLCHAIN_PREFIX + 'objcopy',
        OBJDUMP=TOOLCHAIN_PREFIX + 'objdump',
        SIZE=TOOLCHAIN_PREFIX + 'size',
        # CFLAGS=CFLAGS,
        CCFLAGS=CFLAGS,
        ASFLAGS=ASFLAGS,
        LIBPATH='#lib',
        PROGSUFFIX='.elf',
        # Path for installed binary programs
        BINDIR=os.path.join('$BUILDROOT', _BIN_SUBDIR),
        LINKFLAGS=LINKFLAGS,
        LINKCOM='$LINK -o $TARGET $LINKFLAGS $__RPATH '
        '-Wl,--start-group $SOURCES $_LIBDIRFLAGS $_LIBFLAGS -Wl,--end-group',
        # List of common objects
        COMMON_OBJECTS=[
            ('#lib/wlan.a'),
        ],
        SDKBINDIR=SDKBINDIR,
        MAKEIMG=os.path.normpath('tools/makeimg'),
        MAKEIMG_ALL=os.path.normpath('tools/makeimg_all'),
    ),
    'debug': dict(
        BUILDROOT=os.path.join(_BUILD_BASE, 'Debug'),
    ),
    'release': dict(
        BUILDROOT=os.path.join(_BUILD_BASE, 'Release'),
    ),
}

# Dictionary of flavor-specific settings that should extend values
#  from the base environment (using env.Append).
# `_common` is reserved for settings that apply to the base env.
ENV_EXTENSIONS = {
    '_common': dict(
        # Common flags for all C++ builds
        # CCFLAGS=['-std=c++11', '-Wall',
        #          '-fvectorize', '-fslp-vectorize', '-g3'],
        # Modules should be able to include relative to build root dir
        # CPPPATH=['#$BUILDROOT'],
        CPPDEFINES=[
            'GCC_COMPILE=1',
            'WM_W600'
        ],
        CPPPATH=[
            '#demo',
            '#include',
            '#include/app',
            '#include/driver',
            '#include/net',
            '#include/os',
            '#include/platform',
            '#include/wifi',
            '#platform/boot/gcc',
            '#platform/common/crypto',
            '#platform/common/crypto/digest',
            '#platform/common/crypto/keyformat',
            '#platform/common/crypto/math',
            '#platform/common/crypto/prng',
            '#platform/common/crypto/pubkey',
            '#platform/common/crypto/symmetric',
            '#platform/common/Params',
            '#platform/inc',
            '#platform/sys',
            # '#src/app/ajtcl-15.04.00a/external/sha2',
            # '#src/app/ajtcl-15.04.00a/inc',
            # '#src/app/ajtcl-15.04.00a/target/winnermicro',
            # '#src/app/cjson',
            # '#src/app/cloud',
            # '#src/app/cloud/kii',
            # '#src/app/demo',
            # '#src/app/dhcpserver',
            # '#src/app/dnsserver',
            # '#src/app/gmediarender-0.0.6',
            # '#src/app/httpclient',
            # '#src/app/iperf',
            # '#src/app/libupnp-1.6.19/ixml/inc',
            # '#src/app/libupnp-1.6.19/ixml/include',
            # '#src/app/libupnp-1.6.19/threadutil/include',
            # '#src/app/libupnp-1.6.19/upnp/inc',
            # '#src/app/libupnp-1.6.19/upnp/include',
            # '#src/app/libwebsockets-2.1-stable',
            # '#src/app/matrixssl',
            # '#src/app/matrixssl/core',
            # '#src/app/matrixssl/crypto',
            # '#src/app/mdns/mdnscore',
            # '#src/app/mdns/mdnsposix',
            # '#src/app/mqtt',
            # '#src/app/ntp',
            # '#src/app/oneshotconfig',
            # '#src/app/ota',
            # '#src/app/ping',
            # '#src/app/polarssl/include',
            # '#src/app/rmms',
            # '#src/app/web',
            # '#src/app/wm_atcmd',
            '#src/app/cjson',
            '#src/app/dhcpserver',
            '#src/app/dnsserver',
            '#src/app/httpclient',
            '#src/app/iperf',
            '#src/app/libcoap/include',
            '#src/app/libwebsockets-2.1-stable',
            '#src/app/matrixssl',
            '#src/app/matrixssl/core',
            '#src/app/mdns',
            '#src/app/mdns/mdnscore',
            '#src/app/mdns/mdnsposix',
            '#src/app/mqtt',
            '#src/app/ntp',
            '#src/app/oneshotconfig',
            '#src/app/ota',
            '#src/app/ping',
            '#src/app/polarssl/include',
            '#src/app/sslserver',
            '#src/app/web',
            '#src/app/wm_atcmd',
            '#src/network/api2.0.3',
            '#src/network/lwip2.0.3/include',
            '#src/network/lwip2.0.3/include/arch',
            '#src/network/lwip2.0.3/include/lwip',
            '#src/os/os_ports',
            '#src/os/rtos/include',
            '#src/wlan/driver',
            '#src/wlan/supplicant',
        ],
        # List of supported external libraries
        EXTERNAL_LIBRARIES=[
            ExtLib('airkiss', lib_paths='#lib'),
            # ExtLib('wlan'),
        ],
    ),
    'debug': dict(
        # Extra flags for debug C++ builds
        # CCFLAGS=['-g', '-DDEBUG'],
        CPPDEFINES=['_DEBUG'],
        CCFLAGS=['-O0'],
        ASFLAGS=['-O0'],
    ),
    'release': dict(
        # Extra flags for release C++ builds
        # CCFLAGS=['-O2', '-DNDEBUG'],
        CPPDEFINES=['NDEBUG'],
        CCFLAGS=['-Os'],
        ASFLAGS=['-Os'],
    ),
}


def flavors():
    """Generate supported flavors.

    Each flavor is a string representing a flavor entry in the
    override / extension dictionaries above.
    Each flavor entry must define atleast "BUILDROOT" variable that
    tells the system what's the build base directory for that flavor.
    """
    # Use the keys from the env override / extension dictionaries
    for flavor in set(list(ENV_EXTENSIONS.keys()) + list(ENV_OVERRIDES.keys())):
        # Skip "hidden" records
        if not flavor.startswith('_'):
            yield flavor


def main():
    """Main procedure - print out a requested variable (value per line)"""
    import sys
    if 2 == len(sys.argv):
        var = sys.argv[1].lower()
        items = list()
        if var in ('flavors',):
            items = flavors()
        elif var in ('modules',):
            items = modules()
        elif var in ('build', 'build_dir', 'build_base'):
            items = [_BUILD_BASE]
        elif var in ('bin', 'bin_subdir'):
            items = [_BIN_SUBDIR]
        # print out the item values
        for idx, val in enumerate(items):
            # print(idx, val)
            print(val)


if '__main__' == __name__:
    main()
