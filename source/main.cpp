#include <stdio.h>
#include <stdlib.h>

#include <coreinit/dynload.h>
#include <coreinit/filesystem_fsa.h>
#include <coreinit/time.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <mocha/mocha.h>
#include <sysapp/title.h>
#include <vpad/input.h>
#include <whb/log_console.h>
#include <whb/log.h>
#include <whb/proc.h>

#define BUFFER_SIZE 0x80000

uint64_t menuTitleID = 0;
const char* nandFile = NULL;
const char* dumpFile = "/vol/external01/dump.app";

static bool checkIfTiramisu()
{
    OSDynLoad_Module mod;
    if (OSDynLoad_Acquire("homebrew_kernel", &mod) == OS_DYNLOAD_OK) {
        OSDynLoad_Release(mod);
        return true;
    }
    return false;
}

void printOnScreen(int line, const char* text)
{
    OSScreenPutFontEx(SCREEN_TV, 0, line, text);
    OSScreenPutFontEx(SCREEN_DRC, 0, line, text);
}

void printMainMenu()
{
    OSScreenClearBufferEx(SCREEN_TV, 0);
    OSScreenClearBufferEx(SCREEN_DRC, 0);

    printOnScreen(0, "vWii Theme Injector (v1.0.0)");
    printOnScreen(1, "Created by Nightkingale, on behalf of the UWUVCI-PRIME team");
    printOnScreen(2, "-----------------------------------------------------------");

    // Guess the Wii Menu region using the Wii U Menu installation.
    menuTitleID = _SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU);
    switch (menuTitleID)
    {
        case 0x0005001010040000:
            nandFile = "slccmpt01:/title/00000001/00000002/content/0000001c.app";
            printOnScreen(3, "Region Detected by Wii U Menu: JPN");
            printOnScreen(4, "Wii Menu Theme File: 0000001c.app");
            break;

        case 0x0005001010040100:
            nandFile = "slccmpt01:/title/00000001/00000002/content/0000001f.app";
            printOnScreen(3, "Region Detected by Wii U Menu: USA");
            printOnScreen(4, "Wii Menu Theme File: 0000001f.app");
            break;

        case 0x0005001010040200:
            nandFile = "slccmpt01:/title/00000001/00000002/content/00000022.app";
            printOnScreen(3, "Region Detected by Wii U Menu: EUR");
            printOnScreen(4, "Wii Menu Theme File: 00000022.app");
            break;
    }

    printOnScreen(5, "-----------------------------------------------------------");
    printOnScreen(6, "Press A to dump Wii System Menu assets.");
    printOnScreen(7, "Press B to restore Wii System Menu assets.");
    printOnScreen(8, "Press HOME to exit.");

    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
}

int main()
{
    OSScreenInit();
    WHBProcInit();
    // Set up the log console for use.
    WHBLogConsoleInit();
    WHBLogConsoleSetColor(0x00009900);
    // Initialize the Mocha library.
    if (Mocha_InitLibrary() != MOCHA_RESULT_SUCCESS) {
        WHBLogPrint("Mocha_InitLibrary failed!");
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(3000));
        WHBLogConsoleFree();
        WHBProcShutdown();
        return 0;
    }

    // Mount the storage device differently depending on Tiramisu or Aroma.
    if (checkIfTiramisu())
        Mocha_MountFS("slccmpt01", "/dev/slccmpt01", "/vol/storage_slccmpt01");
    else
        Mocha_MountFS("slccmpt01", NULL, "/vol/storage_slccmpt01");
    // Set some variables for the Wii U GamePad.
    VPADStatus input;
    VPADReadError error;

    // Print the console header.
    printMainMenu();

    while (WHBProcIsRunning())
    {
        // Watch the Wii U GamePad for button presses.
        VPADRead(VPAD_CHAN_0, &input, 1, &error);

        if (input.trigger & VPAD_BUTTON_A)
        {
            WHBLogPrint("Beginning theme data dump...");
            WHBLogPrintf("Source: %s", nandFile);
            WHBLogPrintf("Destination: %s", dumpFile);
            WHBLogPrint("----------------------------------------");
            WHBLogConsoleDraw();

            WHBLogPrint("Dumping theme file to SD card.");
            WHBLogConsoleDraw();
            // Open the theme file and dump it to the SD card.
            FILE *theme = fopen(nandFile, "rb");
            if (theme == NULL)
            {
                WHBLogPrint("Error opening theme file!");
                WHBLogConsoleDraw();
            }
            else
            {
                WHBLogPrint("Theme file opened successfully");
                WHBLogConsoleDraw();
                char *buffer = (char *)malloc(BUFFER_SIZE);
                if (buffer == NULL)
                {
                    WHBLogPrint("Error allocating memory!");
                    WHBLogConsoleDraw();
                }
                else
                {
                    WHBLogPrint("Memory was allocated successfully!");
                    WHBLogConsoleDraw();
                    FILE *dump = fopen(dumpFile, "wb");
                    if (dump == NULL)
                    {
                        WHBLogPrint("Error opening dump file!");
                        WHBLogConsoleDraw();
                    }
                    else
                    {
                        WHBLogPrint("Dump file opened successfully!");
                        WHBLogConsoleDraw();
                        size_t bytesRead = 0;
                        while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, theme)) > 0)
                        {
                            fwrite(buffer, 1, bytesRead, dump);
                        }
                        fclose(dump);
                        WHBLogPrint("Theme file dumped successfully!");
                        WHBLogConsoleDraw();
                    }
                    free(buffer);
                }
                fclose(theme);
            }
            OSSleepTicks(OSMillisecondsToTicks(3000));
            WHBLogPrint("----------------------------------------");
            printMainMenu();
        }
        else if (input.trigger & VPAD_BUTTON_B)
        {
            WHBLogPrint("Beginning theme data restore...");
            WHBLogPrintf("Source: %s", dumpFile);
            WHBLogPrintf("Destination: %s", nandFile);
            WHBLogPrint("----------------------------------------");
            WHBLogConsoleDraw();

            WHBLogPrint("Restoring theme file from SD card.");
            WHBLogConsoleDraw();
            // Open the theme file and restore it to the right place on SLCCMPT.
            FILE *theme = fopen(nandFile, "wb");
            if (theme == NULL)
            {
                WHBLogPrint("Error opening theme file!");
                WHBLogConsoleDraw();
            }
            else
            {
                WHBLogPrint("Theme file opened successfully");
                WHBLogConsoleDraw();
                char *buffer = (char *)malloc(BUFFER_SIZE);
                if (buffer == NULL)
                {
                    WHBLogPrint("Error allocating memory!");
                    WHBLogConsoleDraw();
                }
                else
                {
                    WHBLogPrint("Memory was allocated successfully!");
                    WHBLogConsoleDraw();
                    FILE *restore = fopen(dumpFile, "rb");
                    if (restore == NULL)
                    {
                        WHBLogPrint("Error opening restore file!");
                        WHBLogConsoleDraw();
                    }
                    else
                    {
                        WHBLogPrint("Restore file opened successfully!");
                        WHBLogConsoleDraw();
                        size_t bytesRead = 0;
                        while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, restore)) > 0)
                        {
                            fwrite(buffer, 1, bytesRead, theme);
                        }
                        fclose(restore);
                        WHBLogPrint("Theme file restored successfully!");
                        WHBLogConsoleDraw();
                    }
                    free(buffer);
                }
                fclose(theme);
            }
            OSSleepTicks(OSMillisecondsToTicks(3000));
            WHBLogPrint("----------------------------------------");
            printMainMenu();
        }
    }
    Mocha_DeInitLibrary();
    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}