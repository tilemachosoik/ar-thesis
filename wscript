#!/usr/bin/env python
# encoding: utf-8
from waflib.Tools import waf_unit_test
from waflib import Logs
from waflib.Build import BuildContext
import sys
import os
import fnmatch
import glob
sys.path.insert(0, sys.path[0]+'/waf_tools')

VERSION = '1.0.0'
APPNAME = 'logo_rendering_thesis'

srcdir = '.'
blddir = 'build'

import corrade
import magnum
import magnum_plugins


def options(opt):
    opt.load('compiler_cxx')
    opt.load('compiler_c')
    opt.load('waf_unit_test')
    opt.load('opencv')
    opt.load('rapidyml')
    opt.load('corrade')
    opt.load('magnum')
    opt.load('magnum_plugins')
    opt.load('glfw')
    opt.load('torch')
    opt.load('torchvision')

    opt.add_option('--asan', action='store_true', help='Enable address sanitizer', dest='asan')
    opt.add_option('--install_dir', type='string', help='Path to global installation folder', dest='global_path')


def configure(conf):
    conf.load('compiler_cxx')
    conf.load('compiler_c')
    conf.load('waf_unit_test')
    conf.load('opencv')
    conf.load('rapidyml')
    conf.load('corrade')
    conf.load('magnum')
    conf.load('magnum_plugins')
    conf.load('glfw')
    conf.load('torch')
    conf.load('torchvision')

    global_path = ''
    if conf.options.global_path:
        global_path = conf.options.global_path

    # we need pthread
    conf.check(features='cxx cxxprogram', lib=['pthread'], uselib_store='PTHREAD')
    # we need libz -- first try with pkg-config
    try:
        conf.check_cfg(package='zlib', args=['--cflags', '--libs'], uselib_store='ZLIB', force_static=True)
    except:
        # if failed with pkg-config, try checking if we can directly link to the lib
        conf.check(features='cxx cxxprogram', lib=['z'], uselib_store='ZLIB', force_static=True)
    # we need rapidyml
    conf.check_rapidyml(required=True, global_path=global_path)
    # Find OpenCV with custom script
    conf.check_opencv(required=True, global_path=global_path)
    # Find libtorch and deps with custom script
    conf.check_torch(required=False, global_path=global_path)
    # Find torchvision with custom script
    conf.check_torchvision(required=False, global_path=global_path)

    # Check for Magnum related stuff
    conf.check_corrade(components='Utility PluginManager', required=True, global_path=global_path)
    conf.env['magnum_dep_libs'] = 'Shaders Trade Text'
    if conf.env['DEST_OS'] == 'darwin':
        conf.env['magnum_dep_libs'] += ' WindowlessCglApplication'
    else:
        conf.env['magnum_dep_libs'] += ' WindowlessEglApplication'
    if len(conf.env.INCLUDES_Corrade):
        conf.check_magnum(components=conf.env['magnum_dep_libs'], required=True, global_path=global_path)
    conf.env['magnum_libs'] = magnum.get_magnum_dependency_libs(conf, conf.env['magnum_dep_libs'])

    conf.check_glfw(required=False, global_path=global_path)

    if conf.env.CXX_NAME in ["icc", "icpc"]:
        common_flags = "-Wall -std=c++17"
        opt_flags = " -O3 -xHost -unroll -g "
    elif conf.env.CXX_NAME in ["clang"]:
        common_flags = "-Wall -std=c++17"
        # no-stack-check required for Catalina
        opt_flags = " -O3 -g -faligned-new  -fno-stack-check"
    else:
        gcc_version = int(conf.env['CC_VERSION'][0]+conf.env['CC_VERSION'][1])
        common_flags = "-Wall -std=c++17"
        opt_flags = " -O3 -g"
        if gcc_version >= 71:
            opt_flags = opt_flags + " -faligned-new"

    common_flags += " -Werror"
    all_flags = common_flags + opt_flags
    if conf.options.asan:
        all_flags += ' -fsanitize=address'
        conf.env['LDFLAGS'] += ['-fsanitize=address']
        conf.env.DEFINES_OPENCV = []
    conf.env['CXXFLAGS'] = conf.env['CXXFLAGS'] + all_flags.split(' ')

    print(conf.env['CXXFLAGS'])


def summary(bld):
    lst = getattr(bld, 'utest_results', [])
    total = 0
    tfail = 0
    if lst:
        total = len(lst)
        tfail = len([x for x in lst if x[1]])
    waf_unit_test.summary(bld)
    if tfail > 0:
        bld.fatal("Build failed, because some tests failed!")


def build(bld):
    # compilation of experiment
    libs = bld.env['magnum_libs'] + ' ZLIB PTHREAD OPENCV RAPIDYML TORCH TORCHVISION GLFW'

    # build resources (shader files)
    shaders_resource = corrade.corrade_add_resource(bld, name = 'LiveStreamShaders', config_file = 'src/opengl_rendering/shaders/resources.conf')

    # Main source files
    files = []
    for root, dirnames, filenames in os.walk(bld.path.abspath()+'/src/'):
        for filename in fnmatch.filter(filenames, '*.cpp'):
            ffile = os.path.join(root, filename)
            files.append(ffile)
    files = [f[len(bld.path.abspath())+1:] for f in files]
    streamer_srcs = " ".join(files)
    # External source files
    ext_files = []
    for root, dirnames, filenames in os.walk(bld.path.abspath()+'/external/'):
        for filename in fnmatch.filter(filenames, '*.cpp'):
            ffile = os.path.join(root, filename)
            ext_files.append(ffile)
    ext_files = [f[len(bld.path.abspath())+1:] for f in ext_files]
    ext_srcs = " ".join(ext_files)

    all_srcs = streamer_srcs + ' ' + ext_srcs + ' ' + shaders_resource

    bld.program(features='cxx',
                # install_path = None,
                source=all_srcs,
                includes='./src ./external ' + blddir,
                uselib=libs,
                defines=[],
                target='streamer')
