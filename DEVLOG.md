## [2026-01-27] 성능 최적화: 주문 취소 알고리즘 개선

### 1. 문제 상황 (Problem)
- 기존에는 `std::vector`를 사용하여 주문을 저장함.
- 주문 취소(Cancel) 요청 시, ID를 찾기 위해 벡터 전체를 순회해야 함 (Linear Search).
- **시간 복잡도:** $O(N)$ (N: 주문 수)
- **위험 요소:** 주문이 수백만 건 쌓일 경우, 취소 처리에 병목 현상(Latency Spike) 발생 가능.

### 2. 해결 방안 (Solution)
- 자료구조를 `std::vector`에서 **`std::list` + `std::unordered_map`** 조합으로 변경.
  - **List:** 중간 삭제가 빈번해도 요소들의 메모리 주소(Iterator)가 유지됨.
  - **Map:** `Order ID`를 Key로, `List Iterator`를 Value로 저장하여 즉시 접근.

### 3. 결과 (Result)
- 주문 검색 및 삭제 속도가 데이터 양과 무관하게 일정해짐.
- **시간 복잡도:** $O(1)$ (Constant Time)
- **Trade-off:** Map을 위한 추가 메모리 사용이 있지만, HFT 시스템에서는 속도가 최우선임.