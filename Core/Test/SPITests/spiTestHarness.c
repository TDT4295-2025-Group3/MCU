#include "unity.h"
#include "device/seven_seg_display_c.h"

extern void run_spi_tests(void);

void setUp(void) {}
void tearDown(void) {}

int spi_test_main(void)
{
    SevenSeg_Init();

    UNITY_BEGIN();
    run_spi_tests();
    const int failures = UNITY_END();  // Unity returns the number of failed tests

    if (failures > 0)
    {
        const uint16_t to_show = (failures > 9999) ? 9999U : (uint16_t)failures;
        // Display the number of failed tests + -F to indicate failure
        char buffer[4];
        snprintf(buffer, sizeof(buffer), "%2u", to_show);
        strcat(buffer, " L");
        SevenSeg_DisplayString(buffer);  // display how many failed
    }
    else
    {
        SevenSeg_DisplayNumber(1111U);    // keep success pattern (optional)
    }
    HAL_Delay(2000U); // wait 2 seconds before exiting to allow reading the display
    return failures;
}