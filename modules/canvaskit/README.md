# Prerequisites

Node v14 or later is required to run tests. We use npm (the Node Package Manager) to install
test dependencies. Recent installations of Node have npm as well.
CanvasKit has no other external source dependencies.

## Compiling with GN
To build with GN, you need to have followed the instructions to download Skia and its deps
<https://skia.org/docs/user/download>.

To compile CanvasKit for WASM, you need to install [wasi-sdk][1] and set the `skia_wasm_sdk`
GN argument to point to the wasi-sdk installation directory.
For other available arguments, see `//modules/canvaskit/BUILD.gn`.

[1]: https://github.com/WebAssembly/wasi-sdk

# Compile and Run Local Example

```
# The following installs all npm dependencies and only needs to be when setting up
# or if our npm dependencies have changed (rarely).
npm ci

make release  # make debug is much faster and has better error messages
make local-example
```

This will print a local endpoint for viewing the example.  You can experiment
with the CanvasKit API by modifying `./npm_build/example.html` and refreshing
the page. For some more experimental APIs, there's also `./npm_build/extra.html`.

If CanvasKit fails to build and you are getting compile errors that don't look like Skia code,
you may need to do a fresh install of the npm modules. You can do this by finding the .dts file
mentioned in the error message, deleting it, and rerunning `npm ci`.

If you're using the correct modules plus the latest supported typescript and it still fails,
the module versions listed in package.json may need to be updated as well.

# Unit tests, performance tests, and coverage.

To run unit tests and compute test coverage on a debug gpu build

```
make debug
make test-continuous
```

This reads karma.conf.js, and opens a Chrome browser and begins running all the test
in `test/` it will detect changes to the tests in that directory and automatically
run again, however it will automatically rebuild and reload CanvasKit. Closing the
chrome window will just cause it to re-opened. Kill the karma process to stop continuous
monitoring for changes.

The tests are run with whichever build of CanvasKit you last made. be sure to also
test with `release`, `debug_cpu`, and `release_cpu`. testing with release builds will
expose problems in closure compilation and usually forgotten externs.

## Coverage

Coverage will be automatically computed when running test-continuous locally. Note that
the results will only be useful when testing a debug build. Open
`coverage/<browser version>/index.html` For a summary and detailed line-by-line result.

## Measuring Performance

We use puppeteer to run a Chrome browser to gather performance data in a consistent way.
See `//tools/perf-canvaskit-puppeteer` for more.

## Adding tests

The tests in `tests/` are grouped into files by topic.
Within each file there are `describe` blocks further organizing the tests, and within those
`it()` functions which test particular behaviors. `describe` and `it` are jasmine methods
which can both be temporarily renamed `fdescribe` and `fit`. Which causes jasmine to only those.

We have also defined `gm` which is a method for defining a test which draws something to a canvas
that is shapshotted and reported to gold.skia.org, where you can compare it with the snapshot at
head.

## Testing from Gerrit

When submitting a CL in gerrit, click "choose tryjobs" and type CanvasKit to filter them.
select all of them, which at the time of this writing is four jobs, for each combination
of perf/test gpu/cpu.

The performance results are reported to [perf.skia.org] and correctness results are reported to
[gold.skia.org].

Coverage is not measured while running tests this way.

# Inspecting output WASM

The `wasm2wat` tool from [the WebAssembly Binary Toolkit](https://github.com/WebAssembly/wabt)
can be used to produce a human-readable text version of a `.wasm` file.

The output of `wasm2wat --version` should be `1.0.13 (1.0.17)`. This version has been checked to
work with the tools in `wasm_tools/SIMD/`. These tools programmatically inspect the `.wasm` output
of a CanvasKit build to detect the presence of [wasm SIMD](https://github.com/WebAssembly/simd)
operations.

# Infrastructure Playbook

When dealing with CanvasKit in our CI, we use Docker. Check out
$SKIA_ROOT/infra/wasm-common/docker/README.md for more on building/editing the
images used for building and testing.

## Running Skia's GMs and Unit Tests against wasm+WebGL ##

General Tips:
 - Make use of the skip lists and start indexes in the run-wasm-gm-tests.html to focus in on
   problematic tests.
 - `Uncaught (in promise) RuntimeError: function signature mismatch` tends to mean null was
   dereferenced somewhere. Add SkASSERT to verify.

### Debugging some GMs / Unit Tests
For faster cycle time, it is recommended to focus on specific GMs instead of re-compiling all
of them.

### Testing all GMs / Unit Tests
Change directory to `//tools/run-wasm-gm-tests`. Run `make run_local`, which will put all PNGs
produced by GMs into `/tmp/wasm-gmtests` and run all unit tests.
