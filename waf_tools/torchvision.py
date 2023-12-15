#! /usr/bin/env python
# encoding: utf-8
# Konstantinos Chatzilygeroudis - 2015

"""
Quick n dirty torchvision C++ detection
"""

import os
from copy import deepcopy
from waflib.Configure import conf


def options(opt):
    opt.add_option('--torchvision', type='string', help='path to torchvision C++', dest='torchvision')

@conf
def check_torchvision(conf, *k, **kw):
    def fail(msg, required):
        if required:
            conf.fatal(msg)
        conf.end_msg(msg, 'RED')
    def get_directory(filename, dirs):
        res = conf.find_file(filename, dirs)
        return res[:-len(filename)-1]

    required = kw.get('required', False)
    global_path = kw.get('global_path', '')

    # OSX/Mac uses .dylib and GNU/Linux .so
    suffix = 'dylib' if conf.env['DEST_OS'] == 'darwin' else 'so'

    if conf.options.torchvision:
        includes_check = [conf.options.torchvision + '/include']
        libs_check = [conf.options.torchvision + '/lib']
    else:
        includes_check = ['/usr/local/include', '/usr/include']
        libs_check = ['/usr/local/lib', '/usr/local/lib64', '/usr/lib', '/usr/lib64', '/usr/lib/x86_64-linux-gnu/']

    if len(global_path) > 0:
        if isinstance(global_path, list):
            includes_check = [g + '/include' for g in global_path]
            libs_check = [g + '/lib' for g in global_path]
        else:
            includes_check = [global_path + '/include']
            libs_check = [global_path + '/lib']

    try:
        conf.start_msg('Checking for torchvision includes')

        torchvision_include = []
        torchvision_include.append(get_directory('torchvision/vision.h', includes_check))
        torchvision_include = list(set(torchvision_include))
        conf.end_msg('Found in ' + torchvision_include[0]) # TO-DO: Add version?

        conf.start_msg('Checking for torchvision libs')
        torchvision_lib = []
        torchvision_lib.append(get_directory('libtorchvision.' + suffix, libs_check))
        torchvision_lib = list(set(torchvision_lib))
        conf.env.INCLUDES_TORCHVISION = torchvision_include
        conf.env.LIBPATH_TORCHVISION = torchvision_lib
        conf.env.LIB_TORCHVISION = ['torchvision']
        conf.end_msg(conf.env.LIB_TORCHVISION)
    except:
        fail('Not found', required)
    return 1
