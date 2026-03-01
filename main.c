#include "window.h"
#include "network.h"
#include <stdbool.h>

int main(int argc, char *argv[])
{
  char *username = "default_username";

  if (argc > 1)
  {
    username = argv[1];
  }

  Network network;

  int iResult;
  iResult = InitNetwork(network);

  if (iResult != 0)
  {
    NetworkCleanup(network);
    NETWORK_EXIT(1, "Network init failed with code: %d\n", iResult);
  }

  // specifying the connection
  struct addrinfo *result = NULL, hints;

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_socktype = SOCK_STREAM;

  // resolve server address and port
  char *server_ip = "127.0.0.1"; // default: locahost
  if (argc > 2)
  {
    server_ip = argv[2];
    printf("server ip: %s\n", argv[2]);
  }

  iResult = getaddrinfo(server_ip, DEFAULT_PORT, &hints, &result);
  if (iResult != 0)
  {
    NetworkCleanup(network);
    NETWORK_EXIT(1, "getaddrinfo failed with code: %d\n", iResult);
  }

  // create socket
  SOCKET clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (clientSocket == INVALID_SOCKET)
  {
    int code = WSAGetLastError();
    freeaddrinfo(result);
    NetworkCleanup(network);
    NETWORK_EXIT(1, "Error at socket(): %d\n", code);
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
    NetworkCleanup(network);
    NETWORK_EXIT(1, "Unable to connect to server!\n");
  }

  Window window = create_window(800, 600, "Recall");
  
  // TODO: poll username

  AppData *data = (AppData *)GetWindowLongPtr(window.hwnd, GWLP_USERDATA);
  data->socket = clientSocket;
  data->running = true;
  data->hThread = CreateThread(NULL, 0, NetworkThread, data, 0, NULL);
  data->userData.username = username;

  // immediately send the username to the server
  send(clientSocket, username, strlen(username), 0);

  // main message loop
  MSG msg;
  while (GetMessageA(&msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }

  NetworkCleanup(network);
  return 0;
}