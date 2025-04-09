#pragma warning (disable : 4996) // 'gethostbyname': Use getaddrinfo() or GetAddrInfoW() instead or define _WINSOCK_DEPRECATED_NO_WARNINGS to disable deprecated API warnings (+2 others)

// Required since Windows.h probably defines some of the functions present within Winsock2.h otherwise.
// If you remove it, it won't compile, and I do not care enough to dive deeper (at least right now).
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <processthreadsapi.h>
#include <stdio.h>

#define DEVICE "\\\\.\\GLOBALROOT\\Device\\KernelServer"
#define LISTENING_PORT 4444

typedef struct _INPUT_DATA
{
	unsigned long IOCTL;
	unsigned long InputLength;
	unsigned char InputBuffer[4096];
} INPUT_DATA, * PINPUT_DATA;

DWORD WINAPI SendIOCTL(LPVOID lpParameter)
{
	INPUT_DATA InputData = { 0 };

	HANDLE hDevice = INVALID_HANDLE_VALUE;
	unsigned char pOutputBuffer[0x1000] = { 0 };
	unsigned long BytesReturned = 0;

	BytesReturned = recv((SOCKET)lpParameter, (char*)&InputData, sizeof(InputData), 0);
	if (BytesReturned == SOCKET_ERROR)
	{
		printf("[-] Failed to receive data.\n");

		closesocket((SOCKET)lpParameter);
		return 1;
	}

	hDevice = CreateFileA(DEVICE, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("[-] Could not interact with device.\n");

		closesocket((SOCKET)lpParameter);
		return 1;
	}

	DeviceIoControl(hDevice, InputData.IOCTL, InputData.InputBuffer, InputData.InputLength, 0, 0, &BytesReturned, 0);
	printf("[+] Sent IOCTL.\n");

	CloseHandle(hDevice);
	closesocket((SOCKET)lpParameter);

	return 0;
}

int main(int argc, char** argv)
{
	WSADATA WinsockData = { 0 };

	struct hostent* pHost = 0;
	char* pIP = 0;

	struct sockaddr_in SocketAddress = { 0 };

	SOCKET Socket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	char Initialized = 0;

	if (WSAStartup(MAKEWORD(2, 2), &WinsockData))
	{
		return 1;
	}

	printf("[!] Please ensure that the kernel server driver is loaded!\n");

	while (1)
	{
		pHost = gethostbyname("");
		if (!pHost)
		{
			WSACleanup();
			return 1;
		}

		pIP = inet_ntoa(**(struct in_addr**)pHost->h_addr_list);
		if (!pIP)
		{
			WSACleanup();
			return 1;
		}

		Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (!Socket)
		{
			WSACleanup();
			return 1;
		}

		SocketAddress.sin_family = AF_INET;
		SocketAddress.sin_addr.s_addr = inet_addr(pIP);
		SocketAddress.sin_port = htons(LISTENING_PORT);
		if (bind(Socket, (const struct sockaddr*)&SocketAddress, sizeof(SocketAddress)))
		{
			WSACleanup();
			return 1;
		}

		if (listen(Socket, 0x7FFFFFFF))
		{
			WSACleanup();
			return 1;
		}

		if (!Initialized)
		{
			printf("[+] Listening on %s:%d\n", pIP, LISTENING_PORT);
			Initialized = 1;
		}

		ClientSocket = accept(Socket, 0, 0);
		if (!ClientSocket)
		{
			WSACleanup();
			return 1;
		}

		CreateThread(0, 0, SendIOCTL, (void*)ClientSocket, 0, 0);

		closesocket(Socket);
	}

	return 0;
}