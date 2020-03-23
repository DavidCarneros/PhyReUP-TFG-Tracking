#include <iostream>
#include <winsock.h>
#include "definitions.h"

class UDPClient
{
private:
	WSADATA WinSockData;
	SOCKET UDPSocketClient;
	struct sockaddr_in UDPServer;

public:
	UDPClient();
	~UDPClient();
	void send_hands_data(hands_data_t hands_data);
	void start_client(const char* ipAddr, int port);
	char* storePrintf(const char* fmt, ...);
};

UDPClient::UDPClient() {
	// Initialization of winsock


}

UDPClient::~UDPClient() {

	if (WSACleanup() == SOCKET_ERROR) {
		std::cout << "CleanUp failed " << WSAGetLastError() << std::endl;
		exit(1);
	}
	std::cout << "Cleanup success" << std::endl;

}

void UDPClient::start_client(const char* ipAddr, int port) {
	if (WSAStartup(MAKEWORD(2, 2), &WinSockData) != 0) {
		std::cout << "WSAStartup failed " << std::endl;
		exit(1);
	}
	std::cout << "WSAStartup Success" << std::endl;

	// Fill UDP SERVER structure

	UDPServer.sin_family = AF_INET;
	UDPServer.sin_addr.s_addr = inet_addr(ipAddr);
	UDPServer.sin_port = htons(port);

	// Socket creation
	UDPSocketClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (UDPSocketClient == INVALID_SOCKET) {
		std::cout << "UDP socket creation failed " << WSAGetLastError() << std::endl;
		exit(1);
	}
	std::cout << "UDP Socket creation success " << std::endl;
}


void UDPClient::send_hands_data(hands_data_t hands_data) {
	const char* json_struct = "{\"r_p\":{\"x\":%f,\"y\":%f,\"z\":%f},\"l_p\":{\"x\":%f,\"y\":%f,\"z\":%f}}";
	char* json_send = storePrintf(json_struct, hands_data.hand_right_position.v[0], hands_data.hand_right_position.v[1], hands_data.hand_right_position.v[2],
		hands_data.hand_left_position.v[0], hands_data.hand_left_position.v[1], hands_data.hand_left_position.v[2]);

	sendto(UDPSocketClient, json_send, strlen(json_send), MSG_DONTROUTE, (SOCKADDR*)&UDPServer, sizeof(UDPServer));

}

char* UDPClient::storePrintf(const char* fmt, ...) {
	va_list arg;
	va_start(arg, fmt);
	size_t sz = snprintf(NULL, 0, fmt, arg);
	char* buf = (char*)malloc(sz + 1);
#pragma warning(disable : 4996)
	vsprintf(buf, fmt, arg);
	va_end(arg);
	return buf;
}