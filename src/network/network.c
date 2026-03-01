#include "network.h"
#include "window.h"

// temp until Linux, MacOS implementations to ensure Network struct exists
#ifdef _WIN32

int InitNetwork(Network network)
{
  // INIT WSA (networking)
  int result;
  result = WSAStartup(MAKEWORD(2, 2), &network.wsaData);

  if (result != 0)
  {
    printf("WSAStartup failed with code: %d\n", result);
  }

  return result;
}

void NetworkCleanup(Network network)
{
  WSACleanup();
}

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

#endif // _WIN32