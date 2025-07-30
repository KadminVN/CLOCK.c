/*--------------------------
    CLOCK.C -- Analog Clock Program
    (Icons Always Visible Version)
---------------------------*/

#include <windows.h>
#include <math.h> 
#include <stdio.h>
#include <mmsystem.h> 
#pragma comment(lib, "winmm.lib")

#define ID_TIMER 1
#define ID_DARKMODE_BTN 2
#define ID_ROMAN_BTN 3
#define ID_FONT_BTN 4
#define ID_SOUND_BTN 5
#define TWOPI (2*3.14159)

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

HFONT hMinecraftFont = NULL;         
HFONT hMinecraftBtnFont = NULL;      
HFONT hPixelFont = NULL;
HFONT hPixelBtnFont = NULL;      

const TCHAR* romanNumerals[] = {
    TEXT(""), TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"),
    TEXT("VI"), TEXT("VII"), TEXT("VIII"), TEXT("IX"), TEXT("X"), TEXT("XI"), TEXT("XII")
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static TCHAR szAppName[] = TEXT("Clock");
    HWND hwnd;
    MSG msg;
    WNDCLASS wndclass;

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
        return 0;
    }

    hwnd = CreateWindow(szAppName, TEXT("Analog Clock"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

void SetIsotropic(HDC hdc, int cxClient, int cyClient)
{
    int iScale = (cxClient < cyClient ? cxClient : cyClient) / 2;
    if (iScale == 0) iScale = 1;

    SetMapMode(hdc, MM_ISOTROPIC);
    SetWindowExtEx(hdc, 600, 600, NULL);
    SetViewportExtEx(hdc, iScale, -iScale, NULL);
    SetViewportOrgEx(hdc, cxClient / 2, cyClient / 2, NULL);
}

void RotatePoint(POINT pt[], int iNum, int iAngle)
{
    int i;
    POINT ptTemp;

    for (i = 0; i < iNum; i++)
    {
        ptTemp.x = (int)(pt[i].x * cos(TWOPI * iAngle / 360) +
            pt[i].y * sin(TWOPI * iAngle / 360));
        ptTemp.y = (int)(pt[i].y * cos(TWOPI * iAngle / 360) -
            pt[i].x * sin(TWOPI * iAngle / 360));
        pt[i] = ptTemp;
    }
}

void DrawClock(HDC hdc)
{
    int iAngle;
    POINT pt[3];
    HBRUSH hBrush, hOldBrush;
    HFONT hOldFont = NULL;
    HFONT hCurrentFont = g_bUsePixelFont ? hPixelFont : hMinecraftFont;

    hBrush = CreateSolidBrush(g_bDarkMode ? RGB(255, 255, 255) : RGB(0, 0, 0));
    hOldBrush = SelectObject(hdc, hBrush);

    for (iAngle = 0; iAngle < 360; iAngle += 6)
    {
        pt[0].x = 0;
        pt[0].y = 500;

        RotatePoint(pt, 1, iAngle);

        pt[2].x = pt[2].y = iAngle % 5 ? 18 : 55;

        pt[0].x -= pt[2].x / 2;
        pt[0].y -= pt[2].y / 2;

        pt[1].x = pt[0].x + pt[2].x;
        pt[1].y = pt[0].y + pt[2].y;

        Ellipse(hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);

        if (iAngle % 30 == 0 && hCurrentFont) {
            int hour = iAngle / 30;
            if (hour == 0) hour = 12;
            double rad = iAngle * 3.14159265358979323846 / 180.0;
            int numRadius = 450;
            int tx = (int)(0 + numRadius * sin(rad));
            int ty = (int)(0 + numRadius * cos(rad)) + 35;
            const TCHAR* label = g_bRomanMode ? romanNumerals[hour] : NULL;
            TCHAR buf[8];
            if (g_bRomanMode) {
                lstrcpy(buf, label);
            }
            else {
                wsprintf(buf, TEXT("%d"), hour);
            }
            hOldFont = (HFONT)SelectObject(hdc, hCurrentFont);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_bDarkMode ? RGB(255, 255, 255) : RGB(0, 0, 0));
            SIZE sz;
            GetTextExtentPoint32(hdc, buf, lstrlen(buf), &sz);
            TextOut(hdc, tx - sz.cx / 2, ty - sz.cy / 2, buf, lstrlen(buf));
            SelectObject(hdc, hOldFont);
        }
    }

    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);
}

void DrawHands(HDC hdc, SYSTEMTIME * pst, BOOL fChange)
{
    static POINT pt[3][5] = {
        {0, -110, 70, 0, 0, 300, -70, 0, 0, -110},
        {0, -150, 40, 0, 0, 420, -40, 0, 0, -150},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 420}
    };
    int i, iAngle[3];
    POINT ptTemp[3][5];
    HPEN hPen, hOldPen;
    COLORREF handColor = g_bDarkMode ? RGB(255, 255, 255) : RGB(0, 0, 0);

    iAngle[0] = (pst->wHour * 30) % 360 + pst->wMinute / 2;
    iAngle[1] = pst->wMinute * 6;
    iAngle[2] = pst->wSecond * 6;

    memcpy(ptTemp, pt, sizeof(pt));

    hPen = CreatePen(PS_SOLID, 0, handColor);
    hOldPen = SelectObject(hdc, hPen);

    for (i = fChange ? 0 : 2; i < 3; i++)
    {
        RotatePoint(ptTemp[i], 5, iAngle[i]);
        Polyline(hdc, ptTemp[i], 5);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
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
                0, 0, 260, 30, hwnd, (HMENU)ID_ROMAN_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            hBtnDarkMode = CreateWindow(
                TEXT("BUTTON"), TEXT("Dark Mode"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 180, 30, hwnd, (HMENU)ID_DARKMODE_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            hBtnFontSwitch = CreateWindow(
                TEXT("BUTTON"), TEXT("Switch to Pixel"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 220, 30, hwnd, (HMENU)ID_FONT_BTN,
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

            hPixelBtnFont = CreateFont(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FF_DONTCARE, TEXT("Dogica"));

            // Set fonts for buttons
            if (hBtnDarkMode) SendMessage(hBtnDarkMode, WM_SETFONT, (WPARAM)hMinecraftBtnFont, TRUE);
            if (hBtnRomanMode) SendMessage(hBtnRomanMode, WM_SETFONT, (WPARAM)hMinecraftBtnFont, TRUE);
            if (hBtnFontSwitch) SendMessage(hBtnFontSwitch, WM_SETFONT, (WPARAM)hMinecraftBtnFont, TRUE);

            return 0;
        }

        case WM_SIZE:
            cxClient = LOWORD(lParam);
            cyClient = HIWORD(lParam);

            // Position buttons
            if (hBtnRomanMode)
            {
                int btnWidth = 260, btnHeight = 30;
                int x = (cxClient - btnWidth) / 2;
                int y = 10;
                MoveWindow(hBtnRomanMode, x, y, btnWidth, btnHeight, TRUE);
            }
            if (hBtnDarkMode)
            {
                int btnWidth = 180, btnHeight = 30;
                int x = (cxClient - btnWidth) / 2;
                int y = cyClient - btnHeight - 10;
                MoveWindow(hBtnDarkMode, x, y, btnWidth, btnHeight, TRUE);
            }
            if (hBtnFontSwitch)
            {
                int btnWidth = 220, btnHeight = 30;
                int x = cxClient - btnWidth - 10;
                int y = 10;
                MoveWindow(hBtnFontSwitch, x, y, btnWidth, btnHeight, TRUE);
            }
            if (hBtnSound)
            {
                MoveWindow(hBtnSound, 10, 10, 40, 40, TRUE);
            }
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_DARKMODE_BTN)
            {
                g_bDarkMode = !g_bDarkMode;
                SetWindowText(hBtnDarkMode, g_bDarkMode ? TEXT("Light Mode") : TEXT("Dark Mode"));
                InvalidateRect(hwnd, NULL, FALSE);  // Changed to FALSE to prevent flicker
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
            break;

        case WM_TIMER:
            GetLocalTime(&st);
            fChange = st.wHour != stPrevious.wHour || st.wMinute != stPrevious.wMinute;

            hdc = GetDC(hwnd);
            
            // Only redraw the clock area, not the buttons
            RECT clockRect;
            GetClientRect(hwnd, &clockRect);
            clockRect.top += 50;  // Leave space for top buttons
            clockRect.bottom -= 50; // Leave space for bottom buttons
            
            FillRect(hdc, &clockRect, (HBRUSH)GetStockObject(g_bDarkMode ? BLACK_BRUSH : WHITE_BRUSH));
            
            SetIsotropic(hdc, cxClient, cyClient);
            DrawClock(hdc);
            DrawHands(hdc, &st, TRUE);

            if (g_bSoundOn && (st.wSecond % 1 == 0)) {
                PlaySound(TEXT("d:\\Git Uploads\\CLOCK.c\\Tick.wav"), NULL, SND_FILENAME | SND_ASYNC);
            }

            ReleaseDC(hwnd, hdc);
            stPrevious = st;
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            
            // Fill background
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(g_bDarkMode ? BLACK_BRUSH : WHITE_BRUSH));
            
            // Draw clock
            SetIsotropic(hdc, cxClient, cyClient);
            DrawClock(hdc);
            DrawHands(hdc, &stPrevious, TRUE);
            
            EndPaint(hwnd, &ps);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER);

            // Cleanup fonts
            if (hMinecraftFont) DeleteObject(hMinecraftFont);
            if (hMinecraftBtnFont) DeleteObject(hMinecraftBtnFont);
            if (hPixelFont) DeleteObject(hPixelFont);
            if (hPixelBtnFont) DeleteObject(hPixelBtnFont);

            // Cleanup icons
            if (hSoundOnIcon) DestroyIcon(hSoundOnIcon);
            if (hSoundOffIcon) DestroyIcon(hSoundOffIcon);

            // Cleanup font resources
            RemoveFontResourceEx(TEXT("d:\\Git Uploads\\CLOCK.c\\Minecraft.ttf"), FR_PRIVATE, 0);
            RemoveFontResourceEx(TEXT("d:\\Git Uploads\\CLOCK.c\\Dogica.ttf"), FR_PRIVATE, 0);

            PlaySound(NULL, 0, 0); // Stop any playing sound
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}