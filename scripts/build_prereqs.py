#!/usr/bin/python

from __future__ import print_function
from subprocess import Popen, PIPE
from only_common import find_mlsdk
from distutils.dir_util import copy_tree
import argparse
import os
import platform
import sys

win = platform.system() == 'Windows'
osx = platform.system() == 'Darwin'
linux = platform.system() == 'Linux'

plat = "undefined"
if win:
  plat = "win64"
elif osx:
  plat = "osx"
elif linux:
  plat = "linux64"

def main():
  if plat == "undefined":
    print("FATAL: Failed to detect platform. Exiting")
    exit(1)

  parser = argparse.ArgumentParser(description='Script to build mlsdk stubs and assemble a mlsdk.')
  parser.add_argument('--no-prod', dest='copy_prod', default=True, action='store_false', help='Copy production libs & headers.')
  parser.add_argument('--no-staging', dest='copy_staging', default=True, action='store_false', help='Copy staging libs & headers.')
  args = parser.parse_args()

  mabuVersionResult = run([
    'mabu',
    '--version'
  ])

  if mabuVersionResult[0] is not 0:
    print ("! Error: mabu not available at command line. Run 'envsetup.sh' or make sure a LuminSDK install is available on your PATH")
    exit(1)

  mlsdk = find_mlsdk()
  print("> **WARNING** Modifying MLSDK located here MLSDK=" + mlsdk)

  filePath = os.path.dirname(os.path.realpath(__file__))

  # Copy prod and staging API headers
  if args.copy_prod:
    copy_tree(os.path.join(filePath, "../../../ml_api/include"), os.path.join(mlsdk, "include"))
  if args.copy_staging:
    copy_tree(os.path.join(filePath, "../../../ml_api/staging/include/staging"), os.path.join(mlsdk, "include", "staging"))

  # Build the stubs
  stubsPath = os.path.join(filePath, "..", "stubs")
  stubsInstallPath = os.path.join(stubsPath, "BUILD", "install")

  baseCmd = [
    'python',
    'build.py',
    '--staging'
  ]

  print("> Building stubs. cwd={} cmd={}".format(stubsPath, baseCmd))
  rawResults = run(baseCmd, stubsPath)
  if rawResults[0] != 0:
    print (rawResults[2])
    exit(rawResults[0])

  baseCmd = [
    'python',
    'assemble.py',
    '-m',
    mlsdk,
    '-s',
    stubsInstallPath
  ]

  if not args.copy_prod:
    baseCmd.append('--no-prod')
  if not args.copy_staging:
    baseCmd.append('--no-staging')

  print("> Running assemble.py. cwd={} cmd={}".format(stubsPath, baseCmd))
  rawResults = run(baseCmd, stubsPath)

  exit(0)

def run(cmd, cwd='.'):
  p = Popen(' '.join(cmd), shell=True, stderr=sys.stderr, stdout=sys.stdout, universal_newlines=True, cwd=cwd)
  stdout, stderr = p.communicate()
  return (p.returncode, stdout, stderr)

if __name__ == "__main__":
  main()
