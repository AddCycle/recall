#pragma once

#include <stdio.h>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

typedef struct {
  WSADATA wsaData;
} Network;

// TODO: look at structs padding (compiler specific)
typedef struct {
  char username[255];
  char msg[4096];
  int format;
} NetworkPacketMsg;

DWORD WINAPI NetworkThread(LPVOID param);
// TODO : implement encoder/decoder
// int encode(NetworkPacketMsg *msg, char* buffer);
// NetworkPacketMsg decode(char* buffer);

#endif

int InitNetwork(Network network);
void NetworkCleanup(Network network);

#define DEFAULT_PORT "5000"
#define NETWORK_EXIT(code, ...) printf(__VA_ARGS__); return code