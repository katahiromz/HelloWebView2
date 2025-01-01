// HelloWebView2 --- A sample code of WebView2
// License: MIT
#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"

using namespace Microsoft::WRL;

#define CLASSNAME _T("HelloWebView2 by katahiromz")
#define TITLE _T("HelloWebView2")
#define URL L"https://google.com/"

// Global variables
HINSTANCE g_hInst;
static wil::com_ptr<ICoreWebView2Controller> g_webviewController;
static wil::com_ptr<ICoreWebView2> g_webView;

// The window procedure
LRESULT CALLBACK
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SIZE:
        if (g_webviewController != nullptr)
        {
            RECT bounds;
            GetClientRect(hWnd, &bounds);
            g_webviewController->put_Bounds(bounds);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// The Windows app main function
INT WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    INT       nCmdShow)
{
    g_hInst = hInstance;

    // Register window classes
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = CLASSNAME;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, _T("RegisterClassEx failed"), TITLE, MB_ICONERROR);
        return 1;
    }

    // Create the main window
    DWORD style = WS_OVERLAPPEDWINDOW;
    HWND hWnd = CreateWindow(CLASSNAME, TITLE, style,
                             CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                             NULL,
                             NULL,
                             hInstance,
                             NULL);
    if (!hWnd)
    {
        MessageBox(NULL, _T("CreateWindow failed"), TITLE, MB_ICONERROR);
        return 1;
    }

    // Show the main window
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // WebView2 code starts here
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
                env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        // Initialize webview
                        if (controller != nullptr) {
                            g_webviewController = controller;
                            g_webviewController->get_CoreWebView2(&g_webView);
                        }

                        // Add some webview settings
                        wil::com_ptr<ICoreWebView2Settings> settings;
                        g_webView->get_Settings(&settings);
                        settings->put_IsScriptEnabled(TRUE);
                        settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                        settings->put_IsWebMessageEnabled(TRUE);
                        settings->put_AreDefaultContextMenusEnabled(TRUE);
                        settings->put_IsStatusBarEnabled(FALSE);
                        settings->put_IsZoomControlEnabled(FALSE);
                        settings->put_IsBuiltInErrorPageEnabled(TRUE);
#ifdef NDEBUG
                        settings->put_AreDevToolsEnabled(FALSE);
#endif

                        // Resize the window content
                        PostMessage(hWnd, WM_SIZE, 0, 0);

                        // Navigate
#if 1
                        g_webView->Navigate(URL);
#else
                        auto content = L"<html><body><h1>HelloWebView2</h1>"
                            L"<p>This is an HTML.</p>"
                            L"<script>window.chrome.webview.postMessage('This is a message from JavaScript to native');"
                            L"window.chrome.webview.addEventListener('message', event => { alert(event.data); });</script>"
                            L"</body></html>";
                        g_webView->NavigateToString(content);
#endif

                        // Register an ICoreWebView2NavigationStartingEventHandler to cancel some navigation
                        EventRegistrationToken token;
                        g_webView->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
                            [](ICoreWebView2* g_webView, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                wil::unique_cotaskmem_string uri;
                                args->get_Uri(&uri);
                                std::wstring source(uri.get());
                                auto scheme = source.substr(0, 5);
                                if (scheme != L"https" && scheme != L"http:" && scheme != L"data:") {
                                    args->put_Cancel(true);
                                }
                                return S_OK;
                            }).Get(), &token
                        );

#if 0
                        // Receive message from JavaScript code: window.chrome.webview.postMessage("...");
                        g_webView->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                            [](ICoreWebView2* webView, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                wil::unique_cotaskmem_string message;
                                args->TryGetWebMessageAsString(&message);
                                std::wstring msg = message.get();
                                msg += L'\n';
                                ::OutputDebugStringW(msg.c_str()); // Debug output
                                return S_OK;
                            }).Get(), &token
                        );
#endif

#if 0
                        // Post message to JavaScript
                        g_webView->PostWebMessageAsString(L"This is a message from native to JavaScript");
#endif

                        return S_OK;
                    }).Get());
                return S_OK;
            }
        ).Get()
    );
    // WebView2 code ends here

    // The message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (INT)msg.wParam;
}
