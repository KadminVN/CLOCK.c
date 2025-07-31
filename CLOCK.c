/*--------------------------
    CLOCK.C -- Analog Clock Program
    (Improved version with embedded resources)
---------------------------*/

#include <windows.h>
#include <math.h> 
#include <stdio.h>
#include <mmsystem.h> 
#include <shellapi.h>
#include <tchar.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")

#define ID_TIMER 1
#define ID_DARKMODE_BTN 2
#define ID_ROMAN_BTN 3
#define ID_FONT_BTN 4
#define ID_SOUND_BTN 5
#define ID_DOTS_BTN 6
#define ID_MYSTERY_BTN 7
#define TWOPI (2*3.14159)

// Resource IDs (must match your .rc file)
#define IDI_SOUND_ON     101
#define IDI_SOUND_OFF    102
#define IDR_MC_FONT      103
#define IDR_PIXEL_FONT   104

// Global variables
BOOL g_bDarkMode = FALSE;
HWND hBtnDarkMode = NULL;

BOOL g_bRomanMode = FALSE;
HWND hBtnRomanMode = NULL;

BOOL g_bUsePixelFont = FALSE;
HWND hBtnFontSwitch = NULL;

BOOL g_bSoundOn = TRUE;
HWND hBtnSound = NULL;
HICON hSoundOnIcon = NULL;
HICON hSoundOffIcon = NULL;

BOOL g_bShowDots = TRUE;
HWND hBtnDots = NULL;

HWND hBtnMystery = NULL;

HFONT hMinecraftFont = NULL;         
HFONT hMinecraftBtnFont = NULL;      
HFONT hPixelFont = NULL;
HFONT hPixelBtnFont = NULL;      

const TCHAR* romanNumerals[] = {
    TEXT(""), TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"),
    TEXT("VI"), TEXT("VII"), TEXT("VIII"), TEXT("IX"), TEXT("X"), TEXT("XI"), TEXT("XII")
};

// Function prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void UpdateButtonFonts(HWND hwnd);
BOOL LoadEmbeddedResources(HINSTANCE hInstance);
void FreeResources();
void PlayMysteryVideo();

// Helper function to get executable directory
BOOL GetExeDirectory(TCHAR* buffer, DWORD size)
{
    if (!GetModuleFileName(NULL, buffer, size))
        return FALSE;
    
    TCHAR* lastSlash = _tcsrchr(buffer, TEXT('\\'));
    if (lastSlash)
        *lastSlash = TEXT('\0');
    
    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static TCHAR szAppName[] = TEXT("Clock");
    HWND hwnd;
    MSG msg;
    WNDCLASS wndclass;

    // Load embedded resources
    if (!LoadEmbeddedResources(hInstance))
    {
        MessageBox(NULL, TEXT("Failed to load required resources!"), szAppName, MB_ICONERROR);
        return 0;
    }

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = NULL;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;

    if (!RegisterClass(&wndclass))
    {
        MessageBox(NULL, TEXT("Program requires Windows NT!"), szAppName, MB_ICONERROR);
        FreeResources();
        return 0;
    }

    hwnd = CreateWindow(szAppName, TEXT("Analog Clock"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInstance, NULL);

    if (!hwnd)
    {
        FreeResources();
        return 0;
    }

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    FreeResources();
    return msg.wParam;
}

// Resource loading functions
BOOL LoadEmbeddedResources(HINSTANCE hInstance)
{
    // Load icons from resources
    hSoundOnIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOUND_ON));
    hSoundOffIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOUND_OFF));
    
    if (!hSoundOnIcon || !hSoundOffIcon)
    {
        // Fallback to system icons if embedded ones fail
        hSoundOnIcon = LoadIcon(NULL, IDI_INFORMATION);
        hSoundOffIcon = LoadIcon(NULL, IDI_WARNING);
    }

    // Add embedded fonts
    HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(IDR_MC_FONT), RT_FONT);
    if (hRes)
    {
        HGLOBAL hFontRes = LoadResource(hInstance, hRes);
        if (hFontRes)
        {
            DWORD nFonts = 0;
            AddFontMemResourceEx(hFontRes, SizeofResource(hInstance, hRes), NULL, &nFonts);
        }
    }

    hRes = FindResource(hInstance, MAKEINTRESOURCE(IDR_PIXEL_FONT), RT_FONT);
    if (hRes)
    {
        HGLOBAL hFontRes = LoadResource(hInstance, hRes);
        if (hFontRes)
        {
            DWORD nFonts = 0;
            AddFontMemResourceEx(hFontRes, SizeofResource(hInstance, hRes), NULL, &nFonts);
        }
    }

    // Create fonts
    hMinecraftFont = CreateFont(-40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_DONTCARE, TEXT("Minecraft"));

    hMinecraftBtnFont = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_DONTCARE, TEXT("Minecraft"));

    hPixelFont = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_DONTCARE, TEXT("Dogica"));

    hPixelBtnFont = CreateFont(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_DONTCARE, TEXT("Dogica"));

    // Fallback to system fonts if custom fonts fail
    if (!hMinecraftFont || !hMinecraftBtnFont)
    {
        hMinecraftFont = hMinecraftBtnFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }
    if (!hPixelFont || !hPixelBtnFont)
    {
        hPixelFont = hPixelBtnFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }

    return TRUE;
}

void FreeResources()
{
    if (hMinecraftFont && hMinecraftFont != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
        DeleteObject(hMinecraftFont);
    if (hMinecraftBtnFont && hMinecraftBtnFont != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
        DeleteObject(hMinecraftBtnFont);
    if (hPixelFont && hPixelFont != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
        DeleteObject(hPixelFont);
    if (hPixelBtnFont && hPixelBtnFont != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
        DeleteObject(hPixelBtnFont);

    if (hSoundOnIcon && hSoundOnIcon != LoadIcon(NULL, IDI_INFORMATION))
        DestroyIcon(hSoundOnIcon);
    if (hSoundOffIcon && hSoundOffIcon != LoadIcon(NULL, IDI_WARNING))
        DestroyIcon(hSoundOffIcon);
}

void PlayMysteryVideo()
{
    TCHAR exePath[MAX_PATH];
    if (GetExeDirectory(exePath, MAX_PATH))
    {
        TCHAR videoPath[MAX_PATH];
        _stprintf(videoPath, TEXT("%s\\ASTELLION.mp4"), exePath);
        
        // Try to play the video
        HINSTANCE result = ShellExecute(NULL, TEXT("open"), videoPath, NULL, NULL, SW_SHOWNORMAL);
        
        // If failed, show a message
        if ((INT_PTR)result <= 32)
        {
            MessageBox(NULL, 
                TEXT("Could not play the mystery video!\n")
                TEXT("Please make sure ASTELLION.mp4 is in the same folder as this program."),
                TEXT("Clock"), MB_ICONINFORMATION);
        }
    }
    else
    {
        MessageBox(NULL, 
            TEXT("Could not locate the program directory!\n")
            TEXT("Please make sure ASTELLION.mp4 is in the same folder as this program."),
            TEXT("Clock"), MB_ICONINFORMATION);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int cxClient, cyClient;
    static SYSTEMTIME stPrevious;
    BOOL fChange;
    HDC hdc;
    PAINTSTRUCT ps;
    SYSTEMTIME st;
    RECT rect;

    switch (message)
    {
        case WM_CREATE:
        {
            SetTimer(hwnd, ID_TIMER, 1000, NULL);
            GetLocalTime(&st);
            stPrevious = st;

            // Load custom sound icons
            hSoundOnIcon = (HICON)LoadImage(
                NULL,
                TEXT("sound_on.ico"),
                IMAGE_ICON,
                32, 32,
                LR_LOADFROMFILE);

            hSoundOffIcon = (HICON)LoadImage(
                NULL,
                TEXT("sound_off.ico"),
                IMAGE_ICON,
                32, 32,
                LR_LOADFROMFILE);

            // Create buttons
            hBtnSound = CreateWindow(
                TEXT("BUTTON"),
                NULL,
                WS_CHILD | WS_VISIBLE | BS_ICON,
                10, 10, 40, 40,
                hwnd,
                (HMENU)ID_SOUND_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance,
                NULL);

            hBtnRomanMode = CreateWindow(
                TEXT("BUTTON"), TEXT("Switch to Roman"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 260, 50, hwnd, (HMENU)ID_ROMAN_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            hBtnDarkMode = CreateWindow(
                TEXT("BUTTON"), TEXT("Dark Mode"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 180, 50, hwnd, (HMENU)ID_DARKMODE_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            hBtnFontSwitch = CreateWindow(
                TEXT("BUTTON"), TEXT("Switch to Pixel"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 400, 40, hwnd, (HMENU)ID_FONT_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            hBtnDots = CreateWindow(
                TEXT("BUTTON"), TEXT("Disable Dots"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 220, 50, hwnd, (HMENU)ID_DOTS_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            // Create mystery button with owner-draw style
            hBtnMystery = CreateWindow(
                TEXT("BUTTON"), TEXT("???"),
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                0, 0, 50, 50,
                hwnd, (HMENU)ID_MYSTERY_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            // Set initial icon
            SendMessage(hBtnSound, BM_SETIMAGE, IMAGE_ICON,
                (LPARAM)(g_bSoundOn ? hSoundOnIcon : hSoundOffIcon));

            // Load fonts
            static const TCHAR fontPathMinecraft[] = TEXT("d:\\Git Uploads\\CLOCK.c\\Minecraft.ttf");
            static const TCHAR fontPathPixel[] = TEXT("d:\\Git Uploads\\CLOCK.c\\Dogica.ttf");
            AddFontResourceEx(fontPathMinecraft, FR_PRIVATE, 0);
            AddFontResourceEx(fontPathPixel, FR_PRIVATE, 0);

            hMinecraftFont = CreateFont(-40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FF_DONTCARE, TEXT("Minecraft"));

            hMinecraftBtnFont = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FF_DONTCARE, TEXT("Minecraft"));

            hPixelFont = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FF_DONTCARE, TEXT("Dogica"));

            hPixelBtnFont = CreateFont(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FF_DONTCARE, TEXT("Dogica"));

            // Set initial fonts for all buttons
            UpdateButtonFonts(hwnd);

            return 0;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
            if (lpDrawItem->CtlID == ID_MYSTERY_BTN)
            {
                // Draw the button background
                FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, 
                        (HBRUSH)(COLOR_BTNFACE + 1));
                
                // Draw the border
                if (lpDrawItem->itemState & ODS_SELECTED)
                    DrawEdge(lpDrawItem->hDC, &lpDrawItem->rcItem, EDGE_SUNKEN, BF_RECT);
                else
                    DrawEdge(lpDrawItem->hDC, &lpDrawItem->rcItem, EDGE_RAISED, BF_RECT);
                
                // Set up the font
                HFONT hOldFont = (HFONT)SelectObject(lpDrawItem->hDC, 
                    g_bUsePixelFont ? hPixelBtnFont : hMinecraftBtnFont);
                
                // Calculate text position
                const TCHAR* text = TEXT("???");
                SIZE sz;
                GetTextExtentPoint32(lpDrawItem->hDC, text, 3, &sz);
                int x = (lpDrawItem->rcItem.right - sz.cx) / 2;
                int y = (lpDrawItem->rcItem.bottom - sz.cy) / 2;
                
                // Set background mode
                SetBkMode(lpDrawItem->hDC, TRANSPARENT);
                
                // Draw each question mark with different color
                for (int i = 0; i < 3; i++)
                {
                    COLORREF colors[] = {RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255)}; // Red, Green, Blue
                    SetTextColor(lpDrawItem->hDC, colors[i]);
                    
                    // Draw single character
                    TCHAR ch[2] = {text[i], 0};
                    TextOut(lpDrawItem->hDC, x + i * (sz.cx / 3), y, ch, 1);
                }
                
                // Clean up
                SelectObject(lpDrawItem->hDC, hOldFont);
                return TRUE;
            }
            break;
        }

        case WM_SIZE:
            cxClient = LOWORD(lParam);
            cyClient = HIWORD(lParam);

            // Position buttons
            if (hBtnRomanMode)
            {
                int btnWidth = 260, btnHeight = 50;
                int x = (cxClient - btnWidth) / 2;
                int y = 10;
                MoveWindow(hBtnRomanMode, x, y, btnWidth, btnHeight, TRUE);
            }
            if (hBtnDarkMode)
            {
                int btnWidth = 180, btnHeight = 50;
                int x = (cxClient - btnWidth) / 2;
                int y = cyClient - btnHeight - 10;
                MoveWindow(hBtnDarkMode, x, y, btnWidth, btnHeight, TRUE);
            }
            if (hBtnFontSwitch)
            {
                int btnWidth = 400, btnHeight = 40;
                int x = cxClient - btnWidth - 10;
                int y = 10;
                MoveWindow(hBtnFontSwitch, x, y, btnWidth, btnHeight, TRUE);
            }
            if (hBtnSound)
            {
                MoveWindow(hBtnSound, 10, 10, 40, 40, TRUE);
            }
            if (hBtnDots)
            {
                int btnWidth = 220, btnHeight = 50;
                int x = 10;
                int y = cyClient - btnHeight - 10;
                MoveWindow(hBtnDots, x, y, btnWidth, btnHeight, TRUE);
            }
            if (hBtnMystery)
            {
                int btnSize = 50;  // Square size
                int x = cxClient - btnSize - 10;
                int y = cyClient - btnSize - 10;
                MoveWindow(hBtnMystery, x, y, btnSize, btnSize, TRUE);
            }
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_DARKMODE_BTN)
            {
                g_bDarkMode = !g_bDarkMode;
                SetWindowText(hBtnDarkMode, g_bDarkMode ? TEXT("Light Mode") : TEXT("Dark Mode"));
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (LOWORD(wParam) == ID_ROMAN_BTN)
            {
                g_bRomanMode = !g_bRomanMode;
                SetWindowText(hBtnRomanMode, g_bRomanMode ? TEXT("Switch to nums") : TEXT("Switch to Roman"));
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (LOWORD(wParam) == ID_FONT_BTN)
            {
                g_bUsePixelFont = !g_bUsePixelFont;
                SetWindowText(hBtnFontSwitch, g_bUsePixelFont ? TEXT("Switch to Minecraft") : TEXT("Switch to Pixel"));
                UpdateButtonFonts(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (LOWORD(wParam) == ID_SOUND_BTN)
            {
                g_bSoundOn = !g_bSoundOn;
                SendMessage(hBtnSound, BM_SETIMAGE, IMAGE_ICON,
                    (LPARAM)(g_bSoundOn ? hSoundOnIcon : hSoundOffIcon));
                return 0;
            }
            if (LOWORD(wParam) == ID_DOTS_BTN)
            {
                g_bShowDots = !g_bShowDots;
                SetWindowText(hBtnDots, g_bShowDots ? TEXT("Disable Dots") : TEXT("Enable Dots"));
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (LOWORD(wParam) == ID_MYSTERY_BTN)
            {
                PlayMysteryVideo();
                return 0;
            }
            break;

        case WM_TIMER:
            GetLocalTime(&st);
            fChange = st.wHour != stPrevious.wHour || st.wMinute != stPrevious.wMinute;

            hdc = GetDC(hwnd);
            
            // Draw the clock
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(g_bDarkMode ? BLACK_BRUSH : WHITE_BRUSH));
            
            SetIsotropic(hdc, cxClient, cyClient);
            DrawClock(hdc);
            DrawHands(hdc, &st, TRUE);

            if (g_bSoundOn && (st.wSecond % 1 == 0)) {
                PlaySound(TEXT("d:\\Git Uploads\\CLOCK.c\\Tick.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }

            // Force buttons to redraw on top of the clock
            InvalidateRect(hBtnSound, NULL, TRUE);
            InvalidateRect(hBtnRomanMode, NULL, TRUE);
            InvalidateRect(hBtnDarkMode, NULL, TRUE);
            InvalidateRect(hBtnFontSwitch, NULL, TRUE);
            InvalidateRect(hBtnDots, NULL, TRUE);
            InvalidateRect(hBtnMystery, NULL, TRUE);
            UpdateWindow(hwnd);

            ReleaseDC(hwnd, hdc);
            stPrevious = st;
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            
            // Draw the clock first
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(g_bDarkMode ? BLACK_BRUSH : WHITE_BRUSH));
            
            SetIsotropic(hdc, cxClient, cyClient);
            DrawClock(hdc);
            DrawHands(hdc, &stPrevious, TRUE);
            
            EndPaint(hwnd, &ps);
            
            // Force buttons to redraw on top
            RedrawWindow(hBtnSound, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnRomanMode, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnDarkMode, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnFontSwitch, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnDots, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnMystery, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER);

            if (hMinecraftFont) DeleteObject(hMinecraftFont);
            if (hMinecraftBtnFont) DeleteObject(hMinecraftBtnFont);
            if (hPixelFont) DeleteObject(hPixelFont);
            if (hPixelBtnFont) DeleteObject(hPixelBtnFont);

            if (hSoundOnIcon) DestroyIcon(hSoundOnIcon);
            if (hSoundOffIcon) DestroyIcon(hSoundOffIcon);

            RemoveFontResourceEx(TEXT("d:\\Git Uploads\\CLOCK.c\\Minecraft.ttf"), FR_PRIVATE, 0);
            RemoveFontResourceEx(TEXT("d:\\Git Uploads\\CLOCK.c\\Dogica.ttf"), FR_PRIVATE, 0);

            PlaySound(NULL, 0, 0);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}