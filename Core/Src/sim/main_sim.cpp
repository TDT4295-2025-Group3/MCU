#include <cstdlib>
#include <iostream>

#include "game.hpp"
#include "host_input_timer.hpp"
#include "host_rasterizer.hpp"
#include <hidapi.h>

int main() {
    std::unique_ptr<IInput> input;
    HostRasterizer rasterizer{320, 240};
    HostTimer timer;

    std::unique_ptr<DS4Driver> ds4_controller;
    bool useDS4 = true;

    int hid_res = hid_init();
    if (hid_res == -1) {
        std::cerr << "Failed to initialize hid." << std::endl;
        useDS4 = false;
    }

    hid_device* handle;

    handle = hid_open(0x054C, 0x09CC, nullptr);
    if (!handle) {
        std::cerr << "Failed to find DS4 controller. Falling back to keyboard input." << std::endl;
        useDS4 = false;
    }
    else {
        printf("Using DS4 controller for input.\n");

        wchar_t wstr[255];
        hid_get_manufacturer_string(handle, wstr, 255);
        printf("Manufacturer String: %ls\n", wstr);

        hid_get_product_string(handle, wstr, 255);
        printf("Product String: %ls\n", wstr);

        hid_get_serial_number_string(handle, wstr, 255);
        printf("Serial Number String: (%d) %ls\n", wstr[0], wstr);

        hid_set_nonblocking(handle, 1);
    }

    if (useDS4) {
        ds4_controller = std::make_unique<DS4Driver>();
        input = std::make_unique<DS4Input>(*ds4_controller);
    }else {
        input = std::make_unique<HostInput>();
    }

    Game game{rasterizer, *input, timer};
    game.init();

    uint8_t ds4_buffer[64];

    while (true) {
        if (ds4_controller) {
            // read input from DS4, push all outputs
            DS4_OutputUSBReport outputReport;
            while (ds4_controller->getReadyOutputReport(outputReport)) {
                hid_write(handle, reinterpret_cast<uint8_t *>(&outputReport), sizeof(DS4_OutputUSBReport));
            }

            hid_res = hid_read(handle, ds4_buffer, sizeof(ds4_buffer));
            if (hid_res == -1 || hid_res == 0) continue;
            auto report = reinterpret_cast<DS4_InputUSBReport *>(ds4_buffer);
            if (report->ReportID != 0x01) continue; // not an input report
            ds4_controller->processInput(*report);
        }
        game.tick_once();
    }

    if (handle) hid_close(handle);
    hid_exit();

    return 0;
}
