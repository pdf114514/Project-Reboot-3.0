#include <iostream>
#include <winsock2.h>
#include <thread>
#include <regex>

#include "globals.h"
#include "gui.h"
#include "KismetSystemLibrary.h"

#pragma comment(lib, "ws2_32.lib")

std::string ToLower(std::string str) {
    std::string result = "";
    std::transform(str.begin(), str.end(), std::back_inserter(result), ::tolower);
    return result;
}

#define Pattern(x) std::regex P##x(ToLower(#x) + ";" + ".*?");
#define PatternA(x, y) std::regex P##x(ToLower(#x) + ";" + #y + ".*?");

// std::regex PSetPort("setport;(\\d+).*?");
PatternA(SetPort, (\\d+))

Pattern(Restart)
Pattern(StartBus)

PatternA(InfiniteAmmo, (true|false))
PatternA(InfiniteMaterials, (true|false))

Pattern(StartSafeZone)
Pattern(StopSafeZone)
Pattern(SkipSafeZone)
Pattern(StartShrinkSafeZone)
Pattern(SkipShrinkSafeZone)

class CommunicateServer {
    public:
        CommunicateServer();
        ~CommunicateServer();
        bool Start();
        bool Stop();
    private:
        void HandleConnection();
        SOCKET ServerSocket;
        bool IsRunning;
};

CommunicateServer::CommunicateServer() : ServerSocket(INVALID_SOCKET), IsRunning(false) {}

CommunicateServer::~CommunicateServer() { Stop(); }

bool start_with(const std::string& s, const std::string& prefix) {
    if (s.size() < prefix.size()) return false;
    return std::equal(std::begin(prefix), std::end(prefix), std::begin(s));
}

bool CommunicateServer::Start() {
    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_HOPOPTS);
    if (ServerSocket == INVALID_SOCKET) {
        std::cout << "Failed to create socket" << std::endl;
        return false;
    }

    u_long mode = 1;
    if (ioctlsocket(ServerSocket, FIONBIO, &mode) == SOCKET_ERROR) {
        std::cout << "Failed to set socket to non-blocking" << std::endl;
        return false;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // INADDR_ANY
    serverAddress.sin_port = htons(12345);
    if (bind(ServerSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cout << "Failed to bind socket" << std::endl;
        return false;
    }

    if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Failed to listen on socket" << std::endl;
        closesocket(ServerSocket);
        return false;
    }

    IsRunning = true;
    std::cout << "Listening on port " << 12345 << std::endl;

    std::thread t(&CommunicateServer::HandleConnection, this);
    t.detach();

    return true;
}

bool CommunicateServer::Stop() {
    IsRunning = false;
    if (ServerSocket != INVALID_SOCKET) {
        closesocket(ServerSocket);
        ServerSocket = INVALID_SOCKET;
    }
    return true;
}

void CommunicateServer::HandleConnection() {
    while (IsRunning) {
        SOCKET clientSocket = accept(ServerSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            // this does spam
            // std::cout << "Accept failed " << WSAGetLastError() << std::endl;
            continue;
        }

        u_long mode = 1;
        if (ioctlsocket(clientSocket, FIONBIO, &mode) == SOCKET_ERROR) {
            std::cout << "Failed to set socket to non-blocking" << std::endl;
            continue;
        }

        const int bufferSize = 1024;
        char buffer[bufferSize];
        std::string message;
        std::smatch matches;

        while (true) {
            int bytesRead = recv(clientSocket, buffer, bufferSize, 0);
            if (bytesRead == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) continue;
                std::cout << "Recv failed " << error << std::endl;
                break;
            } else if (bytesRead == 0) {
                std::cout << "Client disconnected." << std::endl;
                break;
            }

            message.clear();
            message = std::string(buffer, bytesRead);
            std::cout << "Received " << bytesRead << " bytes." << std::endl;
            std::cout << message << std::endl;
            // MessageBoxA(nullptr, message.c_str(), "Message", MB_OK);

            if (std::regex_match(message, matches, PSetPort)) {
                std::string port = matches[1].str();
                std::cout << "Port: " << port << std::endl;
                Globals::Port = std::stoi(port);
            } else if (std::regex_match(message, PRestart)) {
                Restart();
            } else if (std::regex_match(message, PStartBus)) {
                if (bStartedBus) continue;
                bStartedBus = true;
                auto gameMode = (AFortGameMode*)GetWorld()->GetGameMode();
                auto gameState = gameMode->GetGameState();
                static auto warmupCountdownEndTimeOffset = gameState->GetOffset("WarmupCountdownEndTime");
                float timeSecods = gameState->GetServerWorldTimeSeconds();
                float duration = 20;
                float earlyDuration = duration;
                static auto warmupCountdownStartTimeOffset = gameState->GetOffset("WarmupCountdownStartTime");
                static auto warmupCountdownDurationOffset = gameMode->GetOffset("WarmupCountdownDuration");
                static auto warmupEarlyCountdownDurationOffset = gameMode->GetOffset("WarmupEarlyCountdownDuration");
                gameState->Get<float>(warmupCountdownEndTimeOffset) = timeSecods + duration;
                gameMode->Get<float>(warmupCountdownDurationOffset) = duration;
                gameMode->Get<float>(warmupEarlyCountdownDurationOffset) = earlyDuration;
            } else if (std::regex_match(message, matches, PInfiniteAmmo)) {
                std::string value = matches[1].str();
                std::cout << "InfiniteAmmo: " << value << std::endl;
                Globals::bInfiniteAmmo = value == "true";
            } else if (std::regex_match(message, matches, PInfiniteMaterials)) {
                std::string value = matches[1].str();
                std::cout << "InfiniteMaterials: " << value << std::endl;
                Globals::bInfiniteMaterials = value == "true";
            } else if (std::regex_match(message, PStartSafeZone)) {
                UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"startsafezone", nullptr);
            } else if (std::regex_match(message, PStopSafeZone)) {
                UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"stopsafezone", nullptr);
            } else if (std::regex_match(message, PSkipSafeZone)) {
                UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"skipsafezone", nullptr);
            } else if (std::regex_match(message, PStartShrinkSafeZone)) {
                UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"startshrinksafezone", nullptr);
            } else if (std::regex_match(message, PSkipShrinkSafeZone)) {
                auto gameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
                auto safeZoneIndicator = gameMode->GetSafeZoneIndicator();
                if (safeZoneIndicator) safeZoneIndicator->SkipShrinkSafeZone();
            } else {
                MessageBoxA(nullptr, message.c_str(), "Message", MB_OK);
            }

        }
    }
}