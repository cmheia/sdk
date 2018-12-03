# Copyright 2014 The Ostrich / by Itamar O
# Copyright 2018 cmheia

"""SCons site init script - automatically imported by SConstruct"""

import bcolors
import colorama
import gzip
import os
import platform

from collections import defaultdict

import SCons
from SCons import Node
from SCons.Errors import StopError

from site_config import flavors, modules, ENV_OVERRIDES, ENV_EXTENSIONS
from site_utils import listify, path_to_key, dummy_op, log_err, log_info, log_warn, dump_info

_ENABLE_DEBUG_LIB = False
_ENABLE_DEBUG_PROG = False

_AUTO_INSTALL_EXE = False
_DEFAULT_FINISHING = True

colorama.init()

OS_NAME = platform.system()
if OS_NAME == 'Windows':
    log_warn('Build on Windows')

log_info('python {}'.format(platform.python_version()))


# binary file builder
def lst_generator(source, target, env, for_signature):
    return '{} --source --all-headers --demangle --line-numbers --wide {}>{}'.format(env['OBJDUMP'], source[0], target[0])


def siz_generator(source, target, env, for_signature):
    return '{} --format=berkeley {}'.format(env['SIZE'], source[0])


def bin_generator(source, target, env, for_signature):
    return '{} -O binary {} {}'.format(env['OBJCOPY'], source[0], target[0])


def hex_generator(source, target, env, for_signature):
    return '{} -O ihex {} {}'.format(env['OBJCOPY'], source[0], target[0])


# .bin.gz
# wm_gzip.exe $(TARGET).bin
def zbin_generator(source, target, env):
    # log_info('zbin source ' + str([s.rstr() for s in source]))
    # log_err('zbin target ' + str([s.rstr() for s in target]))
    try:
        with open(source[0].rstr(), 'rb') as f_in:
            with gzip.open(target[0].rstr(), 'wb') as f_out:
                f_out.writelines(f_in)
                f_out.close()
            f_in.close()
    except Exception as error:
        log_err('zbin_generator oops: {}'.format(error))
        return 1
    return None


# .img
# makeimg.exe $(TARGET).bin $(TARGET).img 0 0 version.txt 90000 10100
def img_generator(source, target, env, for_signature):
    # if not for_signature:
    #     log_info('img source ' + str([s.rstr() for s in source]))
    #     log_err('img target ' + str([s.rstr() for s in target]))
    return '{} {} {} 0 0 {} 90000 10100'.format(env['MAKEIMG'],
                                                source[0],
                                                target[0],
                                                os.path.join(env['SDKBINDIR'], 'version.txt'))


# _gz.img
# makeimg.exe $(TARGET).bin.gz $(TARGET)_gz.img 0 1 version.txt 90000 10100 $(TARGET).bin
def zimg_generator(source, target, env, for_signature):
    # if not for_signature:
    #     log_info('zimg source ' + str([s.rstr() for s in source]))
    #     log_err('zimg target ' + str([s.rstr() for s in target]))
    return '{} {} {} 0 1 {} 90000 10100 {}'.format(env['MAKEIMG'],
                                                   source[0],
                                                   target[0],
                                                   os.path.join(
                                                       env['SDKBINDIR'], 'version.txt'),
                                                   source[1])


# .fls
# makeimg_all.exe secboot.img $(TARGET).img $(TARGET).fls
def fls_generator(source, target, env, for_signature):
    # if not for_signature:
    #     log_info('fls source ' + str([s.rstr() for s in source]))
    #     log_err('fls target ' + str([s.rstr() for s in target]))
    return '{} {} {} {}'.format(env['MAKEIMG_ALL'],
                                os.path.join(env['SDKBINDIR'], 'secboot.img'),
                                source[0],
                                target[0])


def get_base_env(*args, **kwargs):
    """Initialize and return a base construction environment.

    All args received are passed transparently to SCons Environment init.
    """
    # Initialize new construction environment
    env = Environment(*args, **kwargs)  # pylint: disable=undefined-variable
    # If a flavor is activated in the external environment - use it
    if 'BUILD_FLAVOR' in os.environ:
        active_flavor = os.environ['BUILD_FLAVOR']
        if not active_flavor in flavors():
            raise StopError(
                '{} (from env) is not a known flavor.'.format(active_flavor))
        log_warn('Using active flavor "{}" from your environment'.format(
            active_flavor))
        env.flavors = [active_flavor]
    else:
        # If specific flavor target specified, skip processing other flavors
        # Otherwise, include all known flavors
        env.flavors = (set(flavors()).intersection(COMMAND_LINE_TARGETS)  # pylint: disable=undefined-variable
                       or flavors())
    # log_warn('flavors')
    # print(env.flavors)
    # Perform base construction environment customizations from site_config
    if '_common' in ENV_OVERRIDES:
        # https://docs.python.org/3/tutorial/controlflow.html#unpacking-argument-lists
        # env.Replace(ENV={'PATH': os.environ['PATH']})
        env.Replace(**ENV_OVERRIDES['_common'])
    if '_common' in ENV_EXTENSIONS:
        env.Append(**ENV_EXTENSIONS['_common'])

    env.Append(BUILDERS={
        'LST': Builder(
            generator=lst_generator,
            suffix='.lst',
            src_suffix='.elf'
        ),
        'SIZ': Builder(
            generator=siz_generator,
            suffix='.siz',
            src_suffix='.elf'
        ),
        'BIN': Builder(
            generator=bin_generator,
            suffix='.bin',
            src_suffix='.elf'
        ),
        'HEX': Builder(
            generator=hex_generator,
            suffix='.hex',
            src_suffix='.elf'
        )
    })

    env.Append(BUILDERS={
        'ZBIN': Builder(
            action=zbin_generator,
            suffix='.bin.gz',
            src_suffix='.bin'
        ),
        'IMG': Builder(
            generator=img_generator,
            suffix='.img',
            src_suffix='.bin'
        ),
        'ZIMG': Builder(
            generator=zimg_generator,
            suffix='_gz.img',
            src_suffix=['.bin.gz', '.bin']
        ),
        'FLS': Builder(
            generator=fls_generator,
            suffix='.fls',
            src_suffix='.bin'
        )
    })
    # log_warn('base_Environment')
    # print(env.Dump())
    log_warn('COMMAND_LINE_TARGETS: {}'.format(COMMAND_LINE_TARGETS))
    log_warn('DEFAULT_TARGETS: {}'.format(DEFAULT_TARGETS))
    log_warn('BUILD_TARGETS: {}'.format(BUILD_TARGETS))
    return env


class FlavorBuilder(object):
    """Build manager class for flavor."""

    _key_sep = '::'

    @classmethod
    def lib_key(cls, module, target_name):
        """Return unique identifier for target `target_name` in `module`"""
        return '{}{}{}'.format(path_to_key(module), cls._key_sep,
                               path_to_key(target_name))

    @classmethod
    def is_lib_key(cls, str_to_check):
        """Return True if `str_to_check` is a library identifier string"""
        return cls._key_sep in str_to_check

    def __init__(self, base_env, flavor, quiet=True):
        """Initialize a build manager instance for flavor.

        @param base_env     Basic construction environment to start from
        @param flavor       The flavor to process
        """
        self._flavor = flavor
        # Create construction env clone for flavor customizations
        self._env = base_env.Clone()
        # Initialize shared libraries dictionary
        self._libs = dict()
        # Initialize programs dictionary
        self._progs = defaultdict(list)
        # Apply flavored env overrides and customizations
        if flavor in ENV_OVERRIDES:
            self._env.Replace(**ENV_OVERRIDES[flavor])
        if flavor in ENV_EXTENSIONS:
            self._env.Append(**ENV_EXTENSIONS[flavor])
        # Support using the flavor name as target name for its related targets
        self._env.Alias(flavor, '$BUILDROOT')
        self._env['VERBOSE'] = not quiet
        if quiet is True:
            self._env['ARCOMSTR'] = bcolors.OK + \
                'AR ' + bcolors.ENDC + '$TARGET'
            self._env['ASCOMSTR'] = bcolors.OK + \
                'AS ' + bcolors.ENDC + '$SOURCE'
            self._env['CCCOMSTR'] = bcolors.OK + \
                'CC ' + bcolors.ENDC + '$SOURCE'
            self._env['CXXCOMSTR'] = bcolors.OK + \
                'CXX ' + bcolors.ENDC + '$SOURCE'
            self._env['LINKCOMSTR'] = bcolors.OK + \
                'LD ' + bcolors.ENDC + '$TARGET'

    def process_module(self, module):
        """Delegate build to a module-level SConscript using the flavored env.

        @param  module  Directory of module

        @raises AssertionError if `module` does not contain SConscript file
        """
        # Verify the SConscript file exists
        sconscript_path = os.path.join(module, 'SConscript')
        assert os.path.isfile(sconscript_path)
        log_info('|- Reading module {} ...'.format(module))
        # Prepare shortcuts to export to SConscript
        shortcuts = dict(
            Lib=self._lib_wrapper(self._env.Library, module),
            StaticLib=self._lib_wrapper(self._env.StaticLibrary, module),
            SharedLib=self._lib_wrapper(self._env.SharedLibrary, module),
            Prog=self._prog_wrapper(module),
        )
        # Access a protected member of another namespace,
        # using an undocumented feature of SCons
        SCons.Script._SConscript.GlobalDict.update(
            shortcuts)  # pylint: disable=protected-access
        # Execute the SConscript file, with variant_dir set to the
        #  module dir under the project flavored build dir.
        self._env.SConscript(
            sconscript_path,
            variant_dir=os.path.join('$BUILDROOT', module),
            duplicate=0)
        # Add install targets for module
        # If module has hierarchical path, replace path-seps with periods
        bin_prefix = path_to_key(module)
        for prog in self._progs[module]:
            assert isinstance(prog, Node.FS.File)
            bin_name = '{}.{}'.format(bin_prefix, prog.name)
            self._env.InstallAs(os.path.join('$BINDIR', bin_name), prog)

    def install_progs(self):
        # [Ch11]Installing Files in Other Directories: the Install Builder
        # installing a file is still considered a type of file "build."
        # Add install targets for programs from all modules
        for module, prog_nodes in self._progs.items():
            for prog_elf in prog_nodes:
                assert isinstance(prog_elf, Node.FS.File)
                # If module is hierarchical, replace pathseps with periods
                bin_name = path_to_key('{}.{}'.format(module, prog_elf.name))
                install_dest = os.path.join('$BINDIR', bin_name)
                log_info('Install {} As {}'.format(
                    prog_elf.rstr(), install_dest))
                installed_artifact = self._env.InstallAs(os.path.join(
                    '$BINDIR', bin_name), prog_elf)
                log_warn('InstallAs: ' + installed_artifact[0].rstr())

    def finishing_progs(self):
        # Create flashable images for programs from all modules
        for module, prog_nodes in self._progs.items():
            for prog_elf in prog_nodes:
                assert isinstance(prog_elf, Node.FS.File)
                prog_siz = self._env.SIZ(source=prog_elf)
                prog_lst = self._env.LST(source=prog_elf)
                prog_bin = self._env.BIN(source=prog_elf)
                prog_zbin = self._env.ZBIN(source=prog_bin)
                prog_img = self._env.IMG(source=prog_bin)
                prog_zimg = self._env.ZIMG(source=[prog_zbin, prog_bin])
                prog_fls = self._env.FLS(source=prog_img)
                # log_warn('siz : {}'.format(prog_siz[0]))
                # log_warn('lst : {}'.format(prog_lst[0]))
                # log_warn('bin : {}'.format(prog_bin[0]))
                # log_warn('zbin: {}'.format(prog_zbin[0]))
                # log_warn('img : {}'.format(prog_img[0]))
                # log_warn('zimg: {}'.format(prog_zimg[0]))
                # log_warn('fls : {}'.format(prog_fls[0]))

                flashable = self._env.InstallAs(
                    [
                        # os.path.join('$BINDIR', path_to_key('{}.{}'.format(
                        #     module, os.path.basename(prog_zimg[0].rstr())))),
                        os.path.join('$BINDIR', path_to_key('{}.{}'.format(
                            module, Flatten(prog_zimg)[0].name))),
                        os.path.join('$BINDIR', path_to_key('{}.{}'.format(
                            module, Flatten(prog_fls)[0].name))),
                    ],
                    [
                        prog_zimg,
                        prog_fls,
                    ])
                log_info('Flashable files {}'.format(
                    [f.rstr() for f in flashable]))

    def build(self):
        """Build flavor using two-pass strategy."""
        # First pass over all modules - process and collect library targets
        for module in modules():
            # Verify the SConscript file exists
            sconscript_path = os.path.join(module, 'SConscript')
            if not os.path.isfile(sconscript_path):
                raise StopError(
                    'Missing SConscript file for module {}.'.format(module))
            log_info('|- First pass: Reading module "{}" ...'.format(module))
            # log_warn('|- variant_dir "{}" '.format(os.path.join( '$BUILDROOT', module)))
            shortcuts = dict(
                Lib=self._lib_wrapper(self._env.Library, module),
                StaticLib=self._lib_wrapper(self._env.StaticLibrary, module),
                SharedLib=self._lib_wrapper(self._env.SharedLibrary, module),
                Prog=dummy_op,
                env=self._env,
            )
            # Access a protected member of another namespace,
            # using an undocumented feature of SCons
            SCons.Script._SConscript.GlobalDict.update(
                shortcuts)  # pylint: disable=protected-access
            self._env.SConscript(
                sconscript_path,
                variant_dir=os.path.join('$BUILDROOT', module),
                duplicate=0,
                exports=shortcuts)
        # Second pass over all modules - process program targets
        shortcuts = dict(env=self._env)
        for nop_shortcut in ('Lib', 'StaticLib', 'SharedLib'):
            shortcuts[nop_shortcut] = dummy_op
        for module in modules():
            log_info('|- Second pass: Reading module "{}" ...'.format(module))
            shortcuts['Prog'] = self._prog_wrapper(module)
            # Access a protected member of another namespace,
            # using an undocumented feature of SCons
            SCons.Script._SConscript.GlobalDict.update(
                shortcuts)  # pylint: disable=protected-access
            self._env.SConscript(
                os.path.join(module, 'SConscript'),
                variant_dir=os.path.join('$BUILDROOT', module),
                duplicate=0,
                exports=shortcuts)
        if _AUTO_INSTALL_EXE:
            self.install_progs()
        if _DEFAULT_FINISHING:
            self.finishing_progs()
        # Support using the flavor name as target name for its related targets
        self._env.Alias(self._flavor, '$BUILDROOT')

    def _lib_wrapper(self, bldr_func, module):
        """Return a wrapped customized flavored library builder for module.

        @param  builder_func        Underlying SCons builder function
        @param  module              Module name
        """
        def build_lib(lib_name, sources, *args, **kwargs):
            """Customized library builder.

            @param  lib_name    Library name
            @param  sources     Source file (or list of source files)
            """
            if _ENABLE_DEBUG_LIB and self._env['VERBOSE'] is True:
                print(self._env.Dump())
            # Create unique library key from module and library name
            lib_key = self.lib_key(module, lib_name)
            assert lib_key not in self._libs
            # Store resulting library node in shared dictionary
            lib_node = bldr_func(lib_name, sources, *args, **kwargs)
            self._libs[lib_key] = lib_node
            return lib_node
        return build_lib

    def _prog_wrapper(self, module, default_install=True):
        """Return a wrapped customized flavored program builder for module.

        @param  module              Module name
        @param  default_install     Whether built program nodes should be
                                    installed in bin-dir by default
        """
        def build_prog(prog_name, sources, with_libs=None, *args, **kwargs):
            """Customized program builder.

            @param  prog_name   Program name
            @param  sources     Source file (or list of source files)
            @param  with_libs   Library name (or list of library names) to
                                link with.
            @param  install     Binary flag to override default value from
                                closure (`default_install`).
            """
            if _ENABLE_DEBUG_PROG and self._env['VERBOSE'] is True:
                print(self._env.Dump())
                # log_warn('LINKFLAGS:', self._env['LINKFLAGS'], '.')
                dump_info('build_prog:args', *args)
                dump_info('build_prog:kwargs', **kwargs)
            # Make sure sources is a list
            sources = listify(sources) + self._env['COMMON_OBJECTS']
            install_flag = kwargs.pop('install', default_install)
            # Extract optional keywords arguments that we might extend
            cpp_paths = listify(kwargs.pop('CPPPATH', None))
            ext_libs = listify(kwargs.pop('LIBS', None))
            lib_paths = listify(kwargs.pop('LIBPATH', None))
            # Process library dependencies - add libs specified in `with_libs`
            for lib_name in listify(with_libs):
                lib_keys = listify(self._get_matching_lib_keys(lib_name))
                if len(lib_keys) == 1:
                    # Matched internal library
                    lib_key = lib_keys[0]
                    # Extend prog sources with library nodes
                    sources.extend(self._libs[lib_key])
                elif len(lib_keys) > 1:
                    # Matched multiple internal libraries - probably bad!
                    raise StopError('Library identifier "{}" matched {} '
                                    'libraries ({}). Please use a fully '
                                    'qualified identifier instead!'.format(lib_name, len(lib_keys),
                                                                           ', '.join(lib_keys)))
                else:  # empty lib_keys
                    # Maybe it's an external library
                    ext_lib = self._get_external_library(lib_name)
                    if ext_lib:
                        # Matched external library - extend target parameters
                        cpp_paths.extend(ext_lib.cpp_paths)
                        ext_libs.extend(ext_lib.libs)
                        lib_paths.extend(ext_lib.lib_paths)
                    else:
                        raise StopError('Library identifier "{}" didn\'t match '
                                        'any library. Is it a typo?'.format(lib_name))
            # Return extended construction environment parameters to kwargs
            if cpp_paths:
                kwargs['CPPPATH'] = cpp_paths
            if ext_libs:
                kwargs['LIBS'] = ext_libs
            if lib_paths:
                kwargs['LIBPATH'] = lib_paths
            # Build the program and add to prog nodes dict if installable
            prog_nodes = self._env.Program(prog_name, sources, *args, **kwargs)
            if install_flag:
                # storing each installable node in a dictionary instead of
                #  defining InstallAs target on the spot, because there's
                #  an "active" variant dir directive messing with paths.
                self._progs[module].extend(prog_nodes)
            return prog_nodes
        return build_prog

    def _get_matching_lib_keys(self, lib_query):
        """Return list of library keys for given library name query.

        A "library query" is either a fully-qualified "Module::LibName" string
         or just a "LibName".
        If just "LibName" form, return all matches from all modules.
        """
        if self.is_lib_key(lib_query):
            # It's a fully-qualified "Module::LibName" query
            if lib_query in self._libs:
                # Got it. We're done.
                return [lib_query]
        else:
            # It's a target-name-only query. Search for matching lib keys.
            lib_key_suffix = '{}{}'.format(self._key_sep, lib_query)
            return [lib_key for lib_key in self._libs
                    if lib_key.endswith(lib_key_suffix)]

    def _get_external_library(self, lib_name):
        """Return external library object with name `lib_name` (or None)."""
        for lib in self._env['EXTERNAL_LIBRARIES']:
            if lib.name == lib_name:
                return lib
