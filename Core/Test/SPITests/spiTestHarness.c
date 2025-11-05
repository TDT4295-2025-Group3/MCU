#include "unity.h"

extern void run_spi_tests(void);

void setUp(void) {}
void tearDown(void) {}

int spi_test_main(void)
{
    UNITY_BEGIN();
    run_spi_tests();
    return UNITY_END();
}