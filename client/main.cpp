#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <thread>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

bool Initialize()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        cout << "WSAStartup failed: " << result << endl;
        return false;
    }
    return true;
}

void SendMsg(SOCKET s)
{
    cout << "Enter your chat name: ";
    string name;
    getline(cin, name);
    string message;

    while (true)
    {
        cout << "Enter message: ";
        getline(cin, message);
        message = name + ": " + message;

        int bytesent = send(s, message.c_str(), static_cast<int>(message.length()), 0);

        if (bytesent == SOCKET_ERROR)
        {
            cout << "Failed to send: " << WSAGetLastError() << endl;
            break;
        }

        if (message == name + ": quit")
        {
            cout << "Connection closed" << endl;
            break;
        }
    }

    // Close socket after the user decides to quit
    closesocket(s);
}

void ReceiveMsg(SOCKET s)
{
    char buffer[4096];
    int recvlenght;

    while (true)
    {
        recvlenght = recv(s, buffer, sizeof(buffer), 0);

        if (recvlenght <= 0)
        {
            cout << "Disconnected" << endl;
            break;
        }

        buffer[recvlenght] = '\0';  // Null-terminate the received string
        cout << "Received: " << buffer << endl;
    }

    // Close socket when disconnected
    closesocket(s);
}

int main()
{
    if (!Initialize())
    {
        cout << "Failed to initialize" << endl;
        return 1;
    }

    cout << "Client " << endl;

    SOCKET s = INVALID_SOCKET;
    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s == INVALID_SOCKET)
    {
        cout << "Failed to create socket: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    int port = 12345;
    string serveraddress = "127.0.0.1";

    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    if (inet_pton(AF_INET, serveraddress.c_str(), &serveraddr.sin_addr) <= 0)
    {
        cout << "Invalid address: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    if (connect(s, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR)
    {
        cout << "Failed to connect: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    cout << "Connected" << endl;

    thread senderthread(SendMsg, s);
    thread receiverthread(ReceiveMsg, s);

    // Wait for both threads to finish before cleaning up
    senderthread.join();
    receiverthread.join();

    WSACleanup();
    cout << "Cleaned up" << endl;

    return 0;
}
