#!/usr/bin/env python
# encoding: utf-8


import os
from waflib import Utils, Logs
from waflib.Configure import conf


def options(opt):
  opt.add_option('--rapidyml', type='string', help='path to rapidyml', dest='rapidyml')


@conf
def check_rapidyml(conf, *k, **kw):
    def get_directory(filename, dirs):
        res = conf.find_file(filename, dirs)
        return res[:-len(filename)-1]

    required = kw.get('required', False)
    global_path = kw.get('global_path', '')

    msg = ''
    if not required:
        msg = ' [optional]'

    includes_check = ['/usr/include', '/usr/local/include', '/opt/include', '~/.local/include']
    libs_check = ['/usr/lib', '/usr/local/lib', '/opt/lib', '/sw/lib', '/lib', '/usr/lib/x86_64-linux-gnu/', '/usr/lib64', '~/.local/lib']

    # OSX/Mac uses .dylib and GNU/Linux .so
    lib_suffix = '.dylib' if conf.env['DEST_OS'] == 'darwin' else '.so'

    if conf.options.rapidyml:
        includes_check = [conf.options.rapidyml + '/include']
        libs_check = [conf.options.rapidyml + '/lib']

    if len(global_path) > 0:
        if isinstance(global_path, list):
            includes_check = [g + '/include' for g in global_path]
            libs_check = [g + '/lib' for g in global_path]
        else:
            includes_check = [global_path + '/include']
            libs_check = [global_path + '/lib']

    try:
        conf.start_msg('Checking for rapidyml includes' + msg)
        dirs = []
        dirs.append(get_directory('ryml.hpp', includes_check))
        dirs.append(get_directory('ryml_std.hpp', includes_check))
        dirs = list(set(dirs))
        conf.end_msg(dirs)
        conf.env.INCLUDES_RAPIDYML = dirs

        conf.start_msg('Checking for rapidyml libraries' + msg)

        lib_dirs = []
        libraries = ['ryml', 'c4core']
        libs_ext = ['.a', lib_suffix]
        lib_found = False
        type_lib = '.a'
        for libtype in libs_ext:
            try:
                for lib in libraries:
                    lib_dir = get_directory('lib' + lib + libtype, libs_check)
                    lib_dirs.append(lib_dir)
                lib_found = True
                type_lib = libtype
                break
            except:
                lib_found = False
        if not lib_found:
            if required:
                conf.fatal('Not found')
            conf.end_msg('Not found', 'RED')
            return
        lib_dirs = list(set(lib_dirs))
        conf.end_msg(lib_dirs)

        conf.env.LIBPATH_RAPIDYML = lib_dirs
        if type_lib == '.a':
            conf.env.STLIB_RAPIDYML = libraries
        else:
            conf.env.LIB_RAPIDYML = libraries
    except:
        if required:
            conf.fatal('Not found')
        conf.end_msg('Not found', 'RED')
        return
    return 1