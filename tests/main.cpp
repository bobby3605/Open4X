#include "../src/utils/utils.hpp"
#include <iostream>
int main() {
    std::cout << "vector" << std::endl;
    safe_vector<uint> vec;
    size_t size = 10;
    for (uint i = 0; i < size; ++i) {
        vec.push_back(i);
    }
    std::cout << "queue" << std::endl;
    safe_queue<uint> queue;
    for (uint i = 0; i < size; ++i) {
        queue.push(i);
    }
    uint expect = 0;
    uint tmp;
    while (!queue.empty()) {
        if ((tmp = queue.pop()) != expect) {
            std::cout << "failed queue on pop: got: " << tmp << " expected: " << expect << std::endl;
        }
        ++expect;
    }
    std::cout << "deque" << std::endl;
    safe_deque<uint> deque;
    for (uint i = 0; i < size; ++i) {
        deque.push_front(i);
        deque.push_back(i);
    }
    expect = size - 1;
    while (!deque.empty()) {
        if ((tmp = deque.pop_front()) != expect) {
            std::cout << "failed deque on pop front: got: " << tmp << " expected: " << expect << std::endl;
        }
        if ((tmp = deque.pop_back()) != expect) {
            std::cout << "failed deque on pop back: got: " << tmp << " expected: " << expect << std::endl;
        }
        --expect;
    }
    std::cout << "deque push back only" << std::endl;
    safe_deque<uint> deque_push_back_only;
    for (uint i = 0; i < size; ++i) {
        deque_push_back_only.push_back(i);
    }
    expect = size - 1;
    while (!deque_push_back_only.empty()) {
        if ((tmp = deque_push_back_only.pop_back()) != expect) {
            std::cout << "failed deque_push_back_only on pop back: got: " << tmp << " expected: " << expect << std::endl;
        }
        --expect;
    }
    std::cout << "deque push back pop front cycle" << std::endl;
    safe_deque<uint> deque_cycle;
    for (uint j = 0; j < 3; ++j) {
        deque_cycle.push_back(j);
        expect = j;
    }
    while (!deque_cycle.empty()) {
        if ((tmp = deque_cycle.pop_back()) != expect) {
            std::cout << "failed deque_push_back_only on pop back: got: " << tmp << " expected: " << expect << std::endl;
        }
        --expect;
    }
}
