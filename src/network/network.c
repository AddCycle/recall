#include "network.h"
#include "window.h"

DWORD WINAPI NetworkThread(LPVOID param)
{
  AppData *data = (AppData *)param;
  char buffer[512];

  while (data->running)
  {
    int bytes = recv(data->socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
      break;

    buffer[bytes] = '\0';

    // send a message to main_window to append the message
    PostMessageA(data->hwnd, WM_APP + 1, 0, (LPARAM)_strdup(buffer));
  }

  return 0;
}
