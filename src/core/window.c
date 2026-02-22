#include "window.h"

#include <stdio.h>

#ifdef WINDOWS_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

void handle_input_send(AppData *data)
{
  char input[255];    // edit
  char oldText[4096]; // static ->
  char final[8192];   // -> static

  GetWindowTextA(data->hEdit, input, sizeof(input));

  if (strlen(input) == 0)
    return;

  GetWindowTextA(data->hStatic, oldText, sizeof(oldText));

  snprintf(final, sizeof(final), "%s%s: %s\n", oldText, data->userData.username, input);

  SetWindowTextA(data->hStatic, final);

  SetWindowTextA(data->hEdit, ""); // reset edit

  send(data->socket, input, strlen(input), 0);
}

LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_KEYDOWN && wParam == VK_RETURN)
  {
    HWND parent = GetParent(hwnd);

    AppData *data = (AppData *)GetWindowLongPtr(parent, GWLP_USERDATA);
    handle_input_send(data);

    return 0; // to stop the windows "ding" sfx
  }

  return CallWindowProc(
      (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA),
      hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  HINSTANCE hInstance = GetModuleHandleA(0);

  RECT rect;
  GetClientRect(hwnd, &rect);

  switch (uMsg)
  {
  case WM_CREATE:
  {
    CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
    AppData *appdata = (AppData *)cs->lpCreateParams;

    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)appdata);

    AppData *data = (AppData *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    data->hwnd = hwnd;
    data->hEdit = create_edit(hwnd);
    data->hButton = create_button(hwnd);
    data->hStatic = create_static(hwnd);

    WNDPROC oldProc = (WNDPROC)SetWindowLongPtr(
        data->hEdit,
        GWLP_WNDPROC,
        (LONG_PTR)EditProc);

    SetWindowLongPtr(data->hEdit, GWLP_USERDATA, (LONG_PTR)oldProc);

    break;
  }
  case WM_APP + 1:
  {
    AppData *data = (AppData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    char *received = (char *)lParam;

    char oldText[4096];
    char final[8192];

    GetWindowTextA(data->hStatic, oldText, sizeof(oldText));

    snprintf(final, sizeof(final),
             "%s%s\r\n",
             oldText,
             received);

    SetWindowTextA(data->hStatic, final);

    free(received);
    break;
  }
  case WM_KEYDOWN:
  {
    break;
  }
  case WM_COMMAND:
  {
    if (LOWORD(wParam) == 1)
    {
      AppData *data = (AppData *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

      handle_input_send(data);
    }
    break;
  }
  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    HBRUSH hBrush = (HBRUSH)(COLOR_WINDOW + 1);

    FillRect(hdc, &ps.rcPaint, hBrush);

    EndPaint(hwnd, &ps);
    break;
  }
  case WM_CLOSE:
  {
    printf("User requested closing the window\n");
    DestroyWindow(hwnd);
    break;
  }
  case WM_DESTROY:
  {
    AppData *data = (AppData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    data->running = false;

    if (data->socket != INVALID_SOCKET)
    {
      shutdown(data->socket, SD_BOTH);
      closesocket(data->socket);
    }

    WaitForSingleObject(data->hThread, INFINITE);
    CloseHandle(data->hThread);

    free(data);
    printf("window destroyed posting quit message\n");
    PostQuitMessage(0);
    break;
  }
  case WM_QUIT:
  {
    printf("GetMessageA should return 0 exiting the loop and program\n");
    break;
  }
  default:
    break;
  }
  return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

Window create_window(int width, int height, const char *title)
{
  Window window = {.title = title};

  HINSTANCE hInstance = GetModuleHandleA(0);

  WNDCLASS wc = {
      .lpszClassName = "main_window",
      .hInstance = hInstance,
      .lpfnWndProc = WindowProc};

  if (!RegisterClassA(&wc))
    return window;

  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  int x = (screenWidth - width) / 2;
  int y = (screenHeight - height) / 2;

  int dwStyle = WS_OVERLAPPEDWINDOW;

  AppData *data = malloc(sizeof(AppData));

  HWND hwnd = CreateWindowExA(0, wc.lpszClassName, title, dwStyle, x, y, width, height, NULL, NULL, hInstance, data);
  if (!hwnd)
  {
    return window;
  }

  window.hwnd = hwnd;

  RECT rect;
  GetClientRect(window.hwnd, &rect);
  window.width = rect.right - rect.left;
  window.height = rect.bottom - rect.top;
  printf("client_width, client_height:%d,%d\n", window.width, window.height);

  ShowWindow(window.hwnd, SW_SHOW);

  return window;
}

void create_menu(Window window)
{
  HMENU hMenu = CreateMenu();
  int id = 0;
  AppendMenuA(hMenu, MF_STRING, id++, "File");
  AppendMenuA(hMenu, MF_STRING, id++, "Info");

  SetMenu(window.hwnd, hMenu);
}

HWND create_edit(HWND hwnd)
{
  HINSTANCE hInstance = GetModuleHandleA(0);
  RECT rect;
  GetClientRect(hwnd, &rect);
  int windowWidth = rect.right - rect.left;
  int windowHeight = rect.bottom - rect.top;
  int dwStyle = WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP;

  int margin = 10;
  int height = 20;
  int y = windowHeight - height - margin;
  int width = 300;
  int x = margin;
  HWND hwnd_edit = CreateWindowExA(0, "edit", "Type a message here...", dwStyle, x, y, width, height, hwnd, NULL, hInstance, NULL);
  return hwnd_edit;
}

HWND create_button(HWND hwnd)
{
  HINSTANCE hInstance = GetModuleHandleA(0);
  RECT rect;
  GetClientRect(hwnd, &rect);
  int windowWidth = rect.right - rect.left;
  int windowHeight = rect.bottom - rect.top;
  int dwStyle = WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | BS_DEFPUSHBUTTON;

  int margin = 10;
  int height = 20;
  int y = windowHeight - height - margin;

  int width = 100;
  int x = windowWidth - width - margin;
  HWND hwnd_button = CreateWindowExA(0, "button", "Send", dwStyle, x, y, width, height, hwnd, (HMENU)1, hInstance, NULL);
  return hwnd_button;
}

HWND create_static(HWND hwnd)
{
  HINSTANCE hInstance = GetModuleHandleA(0);
  RECT rect;
  GetClientRect(hwnd, &rect);
  int windowWidth = rect.right - rect.left;
  int windowHeight = rect.bottom - rect.top;
  int dwStyle = WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP;

  int margin = 10;
  int width = windowWidth - margin * 2;
  int x = margin;
  int y = margin;
  int height = windowHeight - (40 + margin);
  HWND hwnd_text_area = CreateWindowExA(0, "static", "", dwStyle, x, y, width, height, hwnd, NULL, hInstance, NULL);
  return hwnd_text_area;
}

#endif /* WINDOW_IMPLEMENTATION */