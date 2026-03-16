#include "node.h"
#include <cassert>

// --- Interval helpers ---

bool Node::in_range_open(int x, int a, int b) {
    x &= 0xFF;
    a &= 0xFF;
    b &= 0xFF;

    if (a == b) {
        return x != a;
    }

    if (a < b) {
        return x > a && x < b;
    }

    return x > a || x < b;
}

bool Node::in_range_half(int x, int a, int b) {
    x &= 0xFF;
    a &= 0xFF;
    b &= 0xFF;

    if (a == b) {
        return true;
    }

    if (a < b) {
        return x > a && x <= b;
    }

    return x > a || x <= b;
}

bool Node::in_range_closed_open(int x, int a, int b) {
    x &= 0xFF;
    a &= 0xFF;
    b &= 0xFF;

    if (a == b) {
        return x == a;
    }

    if (a < b) {
        return x >= a && x < b;
    }

    return x >= a || x < b;
}

// --- Key helpers ---

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

// --- Constructor ---

Node::Node(uint8_t id) : id_(id), predecessor_(nullptr), finger_(M + 1, nullptr) {}

// --- Chord routing ---

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

// --- Join ---

void Node::join(Node* n_prime) {
    allNodes.push_back(this);
    std::sort(allNodes.begin(), allNodes.end(),
              [](Node* a, Node* b){ return a->id_ < b->id_; });

    if (n_prime != nullptr) {
        init_finger_table(n_prime);
        update_others();
        fix_self_fingers();
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
    int start = ((int)id_ + (1 << (i-1))) % 256;
    if (in_range_closed_open(s->id_, start, finger_[i]->id_)) {
        finger_[i] = s;
        Node* p = predecessor_;
        if (p != s) {
            p->update_finger_table(s, i);
        }
    }
}

void Node::fix_self_fingers() {
    for (Node* node : allNodes) {
        for (int k = 1; k <= M; k++) {
            int start = ((int)node->id_ + (1 << (k-1))) % 256;
            Node* correct = allNodes[0];
            for (Node* n : allNodes) {
                if ((int)n->id_ >= start) {
                    correct = n;
                    break;
                }
            }
            node->finger_[k] = correct;
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

// --- Insert / Remove ---

void Node::insert(uint8_t key, int value) {
    Node* target = find_successor(key);
    target->setKey(key, value);
}

void Node::remove(uint8_t key) {
    Node* target = find_successor(key);
    target->eraseKey(key);
}

// --- Find ---

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

// --- Leave ---

void Node::leave() {
    Node* succ = successor();
    Node* pred = predecessor_;

    for (auto& kv : localKeys_) {
        succ->setKey(kv.first, kv.second);
    }
    localKeys_.clear();

    pred->finger_[1] = succ;
    succ->predecessor_ = pred;

    for (Node* node : allNodes) {
        if (node == this) continue;
        for (int i = 1; i <= M; i++) {
            if (node->finger_[i] == this) {
                node->finger_[i] = succ;
            }
        }
    }

    allNodes.erase(std::remove(allNodes.begin(), allNodes.end(), this), allNodes.end());
}

// --- Output ---

void Node::prettyPrint() {
    printf("----------------Node id:%d----------------\n", id_);
    printf("Successor: %d Predecessor: %d\n", (int)successor()->id_, (int)predecessor_->id_);
    printf("FingerTables:\n");

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
