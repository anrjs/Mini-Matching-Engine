# 📝 [DevLog] 네트워크 파이프라인 구축 & 바이너리 프로토콜 설계

**작성일:** 2026-02-03
**작성자:** 송무건
**단계:** Phase 2 - 네트워크 기초 및 프로토콜 설계

---

## 1. 변경 사항 요약 (Changes)

기존의 로컬 시뮬레이션 환경(`main.cpp`)을 벗어나, 실제 금융 거래를 위한 **TCP 소켓 통신 환경**을 구축하였습니다. 또한 데이터 전송 효율을 극대화하기 위해 문자열이 아닌 **바이너리 구조체 기반의 프로토콜**을 설계했습니다.

* **Server:** `TcpServer.cpp` (WinSock2 기반 다중 접속 대기 서버)
* **Client:** `TcpClient.cpp` (바이너리 패킷 전송 클라이언트)
* **Common:** `Common/Protocol.h` (서버-클라이언트 공용 통신 규약)

---

## 2. 핵심 구현 상세 (Implementation Details)

### A. 프로토콜 설계와 패킹 (Struct Packing)

네트워크로 구조체를 전송할 때 컴파일러의 **자동 패딩(Padding)**으로 인해 데이터가 밀리는 현상을 방지하기 위해, 1바이트 정렬(`pack`)을 적용하여 패킷 크기를 최적화했습니다.

~~~cpp
#pragma pack(push, 1) // 1바이트 단위 정렬 (Padding 제거)

enum class PacketType : int { LOGIN = 1, ORDER = 2 };

struct OrderPacket {
    int type;      // 패킷 타입 (4B)
    int orderId;   // 주문 번호 (4B)
    int side;      // 매수/매도 (4B)
    int price;     // 가격 (4B)
    int quantity;  // 수량 (4B)
};
// 총 크기: 20 Bytes 고정
#pragma pack(pop)
~~~

### B. 바이너리 직렬화 (Binary Serialization)

금융권 표준에 맞춰, 느린 문자열 파싱 대신 **메모리 덩어리(Raw Bytes)**를 직접 전송하는 방식을 채택했습니다.

~~~cpp
// 구조체의 메모리 시작 주소(&packet)를 char 포인터로 재해석(reinterpret_cast)하여 전송
// 20바이트의 데이터가 비트 단위로 직렬화되어 전송됨
send(sock, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
~~~

### C. 인코딩 호환성 (Encoding)

Windows 콘솔(CP949)과 개발 환경(UTF-8) 간의 인코딩 불일치로 인한 **한글 깨짐 현상**을 해결하기 위해, 입출력 코드 페이지를 강제 설정했습니다.

~~~cpp
SetConsoleOutputCP(65001); // 출력 UTF-8 설정
SetConsoleCP(65001);       // 입력 UTF-8 설정
~~~

---

## 3. 트러블 슈팅 (Troubleshooting)

* **문제:** `std::cin`에 문자열 입력 시 입력 버퍼가 고장나 무한 루프 발생.
    * **해결:** `cin.clear()`로 스트림 상태를 복구하고 `cin.ignore()`로 버퍼를 비워 안정성 확보.
* **문제:** WinSock2 라이브러리 링킹 에러 (`undefined reference`).
    * **해결:** 코드 내 `#pragma comment(lib, "ws2_32.lib")` 명시 및 컴파일 옵션 `-lws2_32` 추가.

---

## 4. 향후 계획 (Next Step)

* 현재 클라이언트는 `OrderPacket`을 바이너리로 전송하고 있으나, 서버는 아직 이를 수신하여 해석하는 로직이 부재함.
* **Next:** 서버(`TcpServer`)에서 수신된 바이너리 데이터를 `OrderPacket`으로 복원(Deserialization)하여 매칭 엔진(`OrderBook`)에 전달하는 **연동 작업** 진행 예정.