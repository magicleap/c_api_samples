####################################################################################
# Build and test native apps.
#
# Run with -help for info.
####################################################################################
import argparse
import os
import sys
from only_common import *
import build_external_deps

vi = sys.version_info
if (vi.major, vi.minor) < (3, 5):
    print("Please run this script with Python 3.5 or newer")
    exit(1)


def layout_package(build_args):
    print_step("Laying out distribution package...")
    layout_args = [build_args.mabu] + build_args.mabu_args + \
        ['--target=host'] + build_args.packages

    subprocess.run(layout_args, cwd=build_args.basedir, check=True)

    # make sure any executables copied are really +x'ed
    fix_permissions(build_args.distdir)


def build_packages(build_args):
    update_for_ccache(build_args.mabu_args, build_args.ccache)
    mabu_projects = list(get_area_projects(build_args))
    cmd = [build_args.mabu] + mabu_projects + build_args.mabu_args
    for target in build_args.targets:
        print_step("Building packages for {}...".format(target))
        subprocess.run(cmd + ['--target', target], check=True)


def register_args(parser):
    parser.add_argument('mabu_args', nargs='*', default=['-j'],
                        help='any additional arguments to pass to mabu. Note that you must to separate these from other flags (i.e. python3.5 build.py --targets device -- --jobs 12). Also, do not use the --target or -t arguments, as they will override --targets')

    parser.add_argument('--build-deps', dest='build_deps', action='store_true', default=None,
                        help='''build external dependencies''')

    parser.add_argument('--ccache', dest='ccache', nargs='?', const='ccache',
                        help='''run compiler with 'ccache' (or the provided executable); without an argument,
                      the script will check whether the executable exists (thus, it's safe to pass this even
                      if you're not sure it's installed)''')

    parser.add_argument('--assemble-mlsdk', '--asmsdk', dest='asm_sdk', action='store_true', default=None,
                        help='''build stubs and overlay into mlsdk''')


def parse_args(project):
    parser = argparse.ArgumentParser()
    register_common_args(parser)
    register_args(parser)

    areas = list(get_area_project_map(project).keys())
    areas.sort()
    parser.add_argument('-a', '--areas', dest='areas', action=AreaParserAction,
                        help='''specify areas to build, comma-separated (default: all); choices: {}'''
                        .format(', '.join(areas)))

    build_args = parser.parse_args()
    canonicalize_args(build_args)
    return build_args


def build(build_args, base_dir, project, dist_dir, packages, clean_rm_dirs=None):
    build_args.basedir = base_dir or '.'
    build_args.project = project
    build_args.distdir = dist_dir
    build_args.packages = packages

    if build_args.build_deps:
        build_external_deps.main()
    build_packages(build_args)
    if not ('-c' in build_args.mabu_args or '--clean' in build_args.mabu_args):
        layout_package(build_args)
