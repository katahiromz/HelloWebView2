// HelloWebView2 --- A sample code of WebView2
// License: MIT

// Detect memory leaks (MSVC Debug only)
#if defined(_MSC_VER) && !defined(NDEBUG) && !defined(_CRTDBG_MAP_ALLOC)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include <windows.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <shlwapi.h>
#ifdef _MSC_VER
    #include "WebView2.h"
#else
    #include "compat/WebView2.h"
#endif

#define CLASSNAME _T("HelloWebView2 by katahiromz")
#define TITLE _T("HelloWebView2")
#define URL L"https://google.com/"

HINSTANCE g_hInst = nullptr;
static ICoreWebView2Controller *g_webviewController = nullptr;
static ICoreWebView2 *g_webView = nullptr;

///////////////////////////////////////////////////////////////////////////////////////////////

class MCoreWebView2HandlersImpl
    : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
    , public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
    , public ICoreWebView2WebMessageReceivedEventHandler
{
protected:
    LONG m_cRefs = 1; // reference counter
    HWND m_hWnd = nullptr;

public:
    MCoreWebView2HandlersImpl(HWND hWnd) : m_hWnd(hWnd) {  }

    // ICoreWebView2CreateCoreWebView2ControllerCompletedHandler interface
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override {
        if (FAILED(result) || !controller)
            return result;

        g_webviewController = controller;
        g_webviewController->AddRef();

        g_webviewController->get_CoreWebView2(&g_webView);
        if (!g_webView) return E_FAIL;

        // Configure settings
        ICoreWebView2Settings* settings = nullptr;
        g_webView->get_Settings(&settings);
        if (settings) {
            settings->put_AreDefaultContextMenusEnabled(TRUE);
            settings->put_AreDefaultScriptDialogsEnabled(TRUE);
            settings->put_IsBuiltInErrorPageEnabled(TRUE);
            settings->put_IsScriptEnabled(TRUE);
            settings->put_IsStatusBarEnabled(FALSE);
            settings->put_IsWebMessageEnabled(TRUE);
            settings->put_IsZoomControlEnabled(FALSE);
            settings->Release();
        }

        // Resize WebView
        PostMessage(m_hWnd, WM_SIZE, 0, 0);

#if 0
        // Navigate to URL
        g_webView->Navigate(URL);
#else
        // Navigate content
        auto content = L"<html><body><h1>HelloWebView2</h1>"
            L"<p>This is an HTML.</p>"
            L"<script>window.chrome.webview.postMessage('This is a message from JavaScript to native');"
            L"window.chrome.webview.addEventListener('message', event => { alert(event.data); });</script>"
            L"</body></html>";
        g_webView->NavigateToString(content);
#endif

#if 1
        EventRegistrationToken token;
        g_webView->add_WebMessageReceived(this, &token);
#endif

#if 1
        // Post message to JavaScript
        g_webView->PostWebMessageAsString(L"This is a message from native to JavaScript");
#endif

        return S_OK;
    }

    // ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler interface
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* env) override {
        if (FAILED(result) || !env) 
            return result;

        env->CreateCoreWebView2Controller(m_hWnd, this);
        return S_OK;
    }

    // ICoreWebView2WebMessageReceivedEventHandler interface
    HRESULT STDMETHODCALLTYPE Invoke(
        ICoreWebView2* sender,
        ICoreWebView2WebMessageReceivedEventArgs* args) override
    {
        // Get the message as a string
        LPWSTR message = nullptr;
        HRESULT hr = args->TryGetWebMessageAsString(&message);
        if (SUCCEEDED(hr) && message) {
            std::wstring msg(message);
            CoTaskMemFree(message);

            msg += L"\n";
            OutputDebugStringW(msg.c_str()); // Debug output
        }

        return S_OK;
    }

    // IUnknown interface
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_cRefs; }
    ULONG STDMETHODCALLTYPE Release() override {
        if (--m_cRefs == 0) {
            delete this;
            return 0;
        }
        return m_cRefs;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (riid == IID_IUnknown ||
            riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler ||
            riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler ||
            riid == IID_ICoreWebView2WebMessageReceivedEventHandler)
        {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////

#define MY_WM_CREATE_WEBVIEW (WM_USER + 100)

// The window procedure
LRESULT CALLBACK
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        PostMessage(hWnd, MY_WM_CREATE_WEBVIEW, 0, 0);
        return 0;
    case MY_WM_CREATE_WEBVIEW:
        {
            auto handler = new MCoreWebView2HandlersImpl(hWnd);
            CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, handler);
            handler->Release();
        }
        return 0;
    case WM_DESTROY:
        if (g_webviewController) {
            g_webviewController->Release();
            g_webviewController = nullptr;
        }
        if (g_webView) {
            g_webView->Release();
            g_webView = nullptr;
        }
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (g_webviewController) {
            RECT bounds;
            GetClientRect(hWnd, &bounds);
            g_webviewController->put_Bounds(bounds);
        }
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
    if (!RegisterClassEx(&wcex)) {
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
    if (!hWnd) {
        MessageBox(NULL, _T("CreateWindow failed"), TITLE, MB_ICONERROR);
        return 1;
    }

    // Show the main window
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // The message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

#if (_WIN32_WINNT >= 0x0500) && !defined(NDEBUG)
    // Detect handle leaks (Debug only)
    TCHAR szText[MAX_PATH];
    wnsprintf(szText, _countof(szText), TEXT("GDI Objects: %ld, User Objects: %ld\n"),
              GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS),
              GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS));
    OutputDebugString(szText);
#endif

#if defined(_MSC_VER) && !defined(NDEBUG)
    // Detect memory leaks (MSVC Debug only)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    return (INT)msg.wParam;
}
