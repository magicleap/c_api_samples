####################################################################################
# Build and test native sample apps
#
# Run with -help for info.
####################################################################################
import os
import sys

vi = sys.version_info
if vi.major < 3 or (vi.major == 3 and vi.minor < 5):
  print("Please run this script with Python 3.5 or newer")
  exit(1)

sys.path.insert(0, '../scripts')

from build_project import parse_args, build

if __name__ == "__main__":
  base = os.path.dirname(os.path.realpath( __file__)) or '.'
  dist = os.path.join(base, "dist", "sdk_native_samples")
  packages = [os.path.join(base, "sdk_native_samples.package")]
  project = os.path.join(base, "project_areas.json")

  build_args = parse_args(project)

  build(build_args, base_dir = base, project = project, dist_dir = dist, packages = packages)
