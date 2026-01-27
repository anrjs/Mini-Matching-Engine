#include <iostream>
#include "OrderBook.h"
#include "Order.h"

int main() {
    OrderBook orderBook;

    std::cout << "=== [1] 매수 주문 2건 등록 ===" << std::endl;
    Order buy1(1, OrderType::LIMIT, Side::BUY, 10000, 10);
    Order buy2(2, OrderType::LIMIT, Side::BUY,  9900,  5);
    orderBook.addOrder(buy1);
    orderBook.addOrder(buy2);

    std::cout << "\n[호가창 상태 확인 - 2건 있어야 함]" << std::endl;
    orderBook.printStatus();

    std::cout << "=== [2] 주문 취소: ID 2 ===" << std::endl;
    orderBook.cancelOrder(2);  // ID 2 삭제

    std::cout << "\n[호가창 상태 확인 - ID 2가 제거되어야 함]" << std::endl;
    orderBook.printStatus();

    std::cout << "=== [3] 주문 취소: 존재하지 않는 ID 99 ===" << std::endl;
    orderBook.cancelOrder(99);  // 실패 케이스

    return 0;
}