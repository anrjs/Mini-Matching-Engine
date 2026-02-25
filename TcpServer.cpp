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
#include <windows.h> // [추가] 콘솔 설정용 헤더

#include "Common/Protocol.h"
#include "OrderBook.h"
#include "Order.h"
#include "Types.h"

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 8080
#define RECV_BUF_SIZE 4096
 
int main() {
    // [한글 깨짐 해결] 콘솔 입출력 인코딩을 UTF-8(CP_UTF8 = 65001)로 설정
    SetConsoleOutputCP(65001);
    // (선택사항) 입력 인코딩도 맞추고 싶다면 아래 주석 해제
    // SetConsoleCP(65001); 

    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    sockaddr_in serverAddr = {};
    sockaddr_in clientAddr = {};
    int clientAddrLen = sizeof(clientAddr);

    // ----- 매칭 엔진 초기화 -----
    OrderBook orderBook;

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

    // ----- 5. 클라이언트 접속 수락 -----
    clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "accept() 실패: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << ">>> 클라이언트 접속 성공!" << std::endl;

    // 리스닝 소켓은 더 이상 사용하지 않음
    closesocket(listenSocket);
    listenSocket = INVALID_SOCKET;

    // ----- 6. 바이너리 OrderPacket 수신 루프 -----
    char recvBuf[RECV_BUF_SIZE];

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
                std::cerr << "recv() 실패: " << WSAGetLastError() << std::endl;
                goto cleanup;
            }
            if (bytesReceived == 0) {
                std::cout << "클라이언트 연결 종료." << std::endl;
                goto cleanup;
            }

            totalReceived += bytesReceived;
        }

        // 수신 버퍼를 OrderPacket으로 해석
        OrderPacket* packet = reinterpret_cast<OrderPacket*>(recvBuf);

        // 패킷 타입 검증
        if (packet->type != static_cast<int>(PacketType::ORDER)) {
            std::cerr << "알 수 없는 패킷 타입 수신: " << packet->type << std::endl;
            continue;
        }

        // side 변환 (1: 매수, 2: 매도)
        Side sideEnum;
        if (packet->side == 1) {
            sideEnum = Side::BUY;
        } else if (packet->side == 2) {
            sideEnum = Side::SELL;
        } else {
            std::cerr << "잘못된 side 값 수신: " << packet->side << std::endl;
            continue;
        }

        OrderId orderId = static_cast<OrderId>(packet->orderId);
        Price price = static_cast<Price>(packet->price);
        Quantity quantity = static_cast<Quantity>(packet->quantity);

        // Order 객체 생성 후 엔진에 투입
        Order order(orderId, OrderType::LIMIT, sideEnum, price, quantity);
        orderBook.addOrder(order);
        orderBook.matchOrder();

        std::cout << "[엔진 투입] "
                  << "주문ID: " << orderId
                  << ", 매수/매도: " << (sideEnum == Side::BUY ? "매수" : "매도")
                  << ", 가격: " << price
                  << ", 수량: " << quantity
                  << std::endl;

        // ----- 응답 전송 -----
        const std::string response = "주문 접수 및 처리 완료";
        int totalToSend = static_cast<int>(response.size());
        int sent = 0;

        while (sent < totalToSend) {
            int n = send(clientSocket, response.c_str() + sent, totalToSend - sent, 0);
            if (n == SOCKET_ERROR) {
                std::cerr << "send() 실패: " << WSAGetLastError() << std::endl;
                goto cleanup;
            }
            sent += n;
        }
    }

 cleanup:
     if (clientSocket != INVALID_SOCKET) {
         closesocket(clientSocket);
         clientSocket = INVALID_SOCKET;
     }
     WSACleanup();
     std::cout << "서버 종료." << std::endl;
     return 0;
 }