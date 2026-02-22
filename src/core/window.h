#pragma once

#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define WINDOWS_IMPLEMENTATION

#include <windows.h>
#include <winsock2.h>

typedef struct Window
{
  int id;
  int width, height;
  const char* title;
  HWND hwnd;
} Window;

typedef struct UserData {
  char* username;
} UserData;

typedef struct {
  HWND hwnd, hEdit, hButton, hStatic;
  SOCKET socket;
  HANDLE hThread;
  bool running;
  UserData userData;
} AppData;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND create_edit(HWND hwnd);
HWND create_button(HWND hwnd);
HWND create_static(HWND hwnd);
void handle_input_send(AppData *data);

#endif

Window create_window(int width, int height, const char* title);
void create_menu(Window window);