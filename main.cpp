#include "tests.h"


using namespace std;

int main() {
    TestRunner tr;
//    RUN_TEST(tr, TestSerpFormat);
//    RUN_TEST(tr, TestTop5);
//    RUN_TEST(tr, TestHitcount);
//    RUN_TEST(tr, TestRanking);
//    RUN_TEST(tr, TestBasicSearch);
    RUN_TEST(tr, TestMultithreading);

    return 0;
}
