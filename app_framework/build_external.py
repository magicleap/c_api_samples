#!/usr/bin/env python3
import argparse
import os
import sys
import subprocess
import platform
import shutil

ABSOLUTE_PATH = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(ABSOLUTE_PATH, '..', 'scripts'))
from build_project import register_common_args, canonicalize_args

IS_HOST_WINDOWS = platform.system() == "Windows"

MABU = 'mabu.cmd' if IS_HOST_WINDOWS else 'mabu'

MABU_PATH = shutil.which(MABU)
if MABU_PATH is not None:
  SDK_PATH = os.path.dirname(MABU_PATH)
else:
  print("Could not locate " + MABU + ", ensure you've ran 'source envsetup.sh' or 'envsetup.bat'")
  exit(1)

def register_args(parser):
    parser.add_argument('--clean', '-c', action='store_true',
                        help='''remove the build output of external dependencies''')
    parser.add_argument('--rebuild', '-r', action='store_true',
                        help='''clean then build the external dependencies''')
    parser.add_argument('--parallel-cmake', dest='parallel_cmake', action='store_true', default=None,
                        help='''build cmake dependencies in parallel''')

def run_cmake(target, parallel=False):
    mabu_spec = subprocess.run(
        [MABU, '--print-spec', '--target', target], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True).stdout.decode("utf-8").strip()

    source_path = os.path.join(ABSOLUTE_PATH, 'external')
    build_path = os.path.join(ABSOLUTE_PATH, 'external', 'build', mabu_spec)
    install_path = os.path.join(ABSOLUTE_PATH, 'external', 'package', mabu_spec)

    os.makedirs(build_path, exist_ok=True)

    additional_defs = []
    if 'lumin' in mabu_spec:
        subprocess.run(
            [MABU, '--target', target,
             '--create-cmake-toolchain', os.path.join(
                 build_path, 'mlsdk.toolchain.cmake'),
             '--path', '.'], stdout=subprocess.PIPE, check=True)
        additional_defs += ['-DCMAKE_TOOLCHAIN_FILE={}/mlsdk.toolchain.cmake'.format(build_path),
                            '-G', 'Unix Makefiles']
        if IS_HOST_WINDOWS:
            make_program = os.path.join(
                SDK_PATH, 'tools', 'mabu', 'tools', 'MinGW', 'msys', '1.0', 'bin', 'make.exe')
            additional_defs += ['-DCMAKE_MAKE_PROGRAM=' + make_program]
    elif IS_HOST_WINDOWS:
        additional_defs += ['-DCMAKE_GENERATOR_PLATFORM=x64']

    additional_build_params = []
    if parallel:
        additional_build_params += ['--parallel']

    build_type = 'Release' if 'release' in target else 'Debug'

    modified_env = os.environ.copy()

    if 'MLSDK' not in os.environ:
      modified_env['MLSDK'] = SDK_PATH
    else:
      print("Using MLSDK environment variable: '" + os.environ['MLSDK'] + "'")

    subprocess.run(['cmake',
                    source_path,
                    '-DCMAKE_BUILD_TYPE=' + build_type,
                    '-DCMAKE_INSTALL_PREFIX=' + install_path,
                    ] + additional_defs, env=modified_env, cwd=build_path, check=True)
    subprocess.run(['cmake',
                    '--build', build_path,
                    '--config', build_type,
                    '--target', 'install',
                    ] + additional_build_params, cwd=build_path, check=True)


def build_external(build_args):
    parallel = build_args.parallel_cmake
    for target in build_args.targets:
        run_cmake(target, parallel)


def clean():
    base_build_path = os.path.join(ABSOLUTE_PATH, 'external', 'build')
    base_install_path = os.path.join(ABSOLUTE_PATH, 'external', 'package')
    shutil.rmtree(base_build_path, ignore_errors=True)
    print("deleted {}".format(base_build_path))
    shutil.rmtree(base_install_path, ignore_errors=True)
    print("deleted {}".format(base_install_path))


def parse_args():
    parser = argparse.ArgumentParser(description='Build external dependencies that use build systems other than mabu')
    register_common_args(parser)
    register_args(parser)

    (build_args, _) = parser.parse_known_args()

    canonicalize_args(build_args)
    return build_args


if __name__ == '__main__':
    build_args = parse_args()
    if build_args.clean or build_args.rebuild:
        clean()
    if not build_args.clean or build_args.rebuild:
        build_external(build_args)
