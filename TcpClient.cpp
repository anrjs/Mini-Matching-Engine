/**
 * TCP Echo Client (Windows / WinSock2)
 * 127.0.0.1:8080 의 TcpServer 에 접속해 메시지를 주고받는 테스트용 클라이언트.
 */

 #ifdef _WIN32
 #ifndef _WIN32_WINNT
 #define _WIN32_WINNT 0x0600
 #endif
 #endif
 
 #include <winsock2.h>
 #include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <limits>

#include "Common/Protocol.h"
 
 #pragma comment(lib, "ws2_32.lib")
 
 #define SERVER_PORT 8080
 
 int main() {
     // [한글 문제 해결 핵심]
     // 1. 출력(Output)을 UTF-8로 설정 (보여줄 때)
     SetConsoleOutputCP(65001);
     // 2. 입력(Input)도 UTF-8로 설정 (타이핑해서 변수에 담을 때) -> 이게 빠져서 깨졌던 겁니다.
     SetConsoleCP(65001);
 
     WSADATA wsaData;
     SOCKET sock = INVALID_SOCKET;
 
     // ----- 1. WSA 초기화 -----
     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
         std::cerr << "WSAStartup 실패" << std::endl;
         return 1;
     }
 
     // ----- 2. 소켓 생성 (IPv4, TCP) -----
     // [수정] PF_INET -> AF_INET으로 통일 (사용자 피드백 반영)
     sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (sock == INVALID_SOCKET) {
         std::cerr << "socket() 실패: " << WSAGetLastError() << std::endl;
         WSACleanup();
         return 1;
     }
 
     // ----- 3. 서버 주소 설정 -----
     sockaddr_in serverAddr = {};
     serverAddr.sin_family = AF_INET; // 여기도 AF_INET
     serverAddr.sin_port = htons(static_cast<u_short>(SERVER_PORT));
     serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
 
     // ----- 4. 서버에 연결 시도 -----
     if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
         std::cerr << "connect() 실패: " << WSAGetLastError() << std::endl;
         closesocket(sock);
         WSACleanup();
         return 1;
     }
 
   std::cout << ">>> 서버(127.0.0.1:" << SERVER_PORT << ")에 접속 성공." << std::endl;
   std::cout << "주문 정보를 입력하세요." << std::endl;
   std::cout << "형식: [1:매수, 2:매도, 3:취소] [가격/주문번호] [수량(취소시 0)]" << std::endl;
   std::cout << "예시(매수): 1 1000 10" << std::endl;
   std::cout << "예시(취소): 3 1 0" << std::endl;

    // ----- 5. 주문 / 취소 패킷 전송 루프 -----
    int currentOrderId = 1;

     while (true) {
       int cmd = 0;
       int second = 0;
       int third = 0;

       std::cout << "> ";
       if (!(std::cin >> cmd >> second >> third)) {
           if (std::cin.eof()) {
               // 입력 스트림 종료 시 루프 종료
               break;
           }
           std::cerr << "입력 오류: 숫자를 정확히 입력하세요." << std::endl;
           std::cin.clear();
           std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
           continue;
       }

       // 남은 입력 라인 정리
       std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

       OrderPacket packet{};

       if (cmd == 3) {
           // 주문 취소
           int cancelOrderId = second;
           if (cancelOrderId <= 0) {
               std::cerr << "취소할 주문번호는 0보다 커야 합니다." << std::endl;
               continue;
           }

           packet.type = static_cast<int>(PacketType::CANCEL);
           packet.orderId = cancelOrderId;
           packet.side = 0;
           packet.price = 0;
           packet.quantity = 0;

           std::cout << "주문 취소 전송 - cancelOrderId=" << cancelOrderId << std::endl;
       } else if (cmd == 1 || cmd == 2) {
           int side = cmd;
           int price = second;
           int quantity = third;

           // 간단한 유효성 검사
           if (price <= 0 || quantity <= 0) {
               std::cerr << "가격과 수량은 0보다 커야 합니다." << std::endl;
               continue;
           }

           packet.type = static_cast<int>(PacketType::ORDER);
           packet.orderId = currentOrderId++; // 자동 증가
           packet.side = side;
           packet.price = price;
           packet.quantity = quantity;

           std::cout << "주문 전송 준비 - orderId=" << packet.orderId
                     << ", side=" << side
                     << ", price=" << price
                     << ", qty=" << quantity << std::endl;
       } else {
           std::cerr << "첫 번째 값은 1(매수), 2(매도), 3(취소)만 허용됩니다." << std::endl;
           continue;
       }

       int bytesToSend = static_cast<int>(sizeof(packet));
       const char* sendData = reinterpret_cast<const char*>(&packet);

       int totalSent = 0;
       while (totalSent < bytesToSend) {
           int sent = send(sock, sendData + totalSent, bytesToSend - totalSent, 0);
           if (sent == SOCKET_ERROR) {
               std::cerr << "send() 실패: " << WSAGetLastError() << std::endl;
               goto cleanup;
           }
           totalSent += sent;
       }
     }
 
 cleanup:
     if (sock != INVALID_SOCKET) closesocket(sock);
     WSACleanup();
     return 0;
 }