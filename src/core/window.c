#include "window.h"

#include <stdio.h>

#ifdef WINDOWS_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

void append_text(HWND hEdit, const char *text)
{
  int len = GetWindowTextLengthA(hEdit);

  SendMessageA(hEdit, EM_SETSEL, len, len); // move cursor to the end
  SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)text);
}

// TODO : make the buffer sizes coherents
void handle_input_send(AppData *data)
{
  char input[4096]; // raw user input

  GetWindowTextA(data->hEdit, input, sizeof(input));

  if (strlen(input) == 0)
    return;

  char line[4096];
  snprintf(line, sizeof(line), "%s: %s\r\n", data->userData.username, input);

  append_text(data->hStatic, line);

  SetWindowTextA(data->hEdit, ""); // reset edit

  send(data->socket, input, strlen(input), 0);
}

LRESULT CALLBACK UserInputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Window user_input(char *title)
{
  HINSTANCE hInstance = GetModuleHandleA(0);

  Window window = {
      .title = title};

  WNDCLASS wc = {
      .lpfnWndProc = UserInputProc,
      .lpszClassName = "user_input",
      .hInstance = hInstance};

  if (!RegisterClassA(&wc))
  {
    printf("error registering user_input class");
  }

  int dwStyle = WS_VISIBLE | WS_DLGFRAME;
  int width = 300;
  int height = 100;
  int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
  int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
  HWND hwnd = CreateWindowExA(0, wc.lpszClassName, title, dwStyle, x, y, width, height, NULL, NULL, hInstance, NULL);

  if (!hwnd)
  {
    printf("error the window is not created properly");
  }

  window.hwnd = hwnd;

  ShowWindow(hwnd, SW_SHOW);

  return window;
}

LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_CHAR && wParam == '\r')
  {
    if (GetKeyState(VK_SHIFT) & 0x8000)
    {
      // allow normal newline
      return CallWindowProc(
          (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA),
          hwnd, msg, wParam, lParam);
    }
    else
    {
      // send message instead of newline
      HWND parent = GetParent(hwnd);
      AppData *data = (AppData *)GetWindowLongPtr(parent, GWLP_USERDATA);

      handle_input_send(data);
      return 0; // prevent beep and prevent newline
    }
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

    // edit procedure
    WNDPROC oldEditProc = (WNDPROC)SetWindowLongPtrA(
        data->hEdit,
        GWLP_WNDPROC,
        (LONG_PTR)EditProc);
    SetWindowLongPtrA(data->hEdit, GWLP_USERDATA, (LONG_PTR)oldEditProc);

    break;
  }
  case WM_APP + 1:
  {
    AppData *data = (AppData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    char *received = (char *)lParam;

    char line[4096];
    snprintf(line, sizeof(line), "%s\r\n", received);

    append_text(data->hStatic, line);

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

  int dwStyle = WS_OVERLAPPEDWINDOW | WS_TABSTOP;

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

HWND create_edit(HWND hwnd)
{
  HINSTANCE hInstance = GetModuleHandleA(0);
  RECT rect;
  GetClientRect(hwnd, &rect);
  int windowWidth = rect.right - rect.left;
  int windowHeight = rect.bottom - rect.top;
  int dwStyle = WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL;

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
  int dwStyle = WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY;

  int margin = 10;
  int width = windowWidth - margin * 2;
  int x = margin;
  int y = margin;
  int height = windowHeight - (40 + margin);
  HWND hwnd_text_area = CreateWindowExA(0, "edit", "", dwStyle, x, y, width, height, hwnd, NULL, hInstance, NULL);
  return hwnd_text_area;
}

#endif /* WINDOW_IMPLEMENTATION */