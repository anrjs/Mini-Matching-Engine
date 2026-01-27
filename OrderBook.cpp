#include "OrderBook.h"

#include <algorithm>
#include <iostream>

// ============================================================================
// OrderBook 구현부
// ============================================================================

void OrderBook::addOrder(const Order& order) {
    // 주문 방향(Side)에 따라 매수/매도 주문 목록에 저장합니다.
    if (order.getSide() == Side::BUY) {
        bids_.push_back(order);
    } else if (order.getSide() == Side::SELL) {
        asks_.push_back(order);
    }

    // 주문 접수 직후 즉시 매칭 시도
    matchOrder();
}

void OrderBook::matchOrder() {
    // Step 1: bids(매수)는 가격 내림차순, asks(매도)는 가격 오름차순으로 정렬
    std::sort(bids_.begin(), bids_.end(),
              [](const Order& a, const Order& b) { return a.getPrice() > b.getPrice(); });
    std::sort(asks_.begin(), asks_.end(),
              [](const Order& a, const Order& b) { return a.getPrice() < b.getPrice(); });

    // Step 2: 매수/매도 주문이 모두 존재하는 동안 반복
    while (!bids_.empty() && !asks_.empty()) {
        // Step 3: 최우선 매수(bids_[0])와 최우선 매도(asks_[0]) 비교
        Order& bestBid = bids_[0];
        Order& bestAsk = asks_[0];

        if (bestBid.getPrice() < bestAsk.getPrice()) {
            // 매수가격 < 매도가격이면 체결 불가
            break;
        }

        // Step 4: 체결 처리 (가능한 최대 수량만큼 체결)
        const Quantity matchQty =
            (bestBid.getQuantity() < bestAsk.getQuantity()) ? bestBid.getQuantity() : bestAsk.getQuantity();

        // 가격은 보통 체결 규칙에 따르지만, 여기서는 매도호가(asks) 가격으로 출력
        std::cout << "체결 발생! 가격: " << bestAsk.getPrice() << ", 수량: " << matchQty << std::endl;

        bestBid.reduceQuantity(matchQty);
        bestAsk.reduceQuantity(matchQty);

        // Step 5: 잔량이 0인 주문 제거
        if (bids_[0].getQuantity() == 0) {
            bids_.erase(bids_.begin());
        }
        if (!asks_.empty() && asks_[0].getQuantity() == 0) {
            asks_.erase(asks_.begin());
        }
    }
}

// ============================================================================
// 디버깅용 출력 함수
// ============================================================================
void OrderBook::printStatus() {
    std::cout << "\n---------- [호가창 현황] ----------" << std::endl;
    
    // 매도(Sell) 주문 출력 (위에서부터)
    std::cout << "[매도(Sell) 대기열] (총 " << asks_.size() << "건)" << std::endl;
    for (const auto& order : asks_) {
        std::cout << "  - ID:" << order.getId() 
                  << " | 가격:" << order.getPrice() 
                  << " | 잔량:" << order.getQuantity() << std::endl;
    }

    std::cout << "-----------------------------------" << std::endl;

    // 매수(Buy) 주문 출력
    std::cout << "[매수(Buy) 대기열] (총 " << bids_.size() << "건)" << std::endl;
    for (const auto& order : bids_) {
        std::cout << "  - ID:" << order.getId() 
                  << " | 가격:" << order.getPrice() 
                  << " | 잔량:" << order.getQuantity() << std::endl;
    }
    std::cout << "-----------------------------------\n" << std::endl;
}

// ============================================================================
// 주문 취소
// ============================================================================
void OrderBook::cancelOrder(OrderId orderId) {
    // 1) 매수 대기열에서 탐색
    for (auto it = bids_.begin(); it != bids_.end(); ++it) {
        if (it->getId() == orderId) {
            bids_.erase(it);
            std::cout << "주문 취소 완료(ID: " << orderId << ")" << std::endl;
            return;
        }
    }

    // 2) 매도 대기열에서 탐색
    for (auto it = asks_.begin(); it != asks_.end(); ++it) {
        if (it->getId() == orderId) {
            asks_.erase(it);
            std::cout << "주문 취소 완료(ID: " << orderId << ")" << std::endl;
            return;
        }
    }

    // 3) 찾지 못한 경우
    std::cout << "취소 실패: 존재하지 않는 주문(ID: " << orderId << ")" << std::endl;
}