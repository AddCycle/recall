#include "window.h"
#include "network.h"
#include <stdbool.h>

int main(int argc, char* argv[])
{
  char* username = "default_username";

  if (argc > 1)
  {
    username = argv[1];
  }

  Network network;

  // INIT WSA (networking)
  int iResult;
  iResult = WSAStartup(MAKEWORD(2, 2), &network.wsaData);

  if (iResult != 0)
  {
    printf("WSAStartup failed with code: %d\n", iResult);
    return 1;
  }

  // specifying the connection
  struct addrinfo *result = NULL, hints;

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_socktype = SOCK_STREAM;

  // resolve server address and port
  const char *server_ip = "127.0.0.1"; // default: locahost
  iResult = getaddrinfo(server_ip, DEFAULT_PORT, &hints, &result);
  if (iResult != 0)
  {
    printf("getaddrinfo failed: %d\n", iResult);
    WSACleanup();
    return 1;
  }

  // create socket
  SOCKET clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (clientSocket == INVALID_SOCKET)
  {
    printf("Error at socket(): %d\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }

  // connect to the server
  iResult = connect(clientSocket, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR)
  {
    closesocket(clientSocket);
    clientSocket = INVALID_SOCKET;
  }

  freeaddrinfo(result);
  if (clientSocket == INVALID_SOCKET)
  {
    printf("Unable to connect to server!\n");
    WSACleanup();
    return 1;
  }

  // immediately send the username to the server
  send(clientSocket, username, strlen(username), 0);

  Window window = create_window(800, 600, "Recall");

  AppData *data = (AppData *)GetWindowLongPtr(window.hwnd, GWLP_USERDATA);
  data->socket = clientSocket;
  data->running = true;
  data->hThread = CreateThread(NULL, 0, NetworkThread, data, 0, NULL);
  data->userData.username = username;

  // main message loop
  MSG msg;
  while (GetMessageA(&msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  WSACleanup();
  return 0;
}