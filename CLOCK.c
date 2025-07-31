/*--------------------------
    CLOCK.C -- Analog Clock Program
    (Complete self-contained version)
---------------------------*/

#include <windows.h>
#include <math.h> 
#include <stdio.h>
#include <mmsystem.h> 
#include <shellapi.h>
#include <tchar.h>
#include <shlwapi.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

#define ID_TIMER 1
#define ID_DARKMODE_BTN 2
#define ID_ROMAN_BTN 3
#define ID_FONT_BTN 4
#define ID_SOUND_BTN 5
#define ID_DOTS_BTN 6
#define ID_MYSTERY_BTN 7
#define TWOPI (2*3.14159)

// Resource IDs
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
void SetIsotropic(HDC hdc, int cxClient, int cyClient);
void RotatePoint(POINT pt[], int iNum, int iAngle);
void DrawClock(HDC hdc);
void DrawHands(HDC hdc, SYSTEMTIME * pst, BOOL fChange);
void DrawColorfulQuestionMarks(HWND hwnd);

// Helper function to get executable directory
BOOL GetExeDirectory(TCHAR* buffer, DWORD size)
{
    if (!GetModuleFileName(NULL, buffer, size))
        return FALSE;
    
    PathRemoveFileSpec(buffer);
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
        hSoundOnIcon = LoadIcon(NULL, IDI_INFORMATION);
        hSoundOffIcon = LoadIcon(NULL, IDI_WARNING);
    }

    // Load Minecraft font
    HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(IDR_MC_FONT), RT_FONT);
    if (hRes)
    {
        HGLOBAL hFontRes = LoadResource(hInstance, hRes);
        if (hFontRes)
        {
            DWORD nFonts = 0;
            DWORD fontSize = SizeofResource(hInstance, hRes);
            void* fontData = LockResource(hFontRes);
            AddFontMemResourceEx(fontData, fontSize, NULL, &nFonts);
        }
    }

    // Load Dogica font with proper error handling
    hRes = FindResource(hInstance, MAKEINTRESOURCE(IDR_PIXEL_FONT), RT_FONT);
    if (!hRes)
    {
        MessageBox(NULL, TEXT("Dogica font resource not found!"), TEXT("Error"), MB_ICONERROR);
    }
    else
    {
        HGLOBAL hFontRes = LoadResource(hInstance, hRes);
        if (!hFontRes)
        {
            MessageBox(NULL, TEXT("Failed to load Dogica font resource!"), TEXT("Error"), MB_ICONERROR);
        }
        else
        {
            DWORD nFonts = 0;
            DWORD fontSize = SizeofResource(hInstance, hRes);
            void* fontData = LockResource(hFontRes);
            
            HANDLE hFont = AddFontMemResourceEx(fontData, fontSize, NULL, &nFonts);
            if (!hFont)
            {
                MessageBox(NULL, TEXT("Failed to register Dogica font!"), TEXT("Error"), MB_ICONERROR);
            }
        }
    }

    // Create fonts with verification
    hMinecraftFont = CreateFont(-40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_DONTCARE, TEXT("Minecraft"));

    hMinecraftBtnFont = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_DONTCARE, TEXT("Minecraft"));

    // Try multiple variations of Dogica font names
    const TCHAR* dogicaNames[] = {
        TEXT("Dogica"),
        TEXT("Dogica Pixel"),
        TEXT("Dogica Bold"),
        TEXT("Dogica Regular"),
        TEXT("Pixel"),
        NULL
    };

    hPixelFont = NULL;
    hPixelBtnFont = NULL;

    // Try all possible names for the main font
    for (int i = 0; dogicaNames[i] && !hPixelFont; i++)
    {
        hPixelFont = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE, dogicaNames[i]);
        
        // Verify the font was actually created with the requested face
        if (hPixelFont)
        {
            HDC hdc = GetDC(NULL);
            HFONT oldFont = (HFONT)SelectObject(hdc, hPixelFont);
            
            TCHAR actualFontName[LF_FACESIZE];
            GetTextFace(hdc, LF_FACESIZE, actualFontName);
            
            SelectObject(hdc, oldFont);
            ReleaseDC(NULL, hdc);
            
            if (_tcsicmp(actualFontName, dogicaNames[i]))
            {
                DeleteObject(hPixelFont);
                hPixelFont = NULL;
            }
        }
    }

    // Try all possible names for the button font
    for (int i = 0; dogicaNames[i] && !hPixelBtnFont; i++)
    {
        hPixelBtnFont = CreateFont(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE, dogicaNames[i]);
            
        // Verify the button font
        if (hPixelBtnFont)
        {
            HDC hdc = GetDC(NULL);
            HFONT oldFont = (HFONT)SelectObject(hdc, hPixelBtnFont);
            
            TCHAR actualFontName[LF_FACESIZE];
            GetTextFace(hdc, LF_FACESIZE, actualFontName);
            
            SelectObject(hdc, oldFont);
            ReleaseDC(NULL, hdc);
            
            if (_tcsicmp(actualFontName, dogicaNames[i]))
            {
                DeleteObject(hPixelBtnFont);
                hPixelBtnFont = NULL;
            }
        }
    }

    // Final fallback if Dogica not found
    if (!hPixelFont || !hPixelBtnFont)
    {
        MessageBox(NULL, 
            TEXT("Dogica font not available. Using Arial instead.\n")
            TEXT("Please ensure the Dogica font is properly embedded."), 
            TEXT("Font Warning"), MB_ICONWARNING);
            
        hPixelFont = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE, TEXT("Arial"));
            
        hPixelBtnFont = CreateFont(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FF_DONTCARE, TEXT("Arial"));
    }

    // Fallback for Minecraft fonts
    if (!hMinecraftFont || !hMinecraftBtnFont)
    {
        hMinecraftFont = hMinecraftBtnFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
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

void UpdateButtonFonts(HWND hwnd)
{
    HFONT hCurrentBtnFont = g_bUsePixelFont ? hPixelBtnFont : hMinecraftBtnFont;
    
    if (hBtnRomanMode)
        SendMessage(hBtnRomanMode, WM_SETFONT, (WPARAM)hCurrentBtnFont, TRUE);
    if (hBtnDarkMode)
        SendMessage(hBtnDarkMode, WM_SETFONT, (WPARAM)hCurrentBtnFont, TRUE);
    if (hBtnFontSwitch)
        SendMessage(hBtnFontSwitch, WM_SETFONT, (WPARAM)hCurrentBtnFont, TRUE);
    if (hBtnDots)
        SendMessage(hBtnDots, WM_SETFONT, (WPARAM)hCurrentBtnFont, TRUE);
    if (hBtnMystery)
        SendMessage(hBtnMystery, WM_SETFONT, (WPARAM)hCurrentBtnFont, TRUE);
    
    // Redraw all buttons
    InvalidateRect(hBtnRomanMode, NULL, TRUE);
    InvalidateRect(hBtnDarkMode, NULL, TRUE);
    InvalidateRect(hBtnFontSwitch, NULL, TRUE);
    InvalidateRect(hBtnDots, NULL, TRUE);
    InvalidateRect(hBtnMystery, NULL, TRUE);
}

void PlayMysteryVideo()
{
    TCHAR exePath[MAX_PATH];
    if (GetExeDirectory(exePath, MAX_PATH))
    {
        TCHAR videoPath[MAX_PATH];
        PathCombine(videoPath, exePath, TEXT("ASTELLION.mp4"));
        
        if (PathFileExists(videoPath))
        {
            ShellExecute(NULL, TEXT("open"), videoPath, NULL, NULL, SW_SHOWNORMAL);
        }
        else
        {
            MessageBox(NULL, 
                TEXT("Mystery video not found!\nPlease place ASTELLION.mp4 in:\n") TEXT("%s"),
                TEXT("Clock"), 
                MB_ICONINFORMATION);
        }
    }
    else
    {
        MessageBox(NULL, 
            TEXT("Could not locate program directory!"), 
            TEXT("Clock"), MB_ICONINFORMATION);
    }
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

    if (g_bShowDots)
    {
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
        }
    }

    for (iAngle = 0; iAngle < 360; iAngle += 30)
    {
        if (hCurrentFont) {
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

void DrawColorfulQuestionMarks(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, g_bUsePixelFont ? hPixelBtnFont : hMinecraftBtnFont);
    
    const TCHAR* text = TEXT("???");
    SIZE sz;
    GetTextExtentPoint32(hdc, text, 3, &sz);
    int x = (rc.right - sz.cx) / 2;
    int y = (rc.bottom - sz.cy) / 2;
    
    SetBkMode(hdc, TRANSPARENT);
    
    for (int i = 0; i < 3; i++)
    {
        COLORREF colors[] = {RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255)};
        SetTextColor(hdc, colors[i]);
        TCHAR ch[2] = {text[i], 0};
        TextOut(hdc, x + i * (sz.cx / 3), y, ch, 1);
    }
    
    SelectObject(hdc, hOldFont);
    EndPaint(hwnd, &ps);
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

            // Create buttons
            hBtnSound = CreateWindow(
                TEXT("BUTTON"), NULL,
                WS_CHILD | WS_VISIBLE | BS_ICON,
                10, 10, 40, 40,
                hwnd, (HMENU)ID_SOUND_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

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

            hBtnMystery = CreateWindow(
                TEXT("BUTTON"), TEXT("???"),
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                0, 0, 50, 50,
                hwnd, (HMENU)ID_MYSTERY_BTN,
                ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            // Set initial icon
            SendMessage(hBtnSound, BM_SETIMAGE, IMAGE_ICON,
                (LPARAM)(g_bSoundOn ? hSoundOnIcon : hSoundOffIcon));

            UpdateButtonFonts(hwnd);

            return 0;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
            if (lpDrawItem->CtlID == ID_MYSTERY_BTN)
            {
                FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, 
                        (HBRUSH)(COLOR_BTNFACE + 1));
                
                if (lpDrawItem->itemState & ODS_SELECTED)
                    DrawEdge(lpDrawItem->hDC, &lpDrawItem->rcItem, EDGE_SUNKEN, BF_RECT);
                else
                    DrawEdge(lpDrawItem->hDC, &lpDrawItem->rcItem, EDGE_RAISED, BF_RECT);
                
                HFONT hOldFont = (HFONT)SelectObject(lpDrawItem->hDC, 
                    g_bUsePixelFont ? hPixelBtnFont : hMinecraftBtnFont);
                
                const TCHAR* text = TEXT("???");
                SIZE sz;
                GetTextExtentPoint32(lpDrawItem->hDC, text, 3, &sz);
                int x = (lpDrawItem->rcItem.right - sz.cx) / 2;
                int y = (lpDrawItem->rcItem.bottom - sz.cy) / 2;
                
                SetBkMode(lpDrawItem->hDC, TRANSPARENT);
                
                for (int i = 0; i < 3; i++)
                {
                    COLORREF colors[] = {RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255)};
                    SetTextColor(lpDrawItem->hDC, colors[i]);
                    TCHAR ch[2] = {text[i], 0};
                    TextOut(lpDrawItem->hDC, x + i * (sz.cx / 3), y, ch, 1);
                }
                
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
                MoveWindow(hBtnRomanMode, (cxClient - 260)/2, 10, 260, 50, TRUE);
            if (hBtnDarkMode)
                MoveWindow(hBtnDarkMode, (cxClient - 180)/2, cyClient-60, 180, 50, TRUE);
            if (hBtnFontSwitch)
                MoveWindow(hBtnFontSwitch, cxClient-410, 10, 400, 40, TRUE);
            if (hBtnSound)
                MoveWindow(hBtnSound, 10, 10, 40, 40, TRUE);
            if (hBtnDots)
                MoveWindow(hBtnDots, 10, cyClient-60, 220, 50, TRUE);
            if (hBtnMystery)
                MoveWindow(hBtnMystery, cxClient-60, cyClient-60, 50, 50, TRUE);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case ID_DARKMODE_BTN:
                    g_bDarkMode = !g_bDarkMode;
                    SetWindowText(hBtnDarkMode, g_bDarkMode ? TEXT("Light Mode") : TEXT("Dark Mode"));
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                    
                case ID_ROMAN_BTN:
                    g_bRomanMode = !g_bRomanMode;
                    SetWindowText(hBtnRomanMode, g_bRomanMode ? TEXT("Switch to nums") : TEXT("Switch to Roman"));
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                    
                case ID_FONT_BTN:
                    g_bUsePixelFont = !g_bUsePixelFont;
                    SetWindowText(hBtnFontSwitch, g_bUsePixelFont ? TEXT("Switch to Minecraft") : TEXT("Switch to Pixel"));
                    UpdateButtonFonts(hwnd);
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                    
                case ID_SOUND_BTN:
                    g_bSoundOn = !g_bSoundOn;
                    SendMessage(hBtnSound, BM_SETIMAGE, IMAGE_ICON,
                        (LPARAM)(g_bSoundOn ? hSoundOnIcon : hSoundOffIcon));
                    return 0;
                    
                case ID_DOTS_BTN:
                    g_bShowDots = !g_bShowDots;
                    SetWindowText(hBtnDots, g_bShowDots ? TEXT("Disable Dots") : TEXT("Enable Dots"));
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                    
                case ID_MYSTERY_BTN:
                    PlayMysteryVideo();
                    return 0;
            }
            break;

        case WM_TIMER:
            GetLocalTime(&st);
            fChange = st.wHour != stPrevious.wHour || st.wMinute != stPrevious.wMinute;

            hdc = GetDC(hwnd);
            
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(g_bDarkMode ? BLACK_BRUSH : WHITE_BRUSH));
            
            SetIsotropic(hdc, cxClient, cyClient);
            DrawClock(hdc);
            DrawHands(hdc, &st, TRUE);

            if (g_bSoundOn) {
                TCHAR soundPath[MAX_PATH];
                if (GetExeDirectory(soundPath, MAX_PATH))
                {
                    PathCombine(soundPath, soundPath, TEXT("Tick.wav"));
                    if (PathFileExists(soundPath))
                        PlaySound(soundPath, NULL, SND_FILENAME | SND_ASYNC);
                }
            }

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
            
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(g_bDarkMode ? BLACK_BRUSH : WHITE_BRUSH));
            
            SetIsotropic(hdc, cxClient, cyClient);
            DrawClock(hdc);
            DrawHands(hdc, &stPrevious, TRUE);
            
            EndPaint(hwnd, &ps);
            
            RedrawWindow(hBtnSound, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnRomanMode, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnDarkMode, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnFontSwitch, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnDots, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            RedrawWindow(hBtnMystery, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER);
            FreeResources();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}