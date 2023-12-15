#!/usr/bin/env python
# encoding: utf-8


import os
from waflib import Utils, Logs
from waflib.Configure import conf


def options(opt):
    opt.add_option('--opencv', type='string', help='path to opencv', dest='opencv')


@conf
def check_opencv(conf, *k, **kw):
    def get_directory(filename, dirs):
        res = conf.find_file(filename, dirs)
        return res[:-len(filename)-1]

    required = kw.get('required', False)
    global_path = kw.get('global_path', '')

    msg = ''
    if not required:
        msg = ' [optional]'

    includes_check = ['/usr/include', '/usr/local/include', '/usr/include/opencv4/', '/usr/local/include/opencv4/', '~/.local/opencv/include/opencv4']
    libs_check = ['/usr/lib', '/usr/local/lib', '/opt/local/lib', '/sw/lib', '/lib', '/usr/lib/x86_64-linux-gnu/', '/usr/lib64', '~/.local/opencv/lib']

    # OSX/Mac uses .dylib and GNU/Linux .so
    lib_suffix = 'dylib' if conf.env['DEST_OS'] == 'darwin' else 'so'

    if conf.options.opencv:
        includes_check = [conf.options.opencv + '/include/opencv4']
        libs_check = [conf.options.opencv + '/lib']

    if len(global_path) > 0:
        if isinstance(global_path, list):
            includes_check = [g + '/include' for g in global_path]
            includes_check += [g + '/include/opencv4' for g in global_path]
            libs_check = [g + '/lib' for g in global_path]
            libs_check += [g + '/lib/x86_64-linux-gnu' for g in global_path]
            libs_check += [g + '/lib64' for g in global_path]
        else:
            includes_check = [global_path + '/include']
            includes_check += [global_path + '/include/opencv4']
            libs_check = [global_path + '/lib']
            libs_check += [global_path + '/lib/x86_64-linux-gnu']
            libs_check += [global_path + '/lib64']

    try:
        conf.start_msg('Checking for OpenCV includes' + msg)
        dirs = []
        dirs.append(get_directory('opencv2/opencv.hpp', includes_check))
        conf.end_msg(dirs)
        dirs = list(set(dirs))
        conf.env.INCLUDES_OPENCV = dirs

        conf.start_msg('Checking for OpenCV libraries' + msg)

        lib_dirs = []
        libraries = ['opencv_core', 'opencv_highgui', 'opencv_imgproc', 'opencv_features2d', 'opencv_imgcodecs', 'opencv_bgsegm', 'opencv_video', 'opencv_videoio', 'opencv_calib3d', 'opencv_freetype']
        for lib in libraries:
            lib_dir = get_directory('lib' + lib + '.' + lib_suffix, libs_check)
            lib_dirs.append(lib_dir)
        lib_dirs = list(set(lib_dirs))
        conf.end_msg(lib_dirs)
        conf.env.LIBPATH_OPENCV = lib_dirs

        conf.env.LIB_OPENCV = libraries

        required = False
        conf.start_msg('Checking for OpenCV CUDA libraries' + msg)
        cuda_libraries = ['opencv_cudawarping', 'opencv_cudaimgproc', 'opencv_cudabgsegm', 'opencv_cudafilters', 'opencv_cudaarithm']
        for lib in cuda_libraries:
            lib_dir = get_directory('lib' + lib + '.' + lib_suffix, libs_check)
            lib_dirs.append(lib_dir)
        lib_dirs = list(set(lib_dirs))
        conf.env.LIBPATH_OPENCV = lib_dirs

        conf.env.LIB_OPENCV = libraries + cuda_libraries
        conf.env.DEFINES_OPENCV = ['USE_OPENCV_CUDA']
        conf.end_msg(lib_dirs)
    except:
        if required:
            conf.fatal('Not found')
        conf.end_msg('Not found', 'RED')
        return
    return 1
