#include <iostream>
#include <winsock.h>
#pragma comment(lib,"WS2_32")

#pragma once
class UDPClient
{
private:
	WSADATA WinSockData;
	SOCKET UDPSocketClient;
	struct sockaddr_in UDPClientData;

public:
	UDPClient();
	~UDPClient();
	void start_client(const char* ipAddr, int port);
	void send_data(const char* json_send);
};