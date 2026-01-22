#pragma once

#include "Types.h"

// ============================================================================
// Order 클래스
// ============================================================================
// 주문 정보를 담는 클래스
// 매매체결 시스템에서 주문의 모든 속성을 관리합니다.
// ============================================================================

class Order {
public:
    // ------------------------------------------------------------------------
    // 생성자
    // ------------------------------------------------------------------------
    
    // 모든 멤버 변수를 초기화하는 생성자
    // @param id 주문 고유 ID
    // @param orderType 주문 유형 (LIMIT, MARKET, CANCEL)
    // @param side 매수/매도 구분 (BUY, SELL)
    // @param price 주문 가격
    // @param quantity 주문 수량
    Order(OrderId id, OrderType orderType, Side side, Price price, Quantity quantity)
        : id_(id), orderType_(orderType), side_(side), price_(price), quantity_(quantity) {}

    // ------------------------------------------------------------------------
    // Getter 메서드 (모든 필드에 대한 접근자)
    // ------------------------------------------------------------------------
    
    // 주문 ID 반환
    // @return 주문 고유 ID
    OrderId getId() const { return id_; }
    
    // 주문 유형 반환
    // @return 주문 유형 (LIMIT, MARKET, CANCEL)
    OrderType getOrderType() const { return orderType_; }
    
    // 매수/매도 구분 반환
    // @return 매수/매도 구분 (BUY, SELL)
    Side getSide() const { return side_; }
    
    // 주문 가격 반환
    // @return 주문 가격
    Price getPrice() const { return price_; }
    
    // 주문 수량 반환
    // @return 주문 수량
    Quantity getQuantity() const { return quantity_; }

private:
    // ------------------------------------------------------------------------
    // 멤버 변수
    // ------------------------------------------------------------------------
    
    OrderId id_;              // 주문 고유 ID
    OrderType orderType_;     // 주문 유형 (지정가, 시장가, 취소)
    Side side_;              // 매수/매도 구분
    Price price_;            // 주문 가격
    Quantity quantity_;      // 주문 수량
};
