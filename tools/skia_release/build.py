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


def generate_dawn_headers_for_wasm(skia_dir, out_dir):
  """Generate Dawn headers using CMake for wasm builds.

  Dawn's CMake build is skipped for wasm in GN, but we still need the generated
  headers (e.g. dawn/webgpu_cpp.h). This runs CMake to build just the header
  generation targets.
  """
  cmake_exe = shutil.which('cmake')
  if not cmake_exe:
    raise Exception('cmake not found in PATH; needed to generate Dawn headers for wasm')

  ninja_exe = shutil.which('ninja')
  if not ninja_exe:
    # Try Skia's bundled ninja
    bundled_ninja = os.path.join(skia_dir, 'third_party', 'ninja', 'ninja')
    if os.path.isfile(bundled_ninja):
      ninja_exe = bundled_ninja
    else:
      raise Exception('ninja not found in PATH; needed to generate Dawn headers for wasm')

  dawn_dir = os.path.join(skia_dir, 'third_party', 'externals', 'dawn')
  build_dir = os.path.join(out_dir, 'cmake_dawn_headers')
  gen_dir = os.path.join(out_dir, 'gen', 'third_party', 'dawn')

  # Import get_third_party_locations from Dawn's cmake_utils
  dawn_scripts = os.path.join(skia_dir, 'third_party', 'dawn')
  sys.path.insert(0, dawn_scripts)
  from cmake_utils import get_third_party_locations
  sys.path.pop(0)

  configure_cmd = [
      cmake_exe,
      '-S', dawn_dir,
      '-B', build_dir,
      '-G', 'Ninja',
      '-DCMAKE_MAKE_PROGRAM=' + ninja_exe,
      '-DDAWN_BUILD_MONOLITHIC_LIBRARY=OFF',
      '-DDAWN_BUILD_SAMPLES=OFF',
      '-DDAWN_BUILD_TESTS=OFF',
      '-DDAWN_BUILD_BENCHMARKS=OFF',
      '-DDAWN_ENABLE_D3D11=OFF',
      '-DDAWN_ENABLE_D3D12=OFF',
      '-DDAWN_ENABLE_METAL=OFF',
      '-DDAWN_ENABLE_VULKAN=OFF',
      '-DDAWN_ENABLE_OPENGLES=OFF',
      '-DDAWN_ENABLE_NULL=ON',
      '-DCMAKE_CXX_STANDARD=17',
      '-DCMAKE_CXX_FLAGS=-std=c++17',
      '-DDAWN_ENABLE_INSTALL=OFF',
      '-DTINT_ENABLE_INSTALL=OFF',
      '-DTINT_BUILD_HLSL_WRITER=OFF',
  ] + get_third_party_locations()

  print('> Generating Dawn headers for wasm')
  subprocess.check_call(configure_cmd)
  # Build only the header generation targets
  subprocess.check_call([ninja_exe, '-C', build_dir, 'dawn_headers', 'dawncpp_headers', 'webgpu_headers_gen'])

  # Copy generated headers to where GN expects them
  generated_headers_src = os.path.join(build_dir, 'gen', 'include')
  generated_headers_dest = os.path.join(gen_dir, 'include')

  if os.path.exists(generated_headers_dest):
    shutil.rmtree(generated_headers_dest)

  shutil.copytree(
      os.path.join(generated_headers_src, 'dawn'),
      os.path.join(generated_headers_dest, 'dawn'),
      dirs_exist_ok=True)
  shutil.copytree(
      os.path.join(generated_headers_src, 'webgpu'),
      os.path.join(generated_headers_dest, 'webgpu'),
      dirs_exist_ok=True)


  return os.path.abspath(generated_headers_dest)


def main():
  skia_dir = common.skia_dir()
  os.chdir(skia_dir)
  prepare_skia_checkout(skia_dir)

  build_type = common.build_type()
  machine = common.machine()
  host = common.host()
  target = common.target()
  ndk = common.ndk()
  wasi_sdk = common.wasi_sdk()
  gpu_as_extension = common.gpu_as_extension()
  if target == 'wasm':
    # WASM release packages emit Ganesh as a separate extension library.
    gpu_as_extension = True
  enable_ganesh = common.enable_ganesh()
  enable_graphite = common.enable_graphite()
  enable_graphite_dawn = common.enable_graphite_dawn()

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
      'extra_ldflags=[]',
  ]

  if target == 'windows':
    args += ['extra_cflags+=["/clang:-fvisibility=default"]']
  else:
    args += ['extra_cflags+=["-fvisibility=default"]']

  if is_ios or is_tvos:
    args += [
        'skia_icu_data_filter="//third_party/externals/icu/filters/ios.json"',
        'skia_icu_data_filter_patch="//third_party/icu/skiko_ios/filter.patch"'
    ]

  if is_macos or is_ios or is_tvos:
    if is_macos:
      args += ['skia_use_fonthost_mac=true']
      if enable_graphite_dawn:
        args += ['dawn_enable_metal=true']
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
    args += ['skia_use_vulkan=true']
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
    if enable_graphite_dawn:
      args += ['dawn_enable_d3d11=true', 'dawn_enable_d3d12=true']
    args += [
        'skia_use_vulkan=true',
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
    if not wasi_sdk:
      raise Exception('--wasi-sdk is required for wasm builds')
    if enable_graphite_dawn:
      args += ['skia_use_webgpu=true']
    sysroot = os.path.abspath(os.path.join(wasi_sdk, 'share', 'wasi-sysroot'))
    gl_headers = os.path.abspath(os.path.join(skia_dir, 'third_party/externals/opengl-registry/api'))
    egl_headers = os.path.abspath(os.path.join(skia_dir, 'third_party/externals/egl-registry/api'))
    dawn_headers = os.path.abspath(os.path.join(skia_dir, 'third_party/externals/dawn/include'))
    dawn_root = os.path.abspath(os.path.join(skia_dir, 'third_party/externals/dawn'))
    dawn_gen_headers = ''
    out_dir = os.path.join('out', build_type + '-' + target + '-' + machine)
    if enable_graphite_dawn:
      dawn_gen_headers = generate_dawn_headers_for_wasm(skia_dir, out_dir)
    args += [
        'skia_use_dng_sdk=false',
        'skia_use_freetype=true',
        'skia_use_freetype_woff2=true',
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
        'skia_gl_standard="webgl"',
        'skia_use_gl=true',
        'skia_enable_svg=true',
        'skia_use_expat=true',
        'extra_cflags_cc+=["-std=c++20"]',
        'skia_enable_optimize_size=' + ('true' if build_type == 'Release' else 'false'),
        'skia_wasm_sdk="' + wasi_sdk + '"',
        'extra_cflags+=["--target=wasm32-wasip1", "-flto=thin", "--sysroot=' + sysroot + '", "-I' + gl_headers + '", "-I' + egl_headers + '", "-I' + dawn_headers + '", "-I' + dawn_root + '", "-I' + dawn_gen_headers + '", "-mllvm", "-wasm-enable-sjlj", "-mexception-handling", "-D_WASI_EMULATED_MMAN", "-D_WASI_EMULATED_SIGNAL", "-D_WASI_EMULATED_PROCESS_CLOCKS", "-D_WASI_EMULATED_GETPID", "-DU_HAVE_TZSET=0", "-DU_HAVE_TIMEZONE=0", "-DU_HAVE_TZNAME=0"]',
        'extra_cflags_cc+=["--target=wasm32-wasip1", "--sysroot=' + sysroot + '", "-I' + gl_headers + '", "-I' + egl_headers + '", "-I' + dawn_headers + '", "-I' + dawn_root + '", "-I' + dawn_gen_headers + '", "-mllvm", "-wasm-enable-sjlj", "-mexception-handling", "-D_WASI_EMULATED_MMAN", "-D_WASI_EMULATED_SIGNAL", "-D_WASI_EMULATED_PROCESS_CLOCKS", "-D_WASI_EMULATED_GETPID", "-DU_HAVE_TZSET=0", "-DU_HAVE_TIMEZONE=0", "-DU_HAVE_TZNAME=0"]',
        'extra_ldflags+=["--target=wasm32-wasip1", "-flto=thin", "-Wl,--gc-sections", "-Wl,--strip-all", "--sysroot=' + sysroot + '", "-lsetjmp", "-lwasi-emulated-mman", "-lwasi-emulated-signal", "-lwasi-emulated-process-clocks", "-lwasi-emulated-getpid", "-mllvm", "-wasm-enable-sjlj", "-mexception-handling"]',
    ]

  if gpu_as_extension:
    args += ['skia_gpu_as_extension=true']
  if not enable_ganesh:
    args += ['skia_enable_ganesh=false']
  if enable_graphite or enable_graphite_dawn:
    args += ['skia_enable_graphite=true']
  if enable_graphite_dawn:
    args += ['skia_use_dawn=true']

  out = os.path.join('out', build_type + '-' + target + '-' + machine)
  gn = 'gn.exe' if host == 'windows' else 'gn'
  gn_cmd = [os.path.join('bin', gn), 'gen', out, '--args=' + ' '.join(args)]
  subprocess.check_call(gn_cmd)
  ninja_targets = ['skia', 'modules']
  if gpu_as_extension:
    if enable_ganesh:
        ninja_targets.append('skia_ganesh_ext')
    if enable_graphite:
        ninja_targets.append('skia_graphite_ext')
    if enable_graphite_dawn:
        ninja_targets.append('skia_graphite_dawn_ext')

  subprocess.check_call([ninja, '-C', out] + ninja_targets)
  return 0


if __name__ == '__main__':
  sys.exit(main())
