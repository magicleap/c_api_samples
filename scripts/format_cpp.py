#!/usr/bin/env python3

import glob
import itertools
import os
import subprocess


def main():
    basedir = os.path.dirname(os.path.realpath(__file__)) or '.'
    paths = [
        os.path.join(basedir, '..', 'tests', '**', '*.cpp'),
        os.path.join(basedir, '..', 'tests', '**', '*.h'),
        os.path.join(basedir, '..', 'samples', '**', '*.cpp'),
        os.path.join(basedir, '..', 'samples', '**', '*.h'),
        os.path.join(basedir, '..', 'app_framework', 'include', '**', '*.cpp'),
        os.path.join(basedir, '..', 'app_framework', 'include', '**', '*.h'),
        os.path.join(basedir, '..', 'app_framework', 'src', '**', '*.cpp'),
        os.path.join(basedir, '..', 'app_framework', 'src', '**', '*.h'),
        os.path.join(basedir, '..', 'app_framework', 'tests', '**', '*.cpp'),
        os.path.join(basedir, '..', 'app_framework', 'tests', '**', '*.h'),
    ]
    all_cpp_files = map(lambda p: glob.iglob(p, recursive=True), paths)

    for filename in itertools.chain.from_iterable(all_cpp_files):
        subprocess.run(['clang-format', '-i', filename], check=True)


if __name__ == "__main__":
    main()
