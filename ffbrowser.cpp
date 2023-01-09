#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <Windows.h>
#include <Windowsx.h>
#include <commctrl.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>
// <IncludeHeader>
// include WebView2 header
#include "WebView2.h"
// </IncludeHeader>
#include "targetver.h"
#include "resource.h"

using namespace Microsoft::WRL;

wil::com_ptr<ICoreWebView2Controller> g_webviewController;
wil::com_ptr<ICoreWebView2> g_webviewWindow;

#define CLASSNAME TEXT("ffbrowser")

class FFBROWSER
{
public:
    HINSTANCE m_hInst;
    INT m_argc;
    LPWSTR* m_argv;

    FFBROWSER(HINSTANCE hInst, INT argc, LPWSTR* argv)
        : m_hInst(hInst), m_argc(argc), m_argv(argv)
    {
    }

    ~FFBROWSER()
    {
    }

    INT start()
    {
        WNDCLASSEX wcx = { sizeof(wcx) };
        wcx.hInstance = m_hInst;
        wcx.lpszClassName = CLASSNAME;
        wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wcx.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcx.lpfnWndProc = WindowProc;
        if (!RegisterClassEx(&wcx))
        {
            MessageBoxA(NULL, "RegisterClassEx failed", NULL, MB_ICONERROR);
            return -1;
        }

        DWORD style = WS_OVERLAPPEDWINDOW;
        HWND hwnd = CreateWindow(CLASSNAME, TEXT("ffbrowser"), style,
            CW_USEDEFAULT, CW_USEDEFAULT, 600, 480, NULL, NULL, m_hInst, this);
        if (hwnd == NULL)
        {
            MessageBoxA(NULL, "CreateWindow failed", NULL, MB_ICONERROR);
            return -2;
        }

        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return (INT)msg.wParam;
    }

    bool create_browser(HWND hWnd)
    {
        CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                    // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
                    env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (controller != nullptr) {
                                g_webviewController = controller;
                                g_webviewController->get_CoreWebView2(&g_webviewWindow);
                            }

                            // Add a few settings for the webview
                            // The demo step is redundant since the values are the default settings
                            ICoreWebView2Settings* Settings;
                            g_webviewWindow->get_Settings(&Settings);
                            Settings->put_IsScriptEnabled(TRUE);
                            Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                            Settings->put_IsWebMessageEnabled(TRUE);

                            // Resize the WebView2 control to fit the bounds of the parent window
                            RECT bounds;
                            GetClientRect(hWnd, &bounds);
                            g_webviewController->put_Bounds(bounds);

                            // Schedule an async task to navigate to Bing
                            g_webviewWindow->Navigate(L"https://www.google.com/");

                            // 4 - Navigation events
                            // 5 - Scripting
                            // 6 - Communication between host and web content
                            return S_OK;
                        }).Get()
                    );
                    return S_OK;
                }
            ).Get()
        );
        return true;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        create_browser(hwnd);
        return TRUE;
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        if (g_webviewController != nullptr) 
        {
            RECT bounds;
            GetClientRect(hwnd, &bounds);
            g_webviewController->put_Bounds(bounds);
        }
    }

    void OnDestroy(HWND hwnd)
    {
        PostQuitMessage(0);
    }

    LRESULT CALLBACK WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        FFBROWSER* pThis;
        if (uMsg == WM_NCCREATE)
        {
            LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
            pThis = (FFBROWSER*)lpcs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        }
        else
        {
            pThis = (FFBROWSER*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }


        if (pThis)
            return pThis->WindowProcDx(hwnd, uMsg, wParam, lParam);
        else
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};

INT ffbrowser_main(HINSTANCE hInstance, INT argc, LPWSTR* argv)
{
    ::InitCommonControls();
    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    FFBROWSER ffb(hInstance, argc, argv);
    INT ret = ffb.start();

    if (SUCCEEDED(hr))
        ::CoUninitialize();
    return ret;
}

INT WINAPI
wWinMain(HINSTANCE   hInstance,
         HINSTANCE   hPrevInstance,
         LPWSTR      lpCmdLine,
         INT         nCmdShow)
{
    INT argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    INT ret = ffbrowser_main(hInstance, argc, argv);
    LocalFree(argv);
    return ret;
}
