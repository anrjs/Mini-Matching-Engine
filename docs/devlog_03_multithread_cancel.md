# 📝 [DevLog] 다중 클라이언트 접속 및 O(1) 취소 네트워크 연동

**작성일:** 2026-02-25
**작성자:** 송무건
**단계:** Phase 5 - 서버 고도화 및 엔진 기능 개방

---

## 1. 변경 사항 요약 (Changes)

단일 스레드 기반의 TCP 서버를 **멀티스레드(Multi-Threading)** 기반으로 전면 개조하여 다수의 클라이언트가 동시에 접속 및 주문을 처리할 수 있도록 아키텍처를 업그레이드함. 또한, 엔진의 핵심 기능인 **$O(1)$ 주문 취소 기능**을 통신 프로토콜에 추가하여 네트워크로 연동함.

* **Server:** `TcpServer.cpp` (스레드 분리 및 Mutex 동기화 적용, CANCEL 패킷 처리 로직 추가)
* **Client:** `TcpClient.cpp` (취소 주문 입력 파이프라인 개조)
* **Common:** `Common/Protocol.h` (`PacketType::CANCEL` 추가)

---

## 2. 핵심 기술적 의사결정 (Technical Decisions)

### A. 멀티스레딩과 자물쇠 (Mutex) 제어
여러 클라이언트(스레드)가 동시에 매칭 엔진(`OrderBook`)에 접근할 때 발생하는 데이터 레이스(Data Race)를 방지하기 위해 `std::mutex`를 도입.
~~~cpp
{
    // 임계 구역(Critical Section)을 최소화하기 위해 지역 블록(Scope) 내에서만 Lock 유지
    std::lock_guard<std::mutex> lock(obMutex);
    orderBook.addOrder(order);
    orderBook.matchOrder();
} // Scope 종료 시 자동으로 Lock 해제 (RAII 패턴)
~~~

### B. 프로토콜 확장 (Cancel Packet)
주문 접수뿐만 아니라 취소 요청도 동일한 바이너리 규격(20 Bytes)을 재사용하도록 설계.
~~~cpp
enum class PacketType : int {
    // ...
    CANCEL = 4 // 취소 패킷 타입 추가
};
~~~
클라이언트에서 `CANCEL` 패킷 전송 시, `orderId` 필드에 취소할 주문 번호를 실어 보내고, 서버는 이를 해석하여 `orderBook.cancelOrder(O(1))`를 즉각 호출함.

---

## 3. 테스트 및 검증 (Validation)

* **동시성 테스트:** 2개의 클라이언트가 동시 접속하여 교차 주문(매수/매도)을 넣었을 때, 스레드 충돌 없이 엔진 내부에서 정상적으로 체결(Match)됨을 확인.
* **취소 연동 테스트:** 클라이언트에서 전송한 취소 패킷(`type=4`)을 서버가 수신하여, 지정된 `orderId`를 맵(Hash Map)에서 찾아 즉시($O(1)$) 삭제하는 로직 정상 작동 확인.