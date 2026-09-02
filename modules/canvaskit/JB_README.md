# JetBrains CanvasKit Notes

## WASM Builds

JetBrains WASM release builds use [wasi-sdk][1]. Install wasi-sdk and set the
`skia_wasm_sdk` GN argument to the wasi-sdk installation directory.

When using the release helper, pass the SDK path with `--wasi-sdk`, for example:

    python3 tools/skia_release/build.py --skia-dir . --target wasm --machine wasm --build-type Release --gpu-as-extension --enable-graphite-dawn --wasi-sdk /opt/wasi-sdk-33.0-arm64-macos/

[1]: https://github.com/WebAssembly/wasi-sdk
