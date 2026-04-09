#! /usr/bin/env python3

import os
import shutil
import subprocess
import sys
import time

import common


def git_sync_with_retries(skia_dir, max_retries=3, backoff_seconds=5):
  attempt = 0
  while True:
    try:
      print("> Running tools/git-sync-deps (attempt {}/{})".format(attempt + 1, max_retries + 1))
      if common.host() == 'windows':
        env = os.environ.copy()
        env['PYTHONHTTPSVERIFY'] = '0'
        subprocess.check_call([sys.executable, "tools/git-sync-deps"], cwd=skia_dir, env=env)
      else:
        subprocess.check_call([sys.executable, "tools/git-sync-deps"], cwd=skia_dir)
      print("Success")
      return
    except subprocess.CalledProcessError as error:
      attempt += 1
      if attempt > max_retries:
        print("All {} retries failed. Giving up.".format(max_retries))
        raise
      wait = backoff_seconds * attempt
      print(f"Failed (exit {error.returncode}), retrying in {wait}s...")
      time.sleep(wait)


def patch_windows_toolchain(skia_dir):
  toolchain_path = skia_dir / "gn" / "toolchain" / "BUILD.gn"
  with toolchain_path.open("r", encoding="utf-8") as toolchain_file:
    contents = toolchain_file.read()

  patched = contents.replace(
      'shell = "cmd.exe /c',
      'shell = "cmd.exe /v:on /c',
  ).replace(
      r'env_setup = "$shell set \"PATH=%PATH%',
      r'env_setup = "$shell set \"PATH=!PATH!',
  )

  if patched != contents:
    with toolchain_path.open("w", encoding="utf-8") as toolchain_file:
      toolchain_file.write(patched)


def prepare_skia_checkout(skia_dir):
  print("> Running tools/git-sync-deps")
  git_sync_with_retries(skia_dir)

  print("> Fetching ninja")
  subprocess.check_call([sys.executable, "bin/fetch-ninja"], cwd=skia_dir)

  if common.host() == 'windows':
    patch_windows_toolchain(skia_dir)


def ninja_path(host):
  return os.path.join('third_party', 'ninja', 'ninja.exe' if host == 'windows' else 'ninja')


def main():
  skia_dir = common.skia_dir()
  os.chdir(skia_dir)
  prepare_skia_checkout(skia_dir)

  build_type = common.build_type()
  machine = common.machine()
  host = common.host()
  target = common.target()
  ndk = common.ndk()

  ninja = ninja_path(host)
  is_ios = target in ('ios', 'iosSim')
  is_tvos = target in ('tvos', 'tvosSim')
  is_ios_sim = target == 'iosSim'
  is_tvos_sim = target == 'tvosSim'
  is_macos = target == 'macos'

  if build_type == 'Debug':
    args = ['is_debug=true']
  else:
    args = ['is_official_build=true']

  args += [
      'target_cpu="' + machine + '"',
      'skia_use_system_expat=false',
      'skia_use_system_libjpeg_turbo=false',
      'skia_use_system_libpng=false',
      'skia_use_system_libwebp=false',
      'skia_use_system_zlib=false',
      'skia_use_system_freetype2=false',
      'skia_use_system_harfbuzz=false',
      'skia_pdf_subset_harfbuzz=true',
      'skia_use_system_icu=false',
      'skia_enable_skottie=true',
      'extra_cflags=[]',
      'extra_cflags_cc=[]',
  ]

  if is_macos or is_ios or is_tvos:
    if is_macos:
      args += ['skia_use_fonthost_mac=true']
    args += ['extra_cflags_cc+=["-frtti"]']
    args += ['skia_use_metal=true']
    if is_ios:
      args += ['target_os="ios"']
      if is_ios_sim:
        args += ['ios_use_simulator=true']
        args += ['extra_cflags+=["-mios-simulator-version-min=12.0"]']
      else:
        args += ['ios_min_target="12.0"']
    elif is_tvos:
      args += ['target_os="tvos"']
      if is_tvos_sim:
        args += ['ios_use_simulator=true']
        args += ['extra_cflags+=["-mtvos-simulator-version-min=14", "-DSK_BUILD_FOR_TVOS"]']
      else:
        args += ['extra_cflags+=["-mtvos-version-min=14", "-DSK_BUILD_FOR_TVOS"]']
    elif machine == 'arm64':
      args += ['extra_cflags+=["-stdlib=libc++"]']
    else:
      args += ['extra_cflags+=["-stdlib=libc++", "-mmacosx-version-min=10.13"]']
  elif target == 'linux':
    if machine == 'arm64':
      args += [
          'skia_gl_standard="gles"',
          'skia_use_egl=true',
          'extra_cflags_cc+=["-fno-exceptions", "-fno-rtti", "-D_GLIBCXX_USE_CXX11_ABI=0", "-mno-outline-atomics"]',
          'cc="gcc-10"',
          'cxx="g++-10"',
      ]
    else:
      args += [
          'extra_cflags_cc+=["-fno-exceptions", "-fno-rtti","-D_GLIBCXX_USE_CXX11_ABI=0"]',
          'cc="gcc-10"',
          'cxx="g++-10"',
      ]
  elif target == 'windows':
    args += [
        'skia_use_direct3d=true',
        'extra_cflags+=["-DSK_FONT_HOST_USE_SYSTEM_SETTINGS"]',
    ]
    if host == 'windows':
      clang_path = shutil.which('clang-cl.exe')
      if not clang_path:
        raise Exception(
          "Please install LLVM from https://releases.llvm.org/, "
            "and make sure that clang-cl.exe is available in PATH"
        )
      args += [
          'clang_win="' + os.path.dirname(os.path.dirname(clang_path)) + '"',
          'is_trivial_abi=false',
      ]
  elif target == 'android':
    args += [
        'ndk="' + ndk + '"',
        'skia_use_vulkan=true',
    ]
  elif target == 'wasm':
    args += [
        'skia_use_dng_sdk=false',
        'skia_use_libjpeg_turbo_decode=true',
        'skia_use_libjpeg_turbo_encode=true',
        'skia_use_libpng_decode=true',
        'skia_use_libpng_encode=true',
        'skia_use_libwebp_decode=true',
        'skia_use_libwebp_encode=true',
        'skia_use_wuffs=true',
        'skia_use_lua=false',
        'skia_use_webgl=true',
        'skia_use_piex=false',
        'skia_use_system_libpng=false',
        'skia_use_system_freetype2=false',
        'skia_use_system_libjpeg_turbo=false',
        'skia_use_system_libwebp=false',
        'skia_enable_tools=false',
        'skia_enable_fontmgr_custom_directory=false',
        'skia_enable_fontmgr_custom_embedded=true',
        'skia_enable_fontmgr_custom_empty=true',
        'skia_use_webgl=true',
        'skia_gl_standard="webgl"',
        'skia_use_gl=true',
        'skia_enable_gpu=true',
        'skia_enable_svg=true',
        'skia_use_expat=true',
        'extra_cflags+=["-DSK_SUPPORT_GPU=1", "-DSK_GL", "-DSK_DISABLE_LEGACY_SHADERCONTEXT", "-sSUPPORT_LONGJMP=wasm"]',
    ]

  out = os.path.join('out', build_type + '-' + target + '-' + machine)
  gn = 'gn.exe' if host == 'windows' else 'gn'
  gn_cmd = [os.path.join('bin', gn), 'gen', out, '--args=' + ' '.join(args)]
  print(gn_cmd)
  subprocess.check_call(gn_cmd)
  subprocess.check_call([ninja, '-C', out, 'skia', 'modules'])
  return 0


if __name__ == '__main__':
  sys.exit(main())
