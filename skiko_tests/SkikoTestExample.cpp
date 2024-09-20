#include "SkikoTest.h"

DEF_TEST_SKIKO(TestExample, reporter) {
    bool isSkikoTest = true;
    REPORTER_ASSERT(reporter, isSkikoTest);
}