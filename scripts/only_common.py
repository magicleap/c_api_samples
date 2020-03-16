from __future__ import print_function
import shutil
import sys, time

vi = sys.version_info
if (vi.major, vi.minor) < (3, 5):
  print("Please run this script with Python 3.5 or newer")
  sys.exit(1)

import os
import shlex
import subprocess
import argparse
import platform

is_verbose = False

def print_step(msg, finished=False):
  sys.stdout.flush()
  sys.stderr.flush()

  if finished:
    print('\n')

  print("[{}] >>>>>> {}\n".format(time.strftime("%H:%M:%S"), msg))

  sys.stdout.flush()
  sys.stderr.flush()


def launch(args, **kwargs):
  """ Launch program from @args and return exit code """
  comp = launch_with_completion(args, **kwargs)
  return comp.returncode if comp else -1


def launch_with_completion(args, **kwargs):
  """ Launch program from @args and return ProcessCompletion object """
  if is_verbose:
    cmd_str = shlex.quote(' '.join(args))
    env = kwargs.get('env')
    if env:
      orig_env = dict(os.environ)
      for k, v in env.items():
        if k not in orig_env or orig_env[k] != env[k]:
          cmd_str = k + '=' + v + ' ' + cmd_str
    print("[{}] Running: {}".format(time.strftime("%H:%M:%S"), cmd_str))
    sys.stdout.flush()
    sys.stderr.flush()

  # Workaround for bug affecting CI windows nodes. Sets mabu's PYTHON variable
  # to match the calling python executable. On CI, it seemed to switch to
  # the system default python under some conditions.
  if args[0] == "mabu" or args[0] == "mabu.cmd":
    args.append("PYTHON=" + sys.executable)

  try:
    comp = subprocess.run(args, **kwargs)
    return comp
  except subprocess.TimeoutExpired:
    print(args, "has taken more than", kwargs['timeout'], "seconds and has been terminated")
    return None
  finally:
    sys.stdout.flush()
    sys.stderr.flush()


def find_mabu():
  """ find the name for invoking mabu (a script) """
  return 'mabu.cmd' if sys.platform == 'win32' else 'mabu'


def find_mlsdk():
  """ find the appropriate MLSDK to use """
  mlsdk = os.getenv('MLSDK')

  if not mlsdk:
    # search PATH to see if it has been set up
    for p in (os.getenv('Path') or os.getenv('PATH')).split(os.pathsep):
      if os.path.exists(os.path.join(p, 'include/ml_api.h')):
        mlsdk = p
        break

  return mlsdk


def update_for_ccache(cmdline, ccache):
  if ccache == 'ccache':
    # default value: make sure it exists
    ccache = shutil.which('ccache')
    if not ccache:
      print("**** Cannot locate ccache, ignoring --ccache")

  if ccache:
    cmdline.insert(0, 'COMPILER_PREFIX=' + ccache)
    cmdline.insert(0, 'LINKER_PREFIX=' + ccache)


def get_os_segment():
  return {
      'Windows': 'win64',
      'Darwin': 'osx',
      'Linux': 'linux64'
  } [platform.system()]


# Map of base directory to raw area to list of .mabu or .package paths
_area_projects = {}
""" :type: dict[str, dict[str, list[str]]] """


# Map of areas to all the contained areas
_area_areas = {}
""" :type: dict[str, list[str]] """


def get_area_project_map(proj_json):
  """
  Get projects per "area"
  :param build_args
  :return: dictionary of "area" to list of *.package paths
  :rtype: dict[str, list[str]]
  """
  global _area_projects
  global _area_areas

  if proj_json not in _area_projects:
    import json

    with open(proj_json, 'rt') as f:
      content = f.read()

      content = '\n'.join(line for line in content.splitlines() if not line.strip().startswith('//'))
      area_json_orig = json.loads(content)

      area_json = {}
      for area, contents in area_json_orig.items():
        for subarea in area.split('|'):
          area_json[subarea] = list(contents)

      # resolve areas first (entries with no punctuation)
      # NOTE: these are not sorted, so we need to visit until
      # all areas are resolved

      area_map = {}
      while True:
        retry_areas = []

        for name, entries in area_json.items():
          areas = []
          for entry in entries:
            # gather only areas (not paths)
            if '/' not in entry and '.' not in entry:
              if entry in area_map:
                areas.append(entry)
              elif entry in area_json:
                retry_areas.append(entry)
              else:
                raise AssertionError("unknown areas referenced in {}: {}".format(area_map_file, entry))

          area_map[name] = areas

        if not retry_areas:
          break

      _area_areas = area_map

      # recurse and find .packages
      area_project_map = {}

      basedir = os.path.dirname(proj_json)
      for area, entries in area_json.items():
        area_projects = []
        for entry in entries:
          # gather only paths (not areas)
          if '.' in entry or '/' in entry:
            full = os.path.join(basedir, entry)
            if os.path.isfile(full):
              area_projects.append(full)
            else:
              for dirpath, dirnames, filenames in os.walk(full):
                area_projects += [os.path.normpath(os.path.join(dirpath, f)) for f in filenames
                                  if f.endswith(".package")]

        area_projects.sort()
        area_project_map[area] = area_projects

      _area_projects[proj_json] = area_project_map

  return _area_projects[proj_json]


def get_area_projects(build_args):
  """
  Get projects for the "areas" configured for the build
  :param build_args
  :return: list of *.package (or *.mabu) paths, filtered by area.
  :rtype: list[str]
  """
  proj_map = get_area_project_map(build_args.project)

  if build_args.areas:
    areas = build_args.areas
  else:
    areas = ['all']

  projects = set()

  all_areas = list(areas)

  for area in areas:
    if area not in proj_map:
      print("*** no such area: " + area, file=sys.stderr)
    else:
      all_areas += _area_areas[area]

  for area in all_areas:
    projects.update(set(proj_map.get(area, [])))

  return projects


def find_files(d, functor):
  bins = []
  for dirpath, dirnames, filenames in os.walk(d):
    if is_non_cli_binary_path(dirpath):
      # don't recurse
      dirnames[:] = []
      continue
    for file in filenames:
       full = os.path.join(dirpath, file)
       if functor(full):
         bins.append(full)
  return bins


def is_non_cli_binary_path(path):
  """ Detect if directory searches descend into non-CLI tools """
  return 'uifrontend' in path.lower() or 'unity' in path.lower()


def is_elf(path):
  with open(path, 'rb') as f:
    sig = f.read(4)
    return sig == b'\x7fELF'


def is_macho(path):
  with open(path, 'rb') as f:
    sig = f.read(4)
    return sig == b'\xCF\xFA\xED\xFE'


if sys.platform == 'linux':
  def is_executable(path):
    ext = os.path.splitext(path)[1]
    return ext == "" and is_elf(path)

elif sys.platform == 'darwin':
  def is_executable(path):
    ext = os.path.splitext(path)[1]
    return ext == "" and is_macho(path)

else:
  def is_executable(path):
    ext = os.path.splitext(path)[1].lower()
    return ext == '.exe'


def fix_permissions(distdir):
  print_step("Correcting file permissions...")

  exes = find_files(distdir, is_executable)

  for exe in exes:
    if is_verbose:
      print("...", exe)
    os.chmod(exe, 0o777)

  print()


class AreaParserAction(argparse.Action):
  def __init__(self, option_strings, dest, nargs=None, **kwargs):
    if nargs is not None:
      raise ValueError("nargs not allowed")
    super(AreaParserAction, self).__init__(option_strings, dest, **kwargs)

  def __call__(self, parser, namespace, area, option_string=None):
    areas = [x.strip() for x in area.strip("'\"").split(',')]
    setattr(namespace, self.dest, (getattr(namespace, self.dest) or []) + areas)


def register_common_args(parser):
  parser.add_argument('-v', '--verbose', action='count', default=0, dest='verbose',
                      help='''increase verbosity incrementally''')

  parser.add_argument('--targets', nargs='*', default=[
                      'ml1_clang-8', 'host'], help='a list of space separated targets, in the same format as mabu\'s --target argument, for which to build the samples for')


def canonicalize_args(build_args):
  global is_verbose

  # global use
  is_verbose = build_args.verbose > 0
  if build_args.verbose > 1:
    build_args.mabu_args.append("-v")

  # avoid repeated lookups
  build_args.mabu = find_mabu()
  build_args.mlsdk = find_mlsdk()

  # get the default areas
  if hasattr(build_args, 'areas') and not build_args.areas:
      build_args.areas = ['all']


if __name__ == "__main__":
  print("Silly human.  This script does nothing on its own.  Look elsewhere.")
