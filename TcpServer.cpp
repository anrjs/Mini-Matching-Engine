/**
 * TCP Echo Server (Windows / WinSock2)
 * 포트 8080에서 클라이언트 접속을 받아 수신 메시지를 [Echo]: 접두사와 함께 되돌려줌.
 */

 #ifdef _WIN32
 #ifndef _WIN32_WINNT
 #define _WIN32_WINNT 0x0600
 #endif
 #endif
 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <windows.h> // [추가] 콘솔 설정용 헤더

#include "Common/Protocol.h"
#include "OrderBook.h"
#include "Order.h"
#include "Types.h"

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 8080
#define RECV_BUF_SIZE 4096
 
// -----------------------------------------------------------------------------
// 클라이언트 전담 스레드 함수
// -----------------------------------------------------------------------------
void handleClient(SOCKET clientSocket, OrderBook& orderBook, std::mutex& obMutex) {
    char recvBuf[RECV_BUF_SIZE];

    std::cout << ">>> 클라이언트 접속 성공 (소켓: " << clientSocket << ")" << std::endl;

    while (true) {
        // 정확히 OrderPacket 크기만큼 수신을 시도
        int bytesNeeded = static_cast<int>(sizeof(OrderPacket));
        int totalReceived = 0;

        while (totalReceived < bytesNeeded) {
            int bytesReceived = recv(
                clientSocket,
                recvBuf + totalReceived,
                bytesNeeded - totalReceived,
                0
            );

            if (bytesReceived == SOCKET_ERROR) {
                std::cerr << "recv() 실패 (소켓: " << clientSocket
                          << "): " << WSAGetLastError() << std::endl;
                goto client_cleanup;
            }
            if (bytesReceived == 0) {
                std::cout << "클라이언트 연결 종료 (소켓: " << clientSocket << ")." << std::endl;
                goto client_cleanup;
            }

            totalReceived += bytesReceived;
        }

        // 수신 버퍼를 OrderPacket으로 해석
        OrderPacket* packet = reinterpret_cast<OrderPacket*>(recvBuf);

        // 패킷 타입별 처리
        if (packet->type == static_cast<int>(PacketType::ORDER)) {
            // side 변환 (1: 매수, 2: 매도)
            Side sideEnum;
            if (packet->side == 1) {
                sideEnum = Side::BUY;
            } else if (packet->side == 2) {
                sideEnum = Side::SELL;
            } else {
                std::cerr << "잘못된 side 값 수신 (소켓: " << clientSocket
                          << "): " << packet->side << std::endl;
                continue;
            }

            OrderId orderId = static_cast<OrderId>(packet->orderId);
            Price price = static_cast<Price>(packet->price);
            Quantity quantity = static_cast<Quantity>(packet->quantity);

            {
                // ----- 스레드 안전하게 OrderBook 접근 -----
                std::lock_guard<std::mutex> lock(obMutex);
                Order order(orderId, OrderType::LIMIT, sideEnum, price, quantity);
                orderBook.addOrder(order);
                orderBook.matchOrder();
            }

            std::cout << "[엔진 투입] (소켓: " << clientSocket << ") "
                      << "주문ID: " << orderId
                      << ", 매수/매도: " << (sideEnum == Side::BUY ? "매수" : "매도")
                      << ", 가격: " << price
                      << ", 수량: " << quantity
                      << std::endl;
        } else if (packet->type == static_cast<int>(PacketType::CANCEL)) {
            // 주문 취소 처리
            OrderId cancelOrderId = static_cast<OrderId>(packet->orderId);
            {
                std::lock_guard<std::mutex> lock(obMutex);
                orderBook.cancelOrder(cancelOrderId);
            }

            std::cout << "[엔진 투입] (소켓: " << clientSocket << ") "
                      << "주문 취소 요청 - 주문ID: " << cancelOrderId
                      << std::endl;
        } else {
            std::cerr << "알 수 없는 패킷 타입 수신 (소켓: " << clientSocket
                      << "): " << packet->type << std::endl;
            continue;
        }

        // ----- 응답 전송 -----
        const std::string response = "주문 접수 및 처리 완료";
        int totalToSend = static_cast<int>(response.size());
        int sent = 0;

        while (sent < totalToSend) {
            int n = send(clientSocket, response.c_str() + sent, totalToSend - sent, 0);
            if (n == SOCKET_ERROR) {
                std::cerr << "send() 실패 (소켓: " << clientSocket
                          << "): " << WSAGetLastError() << std::endl;
                goto client_cleanup;
            }
            sent += n;
        }
    }

client_cleanup:
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        std::cout << "클라이언트 소켓 정리 완료 (소켓: " << clientSocket << ")." << std::endl;
    }
}

int main() {
    // [한글 깨짐 해결] 콘솔 입출력 인코딩을 UTF-8(CP_UTF8 = 65001)로 설정
    SetConsoleOutputCP(65001);
    // (선택사항) 입력 인코딩도 맞추고 싶다면 아래 주석 해제
    // SetConsoleCP(65001); 

    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    sockaddr_in serverAddr = {};
    sockaddr_in clientAddr = {};
    int clientAddrLen = sizeof(clientAddr);

    // ----- 매칭 엔진 및 동기화 객체 초기화 -----
    OrderBook orderBook;
    std::mutex obMutex;

    // ----- 1. WSA 초기화 -----
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup 실패: " << result << std::endl;
        return 1;
    }

    // ----- 2. 리스닝 소켓 생성 (IPv4, TCP) -----
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket() 실패: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // ----- 3. 바인딩 (포트 8080, INADDR_ANY) -----
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<u_short>(DEFAULT_PORT));

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind() 실패: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // ----- 4. 리스닝 (대기열 SOMAXCONN) -----
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen() 실패: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << ">>> OrderBook Server 대기 중... (포트 " << DEFAULT_PORT << ")" << std::endl;

    // ----- 5. 무한 accept 루프 -----
    while (true) {
        SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept() 실패: " << WSAGetLastError() << std::endl;
            continue; // 서버는 계속 대기
        }

        std::cout << ">>> 새로운 클라이언트 접속 (소켓: " << clientSocket << ")" << std::endl;

        // 클라이언트 전담 스레드 생성 및 분리
        std::thread(handleClient, clientSocket, std::ref(orderBook), std::ref(obMutex)).detach();
    }

    // 이 코드는 실제로 도달하지 않지만, 형식상 정리 코드 작성
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
    }
    WSACleanup();
    std::cout << "서버 종료." << std::endl;
    return 0;
}