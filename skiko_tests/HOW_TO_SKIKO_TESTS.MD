### Skia tests
To read about Skia tests go to [testing.md](../site/docs/dev/testing/testing.md) and [tests.md](../site/docs/dev/testing/tests.md)

### How to run Skia tests for Skiko locally
1) Make sure to sync the dependencies: `python3 tools/git-sync-deps`
2) Install ninja: `python3 bin/fetch-ninja`
3) Build DM tool: `bin/gn gen out/Debug && ./third_party/ninja/ninja -C out/Debug dm`
4) Run the relevant tests: `./run_tests_for_skiko.sh` (it runs all the tests defined with `DEF_TEST_SKIKO`)

### How to add a new test
1) You can add a new test in any existing `.cpp` file in `skiko_tests` folder, or you can create a new `.cpp` file. Have a look at other files, for example [SkikoTestExample.cpp](./SkikoTestExample.cpp)
2) If you created a new file, make sure to add it to `skiko_tests_sources` in `skiko_tests.gni` file
3) The test should be defined using `DEF_TEST_SKIKO` available in `SkikoTests.h`

**Note:** 
It's possible to add new tests in other directories too (for example in `tests` with all other Skia tests). But it's important to use `DEF_TEST_SKIKO` to define the scope of the tests.
Keeping the tests in `skiko-tests` directory is probably preferred for our needs, because it simplifies the maintenance.

### GitHub action
We have a workflow running on every PR to `skiko` branch. It runs `skiko_tests`. It can be found [here](../.github/workflows/tests_for_skiko.yml)