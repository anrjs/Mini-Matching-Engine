#ifndef PROTOCOL_H
#define PROTOCOL_H

// 공통 통신 프로토콜 정의
// 패킷 구조체는 네트워크 전송 시 패딩 없이 고정 크기를 유지하기 위해
// 1바이트 정렬을 사용한다.

#pragma pack(push, 1)

// 패킷 타입 정의
enum class PacketType : int {
    LOGIN = 1,        // 로그인
    ORDER = 2,        // 주문 접수
    ORDER_RESULT = 3  // 체결 결과
};

// 주문 패킷 구조체
struct OrderPacket {
    int type;      // 패킷 타입 (항상 ORDER)
    int orderId;   // 주문 번호
    int side;      // 매수(1) / 매도(2)
    int price;     // 가격
    int quantity;  // 수량
};

#pragma pack(pop)

#endif // PROTOCOL_H

