#pragma once

#include <stdio.h>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

typedef struct {
  WSADATA wsaData;
} Network;

DWORD WINAPI NetworkThread(LPVOID param);

#endif

#define DEFAULT_PORT "5000"