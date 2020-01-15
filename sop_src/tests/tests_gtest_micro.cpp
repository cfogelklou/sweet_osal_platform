#include "tests/embedded_tests/gtest_micro/include/gtest/gtest.h"
#include "common_src/utils/ble_log.h"

//TEST(GTestMicroTest1, Trivial) {
    // TODO... Fill me in!
//}

LOG_MODNAME("tests_gtest_micro.cpp");

namespace gtestmicro {
    class GTestMicroTest1 : public GTestBase {
    public:
        GTestMicroTest1() : GTestBase("GTestMicroTest1", "runTest" ) {}

        void runTest(){
            LOG_Log("Hello!\r\n");
        }

    };
}


static gtestmicro::GTestMicroTest1 dummy1;

void test_gtest(void)
{
    gtestmicro::GTestMicroSingleton::inst().runAllTests();
}
