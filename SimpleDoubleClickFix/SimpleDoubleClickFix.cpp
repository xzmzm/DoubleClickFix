// This file contains the 'main' function and mouse hook logic.

#include <windows.h>
#include <iostream>

// Global variables
HHOOK g_hMouseHook = NULL;
DWORD g_dwMainThreadId = 0; // To post WM_QUIT for graceful shutdown

// Timestamps for the last 'up' event for each button
DWORD g_dwLastLeftButtonUpTime = 0;
DWORD g_dwLastRightButtonUpTime = 0;
DWORD g_dwLastMiddleButtonUpTime = 0;
DWORD g_dwLastX1ButtonUpTime = 0;
DWORD g_dwLastX2ButtonUpTime = 0;

// Default thresholds (inspired by SettingsBase)
// A negative threshold means the filtering for that button is disabled.
const int LEFT_THRESHOLD = 50;    // ms, enabled by default
const int RIGHT_THRESHOLD = -1;   // ms, disabled by default
const int MIDDLE_THRESHOLD = -1;  // ms, disabled by default
const int X1_THRESHOLD = -1;      // ms, disabled by default
const int X2_THRESHOLD = -1;      // ms, disabled by default

// LowLevelMouseProc callback function
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
        DWORD currentTime = pMouseStruct->time;
        bool suppressEvent = false;

        switch (wParam) {
            // Left Mouse Button
            case WM_LBUTTONDOWN:
                if (LEFT_THRESHOLD >= 0 && g_dwLastLeftButtonUpTime != 0) {
                    DWORD timeDiff = currentTime - g_dwLastLeftButtonUpTime;
                    if (timeDiff < (DWORD)LEFT_THRESHOLD) {
                        std::cout << "Left button rapid click suppressed. Interval: " << timeDiff << "ms." << std::endl;
                        suppressEvent = true;
                        g_dwLastLeftButtonUpTime = 0; // Reset to prevent repeated suppression for a fast sequence
                    }
                }
                break;
            case WM_LBUTTONUP:
                if (LEFT_THRESHOLD >= 0) {
                    g_dwLastLeftButtonUpTime = currentTime;
                }
                break;

            // Right Mouse Button
            case WM_RBUTTONDOWN:
                if (RIGHT_THRESHOLD >= 0 && g_dwLastRightButtonUpTime != 0) {
                    DWORD timeDiff = currentTime - g_dwLastRightButtonUpTime;
                    if (timeDiff < (DWORD)RIGHT_THRESHOLD) {
                        std::cout << "Right button rapid click suppressed. Interval: " << timeDiff << "ms." << std::endl;
                        suppressEvent = true;
                        g_dwLastRightButtonUpTime = 0;
                    }
                }
                break;
            case WM_RBUTTONUP:
                if (RIGHT_THRESHOLD >= 0) {
                    g_dwLastRightButtonUpTime = currentTime;
                }
                break;

            // Middle Mouse Button
            case WM_MBUTTONDOWN:
                if (MIDDLE_THRESHOLD >= 0 && g_dwLastMiddleButtonUpTime != 0) {
                    DWORD timeDiff = currentTime - g_dwLastMiddleButtonUpTime;
                    if (timeDiff < (DWORD)MIDDLE_THRESHOLD) {
                        std::cout << "Middle button rapid click suppressed. Interval: " << timeDiff << "ms." << std::endl;
                        suppressEvent = true;
                        g_dwLastMiddleButtonUpTime = 0;
                    }
                }
                break;
            case WM_MBUTTONUP:
                if (MIDDLE_THRESHOLD >= 0) {
                    g_dwLastMiddleButtonUpTime = currentTime;
                }
                break;

            // X Buttons (Side Buttons)
            case WM_XBUTTONDOWN: {
                WORD xButton = HIWORD(pMouseStruct->mouseData);
                if (xButton == XBUTTON1 && X1_THRESHOLD >= 0 && g_dwLastX1ButtonUpTime != 0) {
                    DWORD timeDiff = currentTime - g_dwLastX1ButtonUpTime;
                    if (timeDiff < (DWORD)X1_THRESHOLD) {
                        std::cout << "X1 button rapid click suppressed. Interval: " << timeDiff << "ms." << std::endl;
                        suppressEvent = true;
                        g_dwLastX1ButtonUpTime = 0;
                    }
                } else if (xButton == XBUTTON2 && X2_THRESHOLD >= 0 && g_dwLastX2ButtonUpTime != 0) {
                    DWORD timeDiff = currentTime - g_dwLastX2ButtonUpTime;
                    if (timeDiff < (DWORD)X2_THRESHOLD) {
                        std::cout << "X2 button rapid click suppressed. Interval: " << timeDiff << "ms." << std::endl;
                        suppressEvent = true;
                        g_dwLastX2ButtonUpTime = 0;
                    }
                }
                break;
            }
            case WM_XBUTTONUP: {
                 WORD xButton = HIWORD(pMouseStruct->mouseData);
                 if (xButton == XBUTTON1 && X1_THRESHOLD >= 0) {
                    g_dwLastX1ButtonUpTime = currentTime;
                 } else if (xButton == XBUTTON2 && X2_THRESHOLD >= 0) {
                    g_dwLastX2ButtonUpTime = currentTime;
                 }
                break;
            }
        }

        if (suppressEvent) {
            return 1; // Suppress the message by returning a non-zero value
        }
    }
    // Call the next hook in the chain
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

// Console event handler for graceful shutdown
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType) {
    switch (dwCtrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            std::cout << "Console event detected. Unhooking mouse..." << std::endl;
            if (g_hMouseHook != NULL) {
                UnhookWindowsHookEx(g_hMouseHook);
                g_hMouseHook = NULL; 
                std::cout << "Mouse hook uninstalled." << std::endl;
            }
            // For Ctrl+C/Break, explicitly tell the message loop to quit.
            // For CTRL_CLOSE_EVENT, the process terminates anyway.
            if (dwCtrlType != CTRL_CLOSE_EVENT && g_dwMainThreadId != 0) {
                 PostThreadMessage(g_dwMainThreadId, WM_QUIT, 0, 0);
            }
            return TRUE; // Indicate that the event was handled
        default:
            return FALSE; // Pass on unhandled events
    }
}

int main() {
    std::cout << "Simple Double-Click Fix Program" << std::endl;
    std::cout << "---------------------------------" << std::endl;
    if (LEFT_THRESHOLD >= 0)
        std::cout << "Fix is ACTIVE for Left Mouse Button (Threshold: " << LEFT_THRESHOLD << "ms)." << std::endl;
    if (RIGHT_THRESHOLD >= 0)
        std::cout << "Fix is ACTIVE for Right Mouse Button (Threshold: " << RIGHT_THRESHOLD << "ms)." << std::endl;
    if (MIDDLE_THRESHOLD >= 0)
        std::cout << "Fix is ACTIVE for Middle Mouse Button (Threshold: " << MIDDLE_THRESHOLD << "ms)." << std::endl;
    if (X1_THRESHOLD >= 0)
        std::cout << "Fix is ACTIVE for X1 Mouse Button (Threshold: " << X1_THRESHOLD << "ms)." << std::endl;
    if (X2_THRESHOLD >= 0)
        std::cout << "Fix is ACTIVE for X2 Mouse Button (Threshold: " << X2_THRESHOLD << "ms)." << std::endl;
    std::cout << "Press Ctrl+C in this window or close it to stop the program." << std::endl;
    std::cout << "---------------------------------" << std::endl;

    g_dwMainThreadId = GetCurrentThreadId();

    // Set up the console control handler
    if (!SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE)) {
        std::cerr << "Error: Could not set console control handler." << std::endl;
        // Continue, but Ctrl+C/Close might not unhook cleanly
    }

    // Install the low-level mouse hook
    // WH_MOUSE_LL must be processed in the thread that installed the hook.
    // The hook procedure can be in a DLL and can be called by different threads.
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
    if (g_hMouseHook == NULL) {
        std::cerr << "Error: Failed to install mouse hook. GetLastError = " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Mouse hook installed successfully. Monitoring mouse events..." << std::endl;

    // Message loop to keep the hook alive and process messages for this thread.
    // This is necessary for WH_MOUSE_LL hooks.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) { // GetMessage returns >0 for messages other than WM_QUIT
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup: Unhook if not already done by ConsoleHandlerRoutine
    // This part is reached when GetMessage returns 0 (WM_QUIT) or -1 (error).
    if (g_hMouseHook != NULL) {
        UnhookWindowsHookEx(g_hMouseHook);
        std::cout << "Mouse hook uninstalled (fallback cleanup)." << std::endl;
    }
    
    std::cout << "Program terminated." << std::endl;
    return (int)msg.wParam; // Return quit code
}