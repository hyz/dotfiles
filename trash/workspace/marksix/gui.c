/* nuklear - v1.00 - public domain */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 240
#define WINDOW_HEIGHT 320

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_GDI_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_gdi.h"
//#include "nuklear_gdi.c"

#include <windows.h>
#include <ShellApi.h>
void notepad_open(const char* fn) {
    if (fn)
        ShellExecuteA(GetDesktopWindow(), "open", fn, NULL, NULL, SW_SHOW);
}

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    if (nk_gdi_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

int gui_main(char const* (*gen_)(int), void (*poll_)(), void(*stop_)()) //(int ac, char* const av[])
{
    GdiFont* font;
    struct nk_context *ctx;

    WNDCLASSW wc;
    ATOM atom;
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_APPWINDOW;
    HWND wnd;
    HDC dc;
    int running = 1;
    int needs_refresh = 1;

    /* Win32 */
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"NuklearWindowClass";
    atom = RegisterClassW(&wc);

    AdjustWindowRectEx(&rect, style, FALSE, exstyle);

    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"Lucky",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);
    dc = GetDC(wnd);

    /* GUI */
    font = nk_gdifont_create("Arial", 14);
    ctx = nk_gdi_init(font, dc, WINDOW_WIDTH, WINDOW_HEIGHT);
    while (running) {
        MSG msg;
        poll_();

        /* Input */
        nk_input_begin(ctx);
        if (needs_refresh == 0)
        {
            if (GetMessageW(&msg, NULL, 0, 0) <= 0)
            {
                running = 0;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            needs_refresh = 1;
        }
        else
        {
            needs_refresh = 0;
        }
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            needs_refresh = 1;
        }
        nk_input_end(ctx);

        /* GUI */
        {struct nk_panel layout;
        if (nk_begin(ctx, &layout, "Demo", nk_rect(60, 0, 120, 320), 0))
            //NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;

            //nk_layout_row_static(ctx, 30, 80, 1);
            nk_layout_row_dynamic(ctx, 30, 1);
            if (nk_button_label(ctx, "三码", NK_BUTTON_DEFAULT)) {
                notepad_open(gen_(3));
            }
            if (nk_button_label(ctx, "二码", NK_BUTTON_DEFAULT)) {
                notepad_open(gen_(2));
            }
            if (nk_button_label(ctx, "one", NK_BUTTON_DEFAULT)) {
                notepad_open(gen_(1));
            }

#if 0
            if (nk_button_label(ctx, "四码", NK_BUTTON_DEFAULT)) {
                notepad_open(gen_(4));
            }
            if (nk_button_label(ctx, "五码", NK_BUTTON_DEFAULT)) {
                notepad_open(gen_(5));
            }
            if (nk_button_label(ctx, "六码", NK_BUTTON_DEFAULT)) {
                notepad_open(gen_(6));
            }
			if (nk_button_label(ctx, "七码", NK_BUTTON_DEFAULT)) {
				notepad_open(gen_(7));
			}
			if (nk_button_label(ctx, "八码", NK_BUTTON_DEFAULT)) {
				notepad_open(gen_(8));
			}
			if (nk_button_label(ctx, "九码", NK_BUTTON_DEFAULT)) {
				notepad_open(gen_(9));
			}
			if (nk_button_label(ctx, "十码", NK_BUTTON_DEFAULT)) {
				notepad_open(gen_(10));
			}
#endif
            //nk_layout_row_dynamic(ctx, 30, 2);
            //if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            //if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
            //nk_layout_row_dynamic(ctx, 22, 1);
            //nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
        }
        nk_end(ctx);}
        if (nk_window_is_closed(ctx, "Demo")) break;

        /* Draw */
        nk_gdi_render(nk_rgb(30,30,30));
    }
    stop_();

    nk_gdifont_del(font);
    ReleaseDC(wnd, dc);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

