#include "eb_opsimulator.h"


void eb_opsimulator_mouse_move(HWND hwnd, POINT pt)
{
    assert(hwnd != NULL);
    
    PostMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(100);
}

void eb_opsimulator_mouse_lclick(HWND hwnd, POINT pt)
{
    assert(hwnd != NULL);

    PostMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(300);
    PostMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(100);
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(100);
}

void eb_opsimulator_mouse_rclick(HWND hwnd, POINT pt)
{
    assert(hwnd != NULL);

    PostMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(300);
    PostMessage(hwnd, WM_RBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(100);
    PostMessage(hwnd, WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(100);
}

void eb_opsimulator_mouse_dblclick(HWND hwnd, POINT pt)
{
    assert(hwnd != NULL);

    PostMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(100);
    PostMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(50);
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(50);
    PostMessage(hwnd, WM_LBUTTONDBLCLK, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
    Sleep(50);
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
    Sleep(50);
}

void eb_opsimulator_mouse_drag_and_drop(HWND hwnd, POINT src, POINT dest)
{
    assert(hwnd != NULL);

    PostMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(src.x, src.y));
    Sleep(100);
    PostMessage(hwnd, WM_LBUTTONDOWN, 0, MAKELPARAM(src.x, src.y));
    Sleep(500);
    PostMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(dest.x, dest.y));
    Sleep(100);
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(dest.x, dest.y));
    Sleep(100);
}

void eb_opsimulator_press_key(HWND hwnd, UINT vkCode, bool ctrl, bool shift, bool alt, int hold_time)
{
    assert(hwnd != NULL);

    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    UINT ctrlScanCode = MapVirtualKey(VK_CONTROL, MAPVK_VK_TO_VSC);
    UINT shiftScanCode = MapVirtualKey(VK_SHIFT, MAPVK_VK_TO_VSC);
    UINT altScanCode = MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC);

    if (ctrl)
        PostMessage(hwnd, WM_KEYDOWN, VK_CONTROL, 1 | (ctrlScanCode << 16));
    if (shift)
        PostMessage(hwnd, WM_KEYDOWN, VK_SHIFT, 1 | (shiftScanCode << 16));
    if (alt)
        PostMessage(hwnd, WM_KEYDOWN, VK_MENU, 1 | (altScanCode << 16));
    Sleep(100);
    PostMessage(hwnd, WM_KEYDOWN, vkCode, 1 | (scanCode << 16));
    Sleep(hold_time);
    PostMessage(hwnd, WM_KEYUP, vkCode, 1 | (scanCode << 16));
    Sleep(100);
    if (alt)
        PostMessage(hwnd, WM_KEYUP, VK_MENU, 1 | (altScanCode << 16));
    if (shift)
        PostMessage(hwnd, WM_KEYUP, VK_SHIFT, 1 | (shiftScanCode << 16));
    if (ctrl)
        PostMessage(hwnd, WM_KEYUP, VK_CONTROL, 1 | (ctrlScanCode << 16));
}
