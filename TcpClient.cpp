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
     std::cout << "메시지를 입력하고 Enter를 누르세요. (종료: 빈 줄 입력)" << std::endl;
 
     // ----- 5. 통신 루프 -----
     std::string line;
     char recvBuf[4096];
 
     while (true) {
         std::cout << "> ";
         // SetConsoleCP(65001) 덕분에 이제 한글을 입력해도 UTF-8 바이트로 들어옵니다.
         if (!std::getline(std::cin, line)) {
             break;
         }
 
         if (line.empty()) {
             break;
         }
 
         // 데이터 전송
         int bytesToSend = static_cast<int>(line.size());
         const char* sendData = line.c_str();
 
         int totalSent = 0;
         while (totalSent < bytesToSend) {
             int sent = send(sock, sendData + totalSent, bytesToSend - totalSent, 0);
             if (sent == SOCKET_ERROR) {
                 std::cerr << "send() 실패" << std::endl;
                 goto cleanup;
             }
             totalSent += sent;
         }
 
         // 서버 응답 수신
         int bytesReceived = recv(sock, recvBuf, static_cast<int>(sizeof(recvBuf) - 1), 0);
         if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
             std::cout << "서버와 연결이 끊어졌습니다." << std::endl;
             break;
         }
 
         recvBuf[bytesReceived] = '\0';
         std::cout << "[Server]: " << recvBuf << std::endl;
     }
 
 cleanup:
     if (sock != INVALID_SOCKET) closesocket(sock);
     WSACleanup();
     return 0;
 }