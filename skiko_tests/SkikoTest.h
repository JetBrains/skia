#ifndef SKIKOTEST_H
#define SKIKOTEST_H

#include "tests/Test.h"

#define DEF_TEST_SKIKO(name, reporter) DEF_CONDITIONAL_TEST(Skiko_##name, reporter, true)

#endif //SKIKOTEST_H
