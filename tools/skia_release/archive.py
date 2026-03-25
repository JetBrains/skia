#! /usr/bin/env python3

import pathlib
import sys
import zipfile

import common


def parents(path):
  res = []
  parent = path.parent
  while str(parent) != '.':
    res.insert(0, parent)
    parent = parent.parent
  return res


def main():
  skia_dir = common.skia_dir()

  build_type = common.build_type()
  version = common.version()
  machine = common.machine()
  target = common.target()
  classifier = common.classifier()
  out_bin = 'out/' + build_type + '-' + target + '-' + machine

  globs = [
      out_bin + '/gen/third_party/dawn/include',
      out_bin + '/*.a',
      out_bin + '/*.lib',
      out_bin + '/icudtl.dat',
      'include/**/*',
      'modules/particles/include/*.h',
      'modules/skottie/include/*.h',
      'modules/skottie/src/*.h',
      'modules/skottie/src/animator/*.h',
      'modules/skottie/src/effects/*.h',
      'modules/skottie/src/layers/*.h',
      'modules/skottie/src/layers/shapelayer/*.h',
      'modules/skottie/src/text/*.h',
      'modules/skparagraph/include/*.h',
      'modules/skplaintexteditor/include/*.h',
      'modules/skresources/include/*.h',
      'modules/sksg/include/*.h',
      'modules/skshaper/include/*.h',
      'modules/skshaper/src/*.h',
      'modules/svg/include/*.h',
      'modules/skcms/src/*.h',
      'modules/skcms/*.h',
      'modules/skunicode/include/*.h',
      'modules/skunicode/src/*.h',
      'modules/jsonreader/*.h',
      'src/base/*.h',
      'src/core/*.h',
      'src/gpu/ganesh/gl/*.h',
      'src/utils/*.h',
      'third_party/externals/angle2/LICENSE',
      'third_party/externals/angle2/include/**/*',
      'third_party/externals/freetype/docs/FTL.TXT',
      'third_party/externals/freetype/docs/GPLv2.TXT',
      'third_party/externals/freetype/docs/LICENSE.TXT',
      'third_party/externals/freetype/include/**/*',
      'third_party/externals/icu/source/common/**/*.h',
      'third_party/externals/libpng/LICENSE',
      'third_party/externals/libpng/*.h',
      'third_party/externals/libwebp/COPYING',
      'third_party/externals/libwebp/PATENTS',
      'third_party/externals/libwebp/src/dec/*.h',
      'third_party/externals/libwebp/src/dsp/*.h',
      'third_party/externals/libwebp/src/enc/*.h',
      'third_party/externals/libwebp/src/mux/*.h',
      'third_party/externals/libwebp/src/utils/*.h',
      'third_party/externals/libwebp/src/webp/*.h',
      'third_party/externals/harfbuzz/COPYING',
      'third_party/externals/harfbuzz/src/*.h',
      'third_party/externals/swiftshader/LICENSE.txt',
      'third_party/externals/swiftshader/include/**/*',
      'third_party/externals/zlib/LICENSE',
      'third_party/externals/zlib/*.h',
      'third_party/icu/*.h',
  ]

  dist = 'Skia-' + version + '-' + target + '-' + build_type + '-' + machine + classifier + '.zip'
  print('> Writing', dist)

  with zipfile.ZipFile(skia_dir / dist, 'w', compression=zipfile.ZIP_DEFLATED) as zip_file:
    dirs = set()
    files = set()
    for glob in globs:
      for path in pathlib.Path(skia_dir).glob(glob):
        if path.is_dir():
          continue
        relative_path = path.relative_to(skia_dir)
        if relative_path in files:
          continue
        for directory in parents(relative_path):
          if directory not in dirs:
            zip_file.write(skia_dir / directory, arcname=str(directory))
            dirs.add(directory)
        zip_file.write(path, arcname=str(relative_path))
        files.add(relative_path)

  return 0


if __name__ == '__main__':
  sys.exit(main())
