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
 #include <windows.h> // [추가] 콘솔 설정용 헤더
 
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
 
     std::cout << ">>> Echo Server 대기 중... (포트 " << DEFAULT_PORT << ")" << std::endl;
 
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
 
     // ----- 6. 에코 루프 -----
     char recvBuf[RECV_BUF_SIZE];
     const char echoPrefix[] = "[Echo]: ";
     const int prefixLen = static_cast<int>(sizeof(echoPrefix) - 1);
 
     while (true) {
         int bytesReceived = recv(clientSocket, recvBuf, RECV_BUF_SIZE - 1, 0);
 
         if (bytesReceived == SOCKET_ERROR) {
             std::cerr << "recv() 실패: " << WSAGetLastError() << std::endl;
             break;
         }
         if (bytesReceived == 0) {
             std::cout << "클라이언트 연결 종료." << std::endl;
             break;
         }
 
         recvBuf[bytesReceived] = '\0';
         std::cout << "(수신) " << recvBuf << std::endl;
 
         // "[Echo]: " + 수신 메시지 전송
         std::string echoMsg = echoPrefix;
         echoMsg += recvBuf;
 
         int totalToSend = static_cast<int>(echoMsg.size());
         const char* p = echoMsg.c_str();
         int sent = 0;
 
         while (sent < totalToSend) {
             int n = send(clientSocket, p + sent, totalToSend - sent, 0);
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