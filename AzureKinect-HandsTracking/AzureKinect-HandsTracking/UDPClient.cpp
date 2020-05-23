#include "UDPClient.hpp"
#include <iostream>
#include <winsock.h>
#pragma comment(lib,"WS2_32")

UDPClient::UDPClient() {

}

UDPClient::~UDPClient(){
	if (WSACleanup() == SOCKET_ERROR) {
		std::cout << "CleanUp failed" << WSAGetLastError() << std::endl;
		exit(1);
}
	std::cout << "CleanUp success" << std::endl;
}

void UDPClient::start_client(const char* ipAddr, int port) {
	if (WSAStartup(MAKEWORD(2, 2), &WinSockData) != 0) {
		std::cout << "WSAStartup failed" << std::endl;
		exit(1);
	}
	std::cout << "WSAStartup Success" << std::endl;

	UDPClientData.sin_family = AF_INET;
	UDPClientData.sin_addr.s_addr = inet_addr(ipAddr);
	UDPClientData.sin_port = htons(port);

	UDPSocketClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (UDPSocketClient == INVALID_SOCKET) {
		std::cout << "UDP socket creation failed " << WSAGetLastError() << std::endl;
		exit(1);
	}
	std::cout << "UDP Socket creation success" << std::endl;

}

void UDPClient::send_data(const char* json_send) {
	sendto(UDPSocketClient, json_send, strlen(json_send), MSG_DONTROUTE, (SOCKADDR*)&UDPClientData, sizeof(UDPClientData));
}