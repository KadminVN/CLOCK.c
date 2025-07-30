/*--------------------------
    CLOCK.C -- Analog Clock Program
----------------------------*/

#include <windows.h>
#include <math.h> 
#include <stdio.h> // For sprintf

#define ID_TIMER 1
#define ID_DARKMODE_BTN 2
#define ID_ROMAN_BTN 3
#define TWOPI (2*3.14159)

// Global variable for dark mode
BOOL g_bDarkMode = FALSE;
HWND hBtnDarkMode = NULL;

BOOL g_bRomanMode = FALSE;
HWND hBtnRomanMode = NULL;

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
    static TCHAR szAppName[] = TEXT ("Clock");
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
    wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;

    if(!RegisterClass (&wndclass))
    {
        MessageBox (NULL,
                    TEXT ("Program requires Windows NT!"),
                    szAppName, MB_ICONERROR);
        return 0;
    }
    hwnd = CreateWindow(szAppName, TEXT ("Analog Clock"),
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, hInstance, NULL);
    ShowWindow (hwnd, iCmdShow);
    UpdateWindow (hwnd);
    while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }
    return msg.wParam;
}

void SetIsotropic (HDC hdc, int cxClient, int cyClient)
{
    int iScale = (cxClient < cyClient ? cxClient : cyClient) / 2;
    if (iScale == 0) iScale = 1;

    SetMapMode (hdc, MM_ISOTROPIC);
    // Reduce logical window extent from 800x800 to 600x600 for a smaller clock
    SetWindowExtEx (hdc, 600, 600, NULL);
    SetViewportExtEx (hdc, iScale, -iScale, NULL);
    SetViewportOrgEx (hdc, cxClient / 2, cyClient / 2, NULL);
}

void RotatePoint (POINT pt[], int iNum, int iAngle)
{
    int i;
    POINT ptTemp;

    for(i = 0; i < iNum; i++)
    {
        ptTemp.x = (int) (pt[i].x * cos (TWOPI * iAngle / 360) + 
                          pt[i].y * sin (TWOPI * iAngle / 360));
        ptTemp.y = (int) (pt[i].y * cos (TWOPI * iAngle / 360) - 
                          pt[i].x * sin (TWOPI * iAngle / 360));
        pt[i] = ptTemp;
    }
}

HFONT hMinecraftFont = NULL;         // For numbers on the clock
HFONT hMinecraftBtnFont = NULL;      // For button text
HANDLE hFontRes = NULL;

// Roman numerals for 1-12
const TCHAR* romanNumerals[] = {
    TEXT(""), TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"),
    TEXT("VI"), TEXT("VII"), TEXT("VIII"), TEXT("IX"), TEXT("X"), TEXT("XI"), TEXT("XII")
};

void DrawClock (HDC hdc)
{
    int iAngle;
    POINT pt[3];
    HBRUSH hBrush, hOldBrush;
    HFONT hOldFont = NULL;

    // Set brush color based on dark mode
    hBrush = CreateSolidBrush(g_bDarkMode ? RGB(255,255,255) : RGB(0,0,0));
    hOldBrush = SelectObject(hdc, hBrush);

    for(iAngle = 0; iAngle < 360; iAngle += 6)
    {
        // Reduce radius from 700 to 500 for a smaller clock face
        pt[0].x = 0;
        pt[0].y = 500;

        RotatePoint (pt, 1, iAngle);

        // Reduce tick size for smaller clock
        pt[2].x = pt[2].y = iAngle % 5 ? 18 : 55;

        pt[0].x -= pt[2].x / 2;
        pt[0].y -= pt[2].y / 2;

        pt[1].x = pt[0].x + pt[2].x;
        pt[1].y = pt[0].y + pt[2].y;

        Ellipse (hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);

        // Draw hour numbers using Minecraft font (larger font)
        if (iAngle % 30 == 0 && hMinecraftFont) {
            int hour = iAngle / 30;
            if (hour == 0) hour = 12;
            double rad = iAngle * 3.14159265358979323846 / 180.0;
            int numRadius = 450;
            int tx = (int)(0 + numRadius * sin(rad));
            int ty = (int)(0 + numRadius * cos(rad)) + 35;
            // Use Roman or Arabic numerals
            const TCHAR* label = g_bRomanMode ? romanNumerals[hour] : NULL;
            TCHAR buf[8];
            if (g_bRomanMode) {
                lstrcpy(buf, label);
            } else {
                wsprintf(buf, TEXT("%d"), hour);
            }
            hOldFont = (HFONT)SelectObject(hdc, hMinecraftFont);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_bDarkMode ? RGB(255,255,255) : RGB(0,0,0));
            SIZE sz;
            GetTextExtentPoint32(hdc, buf, lstrlen(buf), &sz);
            TextOut(hdc, tx - sz.cx/2, ty - sz.cy/2, buf, lstrlen(buf));
            SelectObject(hdc, hOldFont);
        }
    }

    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);
}

void DrawHands (HDC hdc, SYSTEMTIME * pst, BOOL fChange)
{
    // Reduce hand lengths for smaller clock
    static POINT pt[3][5] = {
        {0, -110, 70, 0, 0, 300, -70, 0, 0, -110},   // hour hand
        {0, -150, 40, 0, 0, 420, -40, 0, 0, -150},   // minute hand
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 420}            // second hand
    };
    int i, iAngle[3];
    POINT ptTemp[3][5];
    HPEN hPen, hOldPen;
    COLORREF handColor = g_bDarkMode ? RGB(255,255,255) : RGB(0,0,0);

    iAngle[0] = (pst->wHour * 30) % 360 + pst->wMinute / 2;
    iAngle[1] = pst->wMinute * 6;
    iAngle[2] = pst->wSecond * 6;

    memcpy (ptTemp, pt, sizeof (pt));

    hPen = CreatePen(PS_SOLID, 0, handColor);
    hOldPen = SelectObject(hdc, hPen);

    for (i = fChange ? 0 : 2; i < 3; i++)
    {
        RotatePoint (ptTemp[i], 5, iAngle[i]);
        Polyline (hdc, ptTemp[i], 5);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message,
                          WPARAM wParam, LPARAM lParam)
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
            SetTimer (hwnd, ID_TIMER, 1000, NULL);
            GetLocalTime (&st);
            stPrevious = st;
            // Create Roman mode button (top middle, wider)
            hBtnRomanMode = CreateWindow(
                TEXT("BUTTON"), TEXT("Switch to Roman"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 260, 30, hwnd, (HMENU)ID_ROMAN_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            // Create dark mode button (bottom middle)
            hBtnDarkMode = CreateWindow(
                TEXT("BUTTON"), TEXT("Dark Mode"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 180, 30, hwnd, (HMENU)ID_DARKMODE_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            static const TCHAR fontPath[] = TEXT("d:\\Git Uploads\\CLOCK.c\\Minecraft.ttf");
            AddFontResourceEx(fontPath, FR_PRIVATE, 0);

            hMinecraftFont = CreateFont(
                -40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FF_DONTCARE, TEXT("Minecraft"));
            hMinecraftBtnFont = CreateFont(
                -24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FF_DONTCARE, TEXT("Minecraft"));

            if (hBtnDarkMode && hMinecraftBtnFont)
                SendMessage(hBtnDarkMode, WM_SETFONT, (WPARAM)hMinecraftBtnFont, TRUE);
            if (hBtnRomanMode && hMinecraftBtnFont)
                SendMessage(hBtnRomanMode, WM_SETFONT, (WPARAM)hMinecraftBtnFont, TRUE);

            return 0;
        }

        case WM_SIZE:
            cxClient = LOWORD (lParam);
            cyClient = HIWORD (lParam);
            // Position Roman mode button at top middle, dark mode at bottom middle
            if (hBtnRomanMode)
            {
                int btnWidth = 260, btnHeight = 30;
                int x = (cxClient - btnWidth) / 2;
                int y = 10;
                MoveWindow(hBtnRomanMode, x, y, btnWidth, btnHeight, TRUE);
                UpdateWindow(hBtnRomanMode);
            }
            if (hBtnDarkMode)
            {
                int btnWidth = 180, btnHeight = 30;
                int x = (cxClient - btnWidth) / 2;
                int y = cyClient - btnHeight - 10;
                MoveWindow(hBtnDarkMode, x, y, btnWidth, btnHeight, TRUE);
                UpdateWindow(hBtnDarkMode);
            }
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_DARKMODE_BTN)
            {
                g_bDarkMode = !g_bDarkMode;
                SetWindowText(hBtnDarkMode, g_bDarkMode ? TEXT("Light Mode") : TEXT("Dark Mode"));
                if (hBtnDarkMode && hMinecraftBtnFont)
                    SendMessage(hBtnDarkMode, WM_SETFONT, (WPARAM)hMinecraftBtnFont, TRUE);
                InvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hBtnDarkMode);
                return 0;
            }
            if (LOWORD(wParam) == ID_ROMAN_BTN)
            {
                g_bRomanMode = !g_bRomanMode;
                SetWindowText(hBtnRomanMode, g_bRomanMode ? TEXT("Switch to nums") : TEXT("Switch to Roman"));
                if (hBtnRomanMode && hMinecraftBtnFont)
                    SendMessage(hBtnRomanMode, WM_SETFONT, (WPARAM)hMinecraftBtnFont, TRUE);
                InvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hBtnRomanMode);
                return 0;
            }
            break;

        case WM_TIMER:
            GetLocalTime (&st);
            fChange = st.wHour != stPrevious.wHour ||
                       st.wMinute != stPrevious.wMinute;
            
            hdc = GetDC (hwnd);
            
            // Xóa toàn bộ vùng client
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(g_bDarkMode ? BLACK_BRUSH : WHITE_BRUSH));
            
            SetIsotropic (hdc, cxClient, cyClient);
            
            // Vẽ lại đồng hồ (chỉ các vạch chia và kim)
            DrawClock (hdc);
            DrawHands (hdc, &st, TRUE);

            // Force redraw the buttons above the clock
            if (hBtnDarkMode)
                RedrawWindow(hBtnDarkMode, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
            if (hBtnRomanMode)
                RedrawWindow(hBtnRomanMode, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
            
            ReleaseDC (hwnd, hdc);
            
            stPrevious = st;
            return 0;

        case WM_PAINT:
            hdc = BeginPaint (hwnd, &ps);
            
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(g_bDarkMode ? BLACK_BRUSH : WHITE_BRUSH));
            
            SetIsotropic (hdc, cxClient, cyClient);
            
            DrawClock (hdc);
            DrawHands (hdc, &stPrevious, TRUE);

            // Force redraw the buttons above the clock
            if (hBtnDarkMode)
                RedrawWindow(hBtnDarkMode, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
            if (hBtnRomanMode)
                RedrawWindow(hBtnRomanMode, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
            
            EndPaint (hwnd, &ps);
            return 0;

        case WM_DESTROY:
            KillTimer (hwnd, ID_TIMER);
            if (hMinecraftFont) {
                DeleteObject(hMinecraftFont);
                hMinecraftFont = NULL;
            }
            if (hMinecraftBtnFont) {
                DeleteObject(hMinecraftBtnFont);
                hMinecraftBtnFont = NULL;
            }
            RemoveFontResourceEx(TEXT("d:\\Git Uploads\\CLOCK.c\\Minecraft.ttf"), FR_PRIVATE, 0);
            PostQuitMessage (0);
            return 0;
    }
    return DefWindowProc (hwnd, message, wParam, lParam);
}