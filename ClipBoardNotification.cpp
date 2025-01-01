// ClipBoardNotification.cpp : 定义应用程序的入口点。
//
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <iostream>
#include <sstream>

#include "framework.h"
#include "ClipBoardNotification.h"

#define ID_EXIT 2
#define ID_SHOW_CLIPBOARD 3
#define OPEN_POPUP_WINDOW 4
#define NOTIFICATION_TRAY_ICON_MSG (WM_USER + 0x100)

HINSTANCE hInst;
HWND hwndMain;
NOTIFYICONDATA nid;
std::wstring clipboardText = L"";
BITMAP clipboardBitmap;
LPCWSTR clipboardStr = NULL;
HBITMAP clipboardImg = NULL;

// 显示托盘图标
void ShowTrayIcon(HWND hwnd) {
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = NOTIFICATION_TRAY_ICON_MSG;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIPBOARDNOTIFICATION)); // 设置窗口图标
    wcscpy_s(nid.szTip, L"Clipboard Monitor");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

// 移除托盘图标
void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// 修复颜色反转的函数（如果存在反转问题）
void FixColorInversion() {
    // 获取图像数据
    if (clipboardBitmap.bmBitsPixel == 32) {
        // 如果是32位颜色位图，可能存在透明通道或颜色反转问题
        BYTE* bits = (BYTE*)clipboardBitmap.bmBits;
        int imageSize = clipboardBitmap.bmWidthBytes * clipboardBitmap.bmHeight;
        printf("Fix Inverse: %d\n", imageSize);
        for (int i = 0; i < imageSize; i += 4) {
            // 交换每个像素的RGB值（如果是反转问题的话）
            BYTE temp = bits[i];
            bits[i] = bits[i + 2];
            bits[i + 2] = temp;
        }
    }
}

// 获取剪切板内容
int GetClipboard() {
    if (!OpenClipboard(NULL)) {
        return -1; // cannot open clipboard
    }

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) {
        // 不是文字, 尝试图像
        hData = GetClipboardData(CF_DIB);
        if (hData == NULL) {
            CloseClipboard();
            return -2; // neither img nor text
        }
        // 获取图像数据
        BITMAPINFO* pBitmapInfo = (BITMAPINFO*)GlobalLock(hData);
        if (pBitmapInfo == NULL) {
            GlobalUnlock(hData);
            CloseClipboard();
            return -4; // not valid bitmap
        }
        // 获取位图数据
        void* pBits = (void*)((BYTE*)pBitmapInfo + pBitmapInfo->bmiHeader.biSize);
        // 获取BITMAP对象
        clipboardImg = CreateDIBitmap(GetDC(NULL), &pBitmapInfo->bmiHeader, CBM_INIT, pBits, pBitmapInfo, DIB_RGB_COLORS);
        GetObject(clipboardImg, sizeof(BITMAP), &clipboardBitmap);
        // FixColorInversion();
        clipboardStr = NULL;
    }
    else {
        // 获取剪切板文字
        wchar_t* pText = static_cast<wchar_t*>(GlobalLock(hData));
        if (pText == NULL) {
            GlobalUnlock(hData);
            CloseClipboard();
            return -3; // not valid text
        }
        clipboardText = pText;
        clipboardStr = clipboardText.c_str();
        clipboardImg = NULL;
    }
    GlobalUnlock(hData);
    CloseClipboard();
    return 0;
}

/* // 创建无边框预览窗口并返回窗口句柄
HWND ShowClipboardPreview0(const std::wstring& text) {
    HWND hwndPreview = CreateWindowEx(
        WS_EX_TOOLWINDOW, L"STATIC", L"Clipboard Preview",
        WS_POPUP | SS_LEFT,
        200, 200, 300, 100, hwndMain, NULL, hInst, NULL
    );

    SetWindowText(hwndPreview, text.c_str());
    SetWindowLong(hwndPreview, GWL_STYLE, GetWindowLong(hwndPreview, GWL_STYLE) & ~WS_BORDER);
    SetWindowPos(hwndPreview, HWND_TOPMOST, 200, 200, 300, 100, SWP_NOACTIVATE);

    // 设置定时器，自动关闭窗口
    SetTimer(hwndMain, 1, 3000, NULL); // 3秒后关闭

    ShowWindow(hwndPreview, SW_SHOWNORMAL);
    return hwndPreview;  // 返回窗口句柄
}
*/

// 判断字符是否是中文字符
bool isChinese(wchar_t ch) {
    return (ch >= 0x4E00 && ch <= 0x9FFF);  // 简单检查是否是常见中文字符范围
}

// 计算 std::wstring 的总行数，中文字符算作 2.5 个字符
int getWstringLen() {
    std::wstringstream wss(clipboardText);
    std::wstring line;
    int totalLines = 0;

    // 读取每一行
    while (std::getline(wss, line, L'\n')) {
        double lineLength = 0.0;  // 使用 double 来处理权重（2.5个字符）

        // 遍历每一行中的字符，判断并累加字符长度
        for (wchar_t ch : line) {
            if (isChinese(ch)) {
                lineLength += 2.5;  // 中文字符算作 2.5 个字符
            }
            else {
                lineLength += 1.0;  // 其他字符算作 1 个字符
            }
        }

        // 计算当前行需要多少行才能显示，按每行 20 个字符来分割
        int linesForThisRow = static_cast<int>(std::ceil(lineLength / 50.0));  // 向上取整
        totalLines += linesForThisRow;
    }

    return totalLines;
}

SIZE getImageSize(int w, int h) {
    float scale = 1.0f * w / clipboardBitmap.bmWidth;
    float newH = clipboardBitmap.bmHeight * scale;
    if (newH > h) {
        scale *= w / newH;
    }
    return {
        (int)(clipboardBitmap.bmWidth * scale),
        (int)(clipboardBitmap.bmHeight * scale)
    };
}

// 创建无边框预览窗口并返回窗口句柄
HWND ShowClipboardPreview(int time_ms) {
    if (!clipboardStr && !clipboardImg) {
        return NULL;
    }
    // 获取屏幕的工作区（即不被任务栏遮挡的区域）
    RECT screenRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);

    // 计算右下角位置，距离屏幕右边和下边各 50 像素
    int screenWidth = screenRect.right - screenRect.left;
    int screenHeight = screenRect.bottom - screenRect.top;
    int windowWidth = 400;
    // 计算文本行数
    int windowHeight = 300;
    if (clipboardStr) {
        // 计算文本行数
        windowHeight = getWstringLen() * 16;
        // 高度在 100 到 500 之间
        if (windowHeight > 500) {
            windowHeight = 500;
        }
        else if (windowHeight < 100) {
            windowHeight = 100;
        }
    }
    else {
        // 计算图像大小
        SIZE size = getImageSize(400, 300);
        windowWidth = size.cx + 2;
        windowHeight = size.cy + 2;
        if (windowWidth < 40) windowWidth = 40;
        if (windowHeight < 40) windowHeight = 40;
    }

    // 计算窗口的左上角位置，距离右边和下边各 50 像素
    int windowX = screenWidth - windowWidth - 50;
    int windowY = screenHeight - windowHeight - 50;

    // 创建无边框预览窗口
    HWND hwndPreview = CreateWindowEx(
        WS_EX_TOOLWINDOW, L"STATIC", L"Clipboard Preview",
        WS_POPUP | WS_BORDER, // 禁用自动换行，启用水平滚动
        windowX, windowY, windowWidth, windowHeight, hwndMain, NULL, hInst, NULL
    );

    if (clipboardStr != NULL) {
        // 设置窗口文本内容
        SetWindowText(hwndPreview, clipboardStr);
    }

    // 设置窗口无边框
    SetWindowLong(hwndPreview, GWL_STYLE, GetWindowLong(hwndPreview, GWL_STYLE) & ~WS_BORDER);

    // 将窗口置于最上层，且不激活
    SetWindowPos(hwndPreview, HWND_TOPMOST, windowX, windowY, windowWidth, windowHeight, SWP_NOACTIVATE);

    // 设置定时器，自动关闭窗口，3秒后关闭
    SetTimer(hwndMain, 1, time_ms, NULL);  // 3秒后关闭窗口

    // 显示窗口
    ShowWindow(hwndPreview, SW_SHOWNORMAL);

    // 仅当剪切板图像已读取时，才绘制
    if (clipboardImg) {
        printf("biHeader: %d biData:0x%X\n", clipboardBitmap.bmBitsPixel, clipboardBitmap.bmBits);
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwndPreview, &ps);

        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, clipboardImg);

        // 将图像绘制到窗口
        SIZE wh = getImageSize(400, 300);
        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchBlt(hdc, 1, 1, wh.cx, wh.cy, hMemDC, 0, 0, clipboardBitmap.bmWidth, clipboardBitmap.bmHeight, SRCCOPY);

        // 清理资源
        SelectObject(hMemDC, hOldBitmap);
        DeleteDC(hMemDC);

        EndPaint(hwndPreview, &ps);
    }

    return hwndPreview;  // 返回窗口句柄
}

HWND hwndPreview = NULL;  // 保存预览窗口句柄
void ShowPopupWindow(int time_ms = 1000) {
    KillTimer(hwndMain, OPEN_POPUP_WINDOW);
    if (hwndPreview != NULL) {
        DestroyWindow(hwndPreview);  // 销毁窗口
        hwndPreview = NULL;  // 清空句柄
    }
    if (GetClipboard() == 0) {
        hwndPreview = ShowClipboardPreview(time_ms);  // 保存预览窗口句柄
    }
}

// 处理消息循环
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    switch (msg) {
    case WM_CREATE:
        ShowTrayIcon(hwnd);
        AddClipboardFormatListener(hwnd);  // 注册剪切板监听
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_EXIT) {
            RemoveTrayIcon();
            PostQuitMessage(0);
        }
        else if (LOWORD(wParam) == ID_SHOW_CLIPBOARD) {
            // 设置定时器，自动打开窗口
            ShowPopupWindow(5000);
        }
        break;

    case WM_CLIPBOARDUPDATE:
        // 当剪切板更新时，读取内容并显示预览
        SetTimer(hwndMain, OPEN_POPUP_WINDOW, 50, NULL); // 50ms后打开
        break;

    case WM_TIMER:
        if (wParam == 1) {
            // 自动关闭预览窗口
            if (hwndPreview) {
                DestroyWindow(hwndPreview);  // 销毁窗口
                hwndPreview = NULL;  // 清空句柄
            }
        }
        else if (wParam == OPEN_POPUP_WINDOW) {
            ShowPopupWindow();
        }
        break;

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    case WM_SYSCOMMAND:
        if (wParam == SC_MINIMIZE) {
            ShowWindow(hwnd, SW_HIDE);
        }
        break;

    case WM_RBUTTONUP:
    case NOTIFICATION_TRAY_ICON_MSG: // 不知道为什么但是托盘事件是这个
    {
        if (lParam == WM_RBUTTONUP) {
            // 托盘右键菜单
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_SHOW_CLIPBOARD, L"Show Clipboard");
            AppendMenu(hMenu, MF_STRING, ID_EXIT, L"Exit");

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        else if (lParam == WM_LBUTTONUP) {
            ShowPopupWindow(3000);
        }
    }
    break;

    default:
        // std::cerr << "oct: " << msg << std::endl;
        printf("Msg: 0x%X\n", msg);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// WinMain：应用程序的入口点
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    printf("Hello, World! WinMain.\n");
    // 初始化窗口类
    hInst = hInstance;

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = L"ClipboardMonitorWindow";
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIPBOARDNOTIFICATION)); // 设置主窗口图标

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Window Class Registration Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    hwndMain = CreateWindow(wc.lpszClassName, L"Clipboard Monitor",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        200, 200, NULL, NULL, hInst, NULL);

    if (!hwndMain) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    // 进入消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

