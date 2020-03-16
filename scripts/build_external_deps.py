#!/usr/bin/python

from __future__ import print_function
from only_common import find_mlsdk
from distutils.dir_util import copy_tree
from subprocess import run
import argparse
import os
import sys

def main():
  filePath = os.path.dirname(os.path.realpath(__file__))
  appFrameworkPath = os.path.join(filePath, "..", "app_framework")

  baseCmd = [
    sys.executable,
    'get_deps.py',
  ]

  print("> Getting external dependencies. cwd={} cmd={}".format(appFrameworkPath, baseCmd))
  run(baseCmd, cwd=appFrameworkPath, check=True)

  baseCmd = [
    sys.executable,
    'build_external.py',
  ] + sys.argv[1:]

  print("> Building external dependencies. cwd={} cmd={}".format(appFrameworkPath, baseCmd))
  run(baseCmd, cwd=appFrameworkPath, check=True)

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Script to build external dependencies of tests.')
  args = parser.parse_known_args()
  main()
