# Source Code

## node.h

```cpp
#ifndef NODE_H
#define NODE_H

#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <iostream>

#define M 8

class Node;
extern std::vector<Node*> allNodes;

class Node {
public:
    uint8_t  id_;
    Node*    predecessor_;
    std::vector<Node*> finger_;
    std::vector<std::pair<uint8_t,int>> localKeys_;

    Node(uint8_t id);

    void    join(Node* node);
    void    leave();
    void    insert(uint8_t key, int value = -1);
    void    remove(uint8_t key);
    void    find(uint8_t key);

    Node*   find_successor(uint8_t id);
    Node*   find_predecessor(uint8_t id);
    Node*   closest_preceding_finger(uint8_t id);
    void    update_finger_table(Node* s, int i);
    Node*   successor() { return finger_[1]; }

    void    prettyPrint();
    void    printKeys();

private:
    void    init_finger_table(Node* n_prime);
    void    update_others();
    void    migrate_keys_on_join();

    void    setKey(uint8_t key, int value);
    void    eraseKey(uint8_t key);
    int     getKey(uint8_t key);

    static bool in_range_open(int x, int a, int b);
    static bool in_range_half(int x, int a, int b);
    static bool in_range_closed_open(int x, int a, int b);
};

#endif
```

## node.cpp

```cpp
#include "node.h"
#include <cassert>

// ============================================================
// Interval helpers
// ============================================================

bool Node::in_range_open(int x, int a, int b) {
    x &= 0xFF; a &= 0xFF; b &= 0xFF;
    if (a == b) return (x != a);   // (a, a) = full ring minus {a}
    if (a < b)  return (x > a && x < b);
    else        return (x > a || x < b);
}

bool Node::in_range_half(int x, int a, int b) {
    x &= 0xFF; a &= 0xFF; b &= 0xFF;
    if (a == b) return true;   // (a, a] = full ring
    if (a < b)  return (x > a && x <= b);
    else        return (x > a || x <= b);
}

bool Node::in_range_closed_open(int x, int a, int b) {
    x &= 0xFF; a &= 0xFF; b &= 0xFF;
    if (a == b) return (x == a);
    if (a < b)  return (x >= a && x < b);
    else        return (x >= a || x < b);
}

// ============================================================
// Key helpers (insertion-order preserving)
// ============================================================

void Node::setKey(uint8_t key, int value) {
    for (auto& kv : localKeys_) {
        if (kv.first == key) { kv.second = value; return; }
    }
    localKeys_.emplace_back(key, value);
}

void Node::eraseKey(uint8_t key) {
    localKeys_.erase(
        std::remove_if(localKeys_.begin(), localKeys_.end(),
                       [key](auto& kv){ return kv.first == key; }),
        localKeys_.end());
}

int Node::getKey(uint8_t key) {
    for (auto& kv : localKeys_) {
        if (kv.first == key) return kv.second;
    }
    return -2;
}

// ============================================================
// Constructor
// ============================================================

Node::Node(uint8_t id) : id_(id), predecessor_(nullptr), finger_(M + 1, nullptr) {}

// ============================================================
// Chord core: find_successor, find_predecessor, closest_preceding_finger
// ============================================================

Node* Node::find_successor(uint8_t id) {
    Node* pred = find_predecessor(id);
    return pred->successor();
}

Node* Node::find_predecessor(uint8_t id) {
    Node* n_prime = this;
    while (!in_range_half(id, n_prime->id_, n_prime->successor()->id_)) {
        n_prime = n_prime->closest_preceding_finger(id);
    }
    return n_prime;
}

Node* Node::closest_preceding_finger(uint8_t id) {
    for (int i = M; i >= 1; i--) {
        if (in_range_open(finger_[i]->id_, id_, id)) {
            return finger_[i];
        }
    }
    return this;
}

// ============================================================
// Join
// ============================================================

void Node::join(Node* n_prime) {
    allNodes.push_back(this);
    std::sort(allNodes.begin(), allNodes.end(),
              [](Node* a, Node* b){ return a->id_ < b->id_; });

    if (n_prime != nullptr) {
        init_finger_table(n_prime);
        update_others();
        migrate_keys_on_join();
    } else {
        for (int i = 1; i <= M; i++) finger_[i] = this;
        predecessor_ = this;
    }
}

void Node::init_finger_table(Node* n_prime) {
    finger_[1] = n_prime->find_successor((id_ + 1) % 256);

    predecessor_ = successor()->predecessor_;
    successor()->predecessor_ = this;

    for (int i = 1; i <= M - 1; i++) {
        uint8_t start_next = (id_ + (1 << i)) % 256;
        if (in_range_closed_open(start_next, id_, finger_[i]->id_)) {
            finger_[i+1] = finger_[i];
        } else {
            finger_[i+1] = n_prime->find_successor(start_next);
        }
    }
}

void Node::update_others() {
    for (int i = 1; i <= M; i++) {
        int val = ((int)id_ - (1 << (i-1)) + 256) % 256;
        Node* p = find_predecessor((uint8_t)val);
        p->update_finger_table(this, i);
    }
}

void Node::update_finger_table(Node* s, int i) {
    if (s->id_ == id_) return;
    if (finger_[i] == this || in_range_closed_open(s->id_, id_, finger_[i]->id_)) {
        finger_[i] = s;
        Node* p = predecessor_;
        if (p != s) {
            p->update_finger_table(s, i);
        }
    }
}

void Node::migrate_keys_on_join() {
    Node* succ = successor();
    std::vector<uint8_t> to_migrate;

    for (auto& kv : succ->localKeys_) {
        if (in_range_half(kv.first, predecessor_->id_, id_)) {
            to_migrate.push_back(kv.first);
        }
    }

    if (!to_migrate.empty()) {
        std::cout << "******************************" << std::endl;
        for (uint8_t key : to_migrate) {
            std::cout << "migrate key " << (int)key
                      << " from node "  << (int)succ->id_
                      << " to node "    << (int)id_  << std::endl;
            setKey(key, succ->getKey(key));
            succ->eraseKey(key);
        }
    }
}

// ============================================================
// Insert / Remove
// ============================================================

void Node::insert(uint8_t key, int value) {
    Node* target = find_successor(key);
    target->setKey(key, value);
}

void Node::remove(uint8_t key) {
    Node* target = find_successor(key);
    target->eraseKey(key);
}

// ============================================================
// Find (lookup with path)
// ============================================================

void Node::find(uint8_t key) {
    Node* responsible = find_successor(key);

    std::string path;
    if (responsible == this) {
        path = "[" + std::to_string((int)id_) + "]";
    } else {
        path = "[" + std::to_string((int)id_) + ","
                   + std::to_string((int)responsible->id_) + "]";
    }

    std::string value_str = "None";
    int v = responsible->getKey(key);
    if (v != -2) {
        value_str = (v == -1) ? "None" : std::to_string(v);
    }

    std::cout << "Look-up result of key "   << (int)key
              << " from node "              << (int)id_
              << " with path "              << path
              << " value is "               << value_str << std::endl;
}

// ============================================================
// Leave
// ============================================================

void Node::leave() {
    Node* succ = successor();
    Node* pred = predecessor_;

    // Transfer keys to successor
    for (auto& kv : localKeys_) {
        succ->setKey(kv.first, kv.second);
    }
    localKeys_.clear();

    // Fix successor/predecessor pointers
    pred->finger_[1] = succ;
    succ->predecessor_ = pred;

    // Update all fingers pointing to this node
    for (Node* node : allNodes) {
        if (node == this) continue;
        for (int i = 1; i <= M; i++) {
            if (node->finger_[i] == this) {
                node->finger_[i] = succ;
            }
        }
    }

    // Remove from global registry
    allNodes.erase(std::remove(allNodes.begin(), allNodes.end(), this), allNodes.end());
}

// ============================================================
// Pretty Print
// ============================================================

void Node::prettyPrint() {
    printf("----------------Node id:%d----------------\n", id_);
    printf("Successor: %d Predecessor: %d\n", (int)successor()->id_, (int)predecessor_->id_);
    printf("FingerTables:\n");

    // Compute pad width for interval alignment
    int idDigits = 1;
    if (id_ >= 10)  idDigits = 2;
    if (id_ >= 100) idDigits = 3;
    int padWidth = 13 + idDigits;

    for (int k = 1; k <= M; k++) {
        int start = (id_ + (1 << (k-1))) % 256;
        int end   = (k < M) ? (id_ + (1 << k)) % 256 : id_;
        char interval[64];
        snprintf(interval, sizeof(interval), "[ %d , %d )", start, end);
        printf("| k = %d %-*ssucc. = %d |\n",
               k, padWidth, interval, (int)finger_[k]->id_);
    }
}

void Node::printKeys() {
    printf("----------------Node id:%d----------------\n", id_);
    printf("{");
    bool first = true;
    for (auto& kv : localKeys_) {
        if (!first) printf(", ");
        if (kv.second == -1) printf("%d: None", (int)kv.first);
        else                 printf("%d: %d",   (int)kv.first, kv.second);
        first = false;
    }
    printf("}\n\n");
}
```

## main.cpp

```cpp
#include "node.h"
#include <iostream>
#include <vector>
#include <algorithm>

std::vector<Node*> allNodes;

int main() {
    // Section 1 & 2: Build ring, print finger tables
    Node n0(0), n1(30), n2(65), n3(110), n4(160), n5(230);

    n0.join(nullptr);
    n1.join(&n0);
    n2.join(&n1);
    n3.join(&n2);
    n4.join(&n3);
    n5.join(&n4);

    n0.prettyPrint();
    n1.prettyPrint();
    n2.prettyPrint();
    n3.prettyPrint();
    n4.prettyPrint();
    n5.prettyPrint();

    // Section 3: Insert keys
    n0.insert(3, 3);
    n1.insert(200);
    n2.insert(123);
    n3.insert(45, 3);
    n4.insert(99);
    n2.insert(60, 10);
    n0.insert(50, 8);
    n3.insert(100, 5);
    n3.insert(101, 4);
    n3.insert(102, 6);
    n5.insert(240, 8);
    n5.insert(250, 10);

    // Section 3.1: Key distribution
    n0.printKeys();
    n1.printKeys();
    n2.printKeys();
    n3.printKeys();
    n4.printKeys();
    n5.printKeys();

    // Section 3.2: n6 joins — migration printed automatically inside join()
    Node n6(100);
    n6.join(&n0);

    // Section 4: Lookups
    std::vector<uint8_t> keys = {3, 200, 123, 45, 99, 60, 50, 100, 101, 102, 240, 250};

    std::cout << "--------------------------node 0--------------------------" << std::endl;
    for (auto k : keys) n0.find(k);

    std::cout << "--------------------------node 65--------------------------" << std::endl;
    for (auto k : keys) n2.find(k);

    std::cout << "--------------------------node 100--------------------------" << std::endl;
    for (auto k : keys) n6.find(k);

    // Section 5: n2 leaves (optional +20pts)
    n2.leave();
    n0.prettyPrint();
    n1.prettyPrint();
    n0.printKeys();
    n1.printKeys();
    n6.printKeys();
    n3.printKeys();
    n4.printKeys();
    n5.printKeys();

    return 0;
}
```
