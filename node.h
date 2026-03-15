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
    void    fix_self_fingers();

    void    setKey(uint8_t key, int value);
    void    eraseKey(uint8_t key);
    int     getKey(uint8_t key);

    static bool in_range_open(int x, int a, int b);
    static bool in_range_half(int x, int a, int b);
    static bool in_range_closed_open(int x, int a, int b);
};

#endif
