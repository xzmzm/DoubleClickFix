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

// Flags to track suppression state to swallow the matching 'up' event
bool g_bLeftDownSuppressed = false;
bool g_bRightDownSuppressed = false;
bool g_bMiddleDownSuppressed = false;
bool g_bX1DownSuppressed = false;
bool g_bX2DownSuppressed = false;

// Default thresholds (inspired by SettingsBase)
// A negative threshold means the filtering for that button is disabled.
const int LEFT_THRESHOLD = 50;    // ms, enabled by default
const int RIGHT_THRESHOLD = -1;   // ms, disabled by default
const int MIDDLE_THRESHOLD = -1;  // ms, disabled by default
const int X1_THRESHOLD = -1;      // ms, disabled by default
const int X2_THRESHOLD = -1;      // ms, disabled by default

// Global variables for precise time
LARGE_INTEGER g_qpcFrequency;
BOOL g_qpcIsAvailable = FALSE;

// Helper function to get current time in milliseconds with highest precision available
DWORD GetCurrentPreciseTimeMs() {
    if (g_qpcIsAvailable) {
        LARGE_INTEGER qpcTime;
        QueryPerformanceCounter(&qpcTime);
        // Calculate milliseconds. Intermediate product qpcTime.QuadPart * 1000 fits in LONGLONG for ~29 years of continuous uptime.
        // The final result is cast to DWORD, accepting a ~49.7 day rollover for the millisecond counter.
        // This maintains consistency with GetTickCount() and the original pMouseStruct->time behavior.
        ULONGLONG milliseconds = (qpcTime.QuadPart * 1000) / g_qpcFrequency.QuadPart;
        return (DWORD)milliseconds;
    } else {
        return GetTickCount(); // Fallback if QPC is not available
    }
}

// LowLevelMouseProc callback function
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
        DWORD currentTime = GetCurrentPreciseTimeMs(); // Use more precise time
        bool suppressEvent = false;

        switch (wParam) {
            // Left Mouse Button
            case WM_LBUTTONDOWN:
                if (LEFT_THRESHOLD >= 0 && g_dwLastLeftButtonUpTime != 0) {
                    DWORD timeDiff = currentTime - g_dwLastLeftButtonUpTime;
                    if (timeDiff < (DWORD)LEFT_THRESHOLD) {
                        std::cout << "Left button rapid click suppressed. Interval: " << timeDiff << "ms." << std::endl;
                        suppressEvent = true;
                        g_bLeftDownSuppressed = true; // Mark to suppress the corresponding UP event
                    }
                }
                if (!suppressEvent) {
                    g_dwLastLeftButtonUpTime = 0; // A valid down click consumes the last up time
                }
                break;
            case WM_LBUTTONUP:
                if (LEFT_THRESHOLD >= 0) {
                    if (g_bLeftDownSuppressed) {
                        suppressEvent = true;
                        g_bLeftDownSuppressed = false; // Reset the flag
                    } else {
                        g_dwLastLeftButtonUpTime = currentTime;
                    }
                }
                break;

            // Right Mouse Button
            case WM_RBUTTONDOWN:
                if (RIGHT_THRESHOLD >= 0 && g_dwLastRightButtonUpTime != 0) {
                    DWORD timeDiff = currentTime - g_dwLastRightButtonUpTime;
                    if (timeDiff < (DWORD)RIGHT_THRESHOLD) {
                        std::cout << "Right button rapid click suppressed. Interval: " << timeDiff << "ms." << std::endl;
                        suppressEvent = true;
                        g_bRightDownSuppressed = true;
                    }
                }
                if (!suppressEvent) {
                    g_dwLastRightButtonUpTime = 0;
                }
                break;
            case WM_RBUTTONUP:
                if (RIGHT_THRESHOLD >= 0) {
                    if (g_bRightDownSuppressed) {
                        suppressEvent = true;
                        g_bRightDownSuppressed = false;
                    } else {
                        g_dwLastRightButtonUpTime = currentTime;
                    }
                }
                break;

            // Middle Mouse Button
            case WM_MBUTTONDOWN:
                if (MIDDLE_THRESHOLD >= 0 && g_dwLastMiddleButtonUpTime != 0) {
                    DWORD timeDiff = currentTime - g_dwLastMiddleButtonUpTime;
                    if (timeDiff < (DWORD)MIDDLE_THRESHOLD) {
                        std::cout << "Middle button rapid click suppressed. Interval: " << timeDiff << "ms." << std::endl;
                        suppressEvent = true;
                        g_bMiddleDownSuppressed = true;
                    }
                }
                if (!suppressEvent) {
                    g_dwLastMiddleButtonUpTime = 0;
                }
                break;
            case WM_MBUTTONUP:
                if (MIDDLE_THRESHOLD >= 0) {
                    if (g_bMiddleDownSuppressed) {
                        suppressEvent = true;
                        g_bMiddleDownSuppressed = false;
                    } else {
                        g_dwLastMiddleButtonUpTime = currentTime;
                    }
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
                        g_bX1DownSuppressed = true;
                    }
                } else if (xButton == XBUTTON2 && X2_THRESHOLD >= 0 && g_dwLastX2ButtonUpTime != 0) {
                    DWORD timeDiff = currentTime - g_dwLastX2ButtonUpTime;
                    if (timeDiff < (DWORD)X2_THRESHOLD) {
                        std::cout << "X2 button rapid click suppressed. Interval: " << timeDiff << "ms." << std::endl;
                        suppressEvent = true;
                        g_bX2DownSuppressed = true;
                    }
                }
                if (!suppressEvent) {
                    if (xButton == XBUTTON1) g_dwLastX1ButtonUpTime = 0;
                    else if (xButton == XBUTTON2) g_dwLastX2ButtonUpTime = 0;
                }
                break;
            }
            case WM_XBUTTONUP: {
                 WORD xButton = HIWORD(pMouseStruct->mouseData);
                 if (xButton == XBUTTON1 && X1_THRESHOLD >= 0) {
                     if (g_bX1DownSuppressed) {
                         suppressEvent = true;
                         g_bX1DownSuppressed = false;
                     } else {
                         g_dwLastX1ButtonUpTime = currentTime;
                     }
                 } else if (xButton == XBUTTON2 && X2_THRESHOLD >= 0) {
                     if (g_bX2DownSuppressed) {
                         suppressEvent = true;
                         g_bX2DownSuppressed = false;
                     } else {
                         g_dwLastX2ButtonUpTime = currentTime;
                     }
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

    // Initialize high-resolution timer capabilities
    g_qpcIsAvailable = QueryPerformanceFrequency(&g_qpcFrequency);
    if (g_qpcIsAvailable) {
        std::cout << "High-resolution performance counter is available. Using precise timing." << std::endl;
    } else {
        std::cout << "Warning: High-resolution performance counter not available. Falling back to GetTickCount() for timing (lower precision)." << std::endl;
    }

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
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
    if (g_hMouseHook == NULL) {
        std::cerr << "Error: Failed to install mouse hook. GetLastError = " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Mouse hook installed successfully. Monitoring mouse events..." << std::endl;

    // Message loop to keep the hook alive and process messages for this thread.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) { 
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup: Unhook if not already done by ConsoleHandlerRoutine
    if (g_hMouseHook != NULL) {
        UnhookWindowsHookEx(g_hMouseHook);
        std::cout << "Mouse hook uninstalled (fallback cleanup)." << std::endl;
    }
    
    std::cout << "Program terminated." << std::endl;
    return (int)msg.wParam; // Return quit code
}