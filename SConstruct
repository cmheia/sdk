# Copyright 2018 cmheia
# gnu arm toolchain must be already in system path

import os

from site_config import ENV_DEFAULT_OPTIONS
from site_utils import log_err, log_info, log_warn

PROJECT_HELP = """usage: scons [OPTION] [TARGET or FLAVOR_NAME] ...

SCons Options:
  -c, --clean, --remove       Remove specified targets and dependencies.
  -h, --help                  Print this one message and exit.
  -H, --help-options          Print SCons standard help message.
  -j N, --jobs=N              Allow N jobs at once (I recommend 8).
  -n, --no-exec, --just-print, --dry-run, --recon
                              Don't build; just print commands.
  -s, --silent, --quiet       Don't print commands.
  -u, --up, --search-up       Search up directory tree for SConstruct,
                                build targets at or below current directory.
"""

if GetOption('help'):
    # Skip it all if user just wants help
    Help(PROJECT_HELP)
else:
    VERBOSE = False
    if ARGUMENTS.get('VERBOSE') is '1':
        VERBOSE = True

    # Get the base construction environment
    _BASE_ENV = get_base_env(**ENV_DEFAULT_OPTIONS)
    # log_warn('CPPDEFINES')
    # print(_BASE_ENV['CPPDEFINES'])

    _BASE_ENV['PROJECT_ROOT'] = os.path.abspath('.')

    log_info('PROJECT_ROOT: {}'.format(_BASE_ENV['PROJECT_ROOT']))

    # Build every selected flavor
    for flavor in _BASE_ENV.flavors:
        log_info('+ Processing flavor {} ...'.format(flavor))
        # Prepare flavored environment
        flav_bldr = FlavorBuilder(_BASE_ENV, flavor, quiet=not VERBOSE)
        # if VERBOSE is True:
        #     log_warn('Environment')
        #     print(flavored_env.Dump())
        #     print(flavored_env['CPPDEFINES'])
        flav_bldr.build()
