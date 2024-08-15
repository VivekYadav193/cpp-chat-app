#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <tchar.h>
#include <thread>
#include <vector>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

mutex clientMutex;

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

void InteractWithClient(SOCKET clientSocket, vector<SOCKET>& clients)
{
    cout << "Client connected" << endl;
    char buffer[4096];

    while (true)
    {
        int bytesrecvd = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesrecvd <= 0)
        {
            cout << "Client disconnected" << endl;
            break;
        }

        if (bytesrecvd == SOCKET_ERROR)
        {
            cout << "Error in receiving" << endl;
            break;
        }

        string message(buffer, bytesrecvd);
        cout << "Received: " << message << endl;

        lock_guard<mutex> lock(clientMutex);
        for (auto client : clients)
        {
            if (client != clientSocket)
                send(client, message.c_str(), message.length(), 0);
        }
    }

    lock_guard<mutex> lock(clientMutex);
    auto it = find(clients.begin(), clients.end(), clientSocket);
    if (it != clients.end()) {
        clients.erase(it);
    }

    closesocket(clientSocket);
}

int main()
{
    if (!Initialize())
    {
        cout << "Failed to initialize" << endl;
        return 1;
    }

    cout << "Server created" << endl;

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        cout << "Failed to create socket" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(12345);

    if (InetPton(AF_INET, _T("0.0.0.0"), &serveraddr.sin_addr) != 1)
    {
        cout << "Failed to convert IP address" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR)
    {
        cout << "Failed to bind" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cout << "Failed to listen" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Listening for incoming connections" << endl;

    vector<SOCKET> clients;

    while (true)
    {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            cout << "Invalid client Socket" << endl;
            continue;
        }

        {
            lock_guard<mutex> lock(clientMutex);
            clients.push_back(clientSocket);
        }

        thread t1(InteractWithClient, clientSocket, std::ref(clients));
        t1.detach();
    }

    closesocket(listenSocket);
    WSACleanup();

    cout << "Cleaned up" << endl;
    return 0;
}
