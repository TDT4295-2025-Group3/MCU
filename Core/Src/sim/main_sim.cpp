#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

#include "game.hpp"
#include "host_input_timer.hpp"
#include "host_rasterizer.hpp"
#include "hid_driver.hpp"
#include <hidapi.h>

int main()
{
    HostInput keyboardInput;
    std::unique_ptr<DS4Driver> ds4Controller;
    std::unique_ptr<DS4Input> ds4Input;
    IInput *activeInput = &keyboardInput;
    HostRasterizer rasterizer{800, 600};
    HostTimer timer;

    std::string modelBasePath;
    bool basePathFromEnv = false;

    if (const char *envPath = std::getenv("MCU_MODEL_PATH"); envPath && envPath[0] != '\0')
    {
        modelBasePath = envPath;
        basePathFromEnv = true;
    }
    else
    {
        const std::filesystem::path cwd = std::filesystem::current_path();
        const std::array<std::filesystem::path, 3> candidates = {
            cwd / "models",
            cwd / "../models",
            cwd / "../../models"};

        for (const auto &candidate : candidates)
        {
            std::error_code ec;
            if (std::filesystem::is_directory(candidate, ec) && !ec)
            {
                const auto resolved = std::filesystem::weakly_canonical(candidate, ec);
                if (!ec)
                {
                    modelBasePath = resolved.string();
                    break;
                }
            }
        }
    }

    bool useDS4 = true;

    int hid_res = hid_init();
    if (hid_res == -1)
    {
        std::cerr << "Failed to initialize hid." << std::endl;
        useDS4 = false;
    }

    hid_device *handle;

    handle = hid_open(0x054C, 0x09CC, nullptr);
    if (!handle)
    {
        std::cerr << "Failed to find DS4 controller. Falling back to keyboard input." << std::endl;
        useDS4 = false;
    }
    else
    {
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

    if (useDS4)
    {
        ds4Controller = std::make_unique<DS4Driver>();
        ds4Input = std::make_unique<DS4Input>(*ds4Controller);
        activeInput = ds4Input.get();
        ds4Controller->queueInitReport();
    }

    Game game{rasterizer, *activeInput, timer, false};
    game.init();

    uint8_t ds4_buffer[64];

    while (true)
    {
        if (ds4Controller)
        {
            // read input from DS4, push all outputs
            DS4_OutputUSBReport outputReport;
            while (ds4Controller->getReadyOutputReport(outputReport))
            {
                hid_write(handle, reinterpret_cast<uint8_t *>(&outputReport), sizeof(DS4_OutputUSBReport));
            }

            hid_res = hid_read(handle, ds4_buffer, sizeof(ds4_buffer));
            if (hid_res == -1 || hid_res == 0)
                continue;
            auto report = reinterpret_cast<DS4_InputUSBReport *>(ds4_buffer);
            if (report->ReportID != 0x01)
                continue; // not an input report
            ds4Controller->processInput(*report);
        }
        game.tick_once();
    }

    if (handle)
        hid_close(handle);
    hid_exit();

    return 0;
}
