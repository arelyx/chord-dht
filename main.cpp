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
