#!/usr/bin/env python3
import argparse
import zipfile, urllib.request, shutil
import os
import sys
import platform
import csv

vi = sys.version_info
if platform.system() == 'Darwin':
  if (vi.major, vi.minor) < (3, 7):
    print("Please run this script with Python 3.7 or newer")
    exit(1)
else:
  if (vi.major, vi.minor) < (3, 5):
    print("Please run this script with Python 3.5 or newer")
    exit(1)

def register_args(parser):
  parser.add_argument('--use-artifactory', '-ua', action='store_true',
                      help='''use zips from artifactory for jenkins build''')

def parse_args():
    parser = argparse.ArgumentParser()
    register_args(parser)
    build_args = parser.parse_args()
    return build_args

if __name__ == '__main__':

    build_args = parse_args()

    deps = []
    deps_file = 'external_deps.csv'
    if build_args.use_artifactory:
      deps_file = 'external_deps.jenkins.csv'

    with open(deps_file) as csv_file:
      csv_reader = csv.reader(csv_file, delimiter=',')
      for row in csv_reader:
        deps.append((row[0], row[1], row[2], row[3]))

    for dep in deps:
        print("Downloading {0} : {1}".format(dep[0], dep[2]))
        with urllib.request.urlopen(dep[2]) as response, open(dep[1], 'wb') as out_file:
            shutil.copyfileobj(response, out_file)

        if os.path.isfile(os.path.join(os.getcwd(), dep[1])):
            shutil.unpack_archive(dep[1], dep[3])
            os.remove(dep[1])
