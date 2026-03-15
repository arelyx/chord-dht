#include "node.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <string>

std::vector<Node*> allNodes;

void lookup_header(int id) {
    printf("--------------------------node %d--------------------------\n", id);
}

// ============================================================
// Test 01: baseline (reproduces main.cpp)
// ============================================================
void test_01_baseline() {
    allNodes.clear();

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

    n0.printKeys();
    n1.printKeys();
    n2.printKeys();
    n3.printKeys();
    n4.printKeys();
    n5.printKeys();

    Node n6(100);
    n6.join(&n0);

    std::vector<uint8_t> keys = {3, 200, 123, 45, 99, 60, 50, 100, 101, 102, 240, 250};

    lookup_header(0);
    for (auto k : keys) n0.find(k);

    lookup_header(65);
    for (auto k : keys) n2.find(k);

    lookup_header(100);
    for (auto k : keys) n6.find(k);

    n2.leave();
    n0.prettyPrint();
    n1.prettyPrint();
    n0.printKeys();
    n1.printKeys();
    n6.printKeys();
    n3.printKeys();
    n4.printKeys();
    n5.printKeys();
}

// ============================================================
// Test 02: two_nodes
// ============================================================
void test_02_two_nodes() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    nodes[0]   = new Node(0);
    nodes[128] = new Node(128);

    nodes[0]->join(nullptr);
    nodes[128]->join(nodes[0]);

    nodes[0]->prettyPrint();
    nodes[128]->prettyPrint();

    std::vector<std::pair<uint8_t,int>> inserts = {
        {0,0},{1,1},{64,64},{127,127},{128,128},{129,129},{200,200},{255,255}
    };
    for (auto& p : inserts) nodes[0]->insert(p.first, p.second);

    nodes[0]->printKeys();
    nodes[128]->printKeys();

    std::vector<uint8_t> lookup_keys = {0,1,64,127,128,129,200,255};
    int lookup_from[] = {0, 128};
    for (int from : lookup_from) {
        lookup_header(from);
        for (auto k : lookup_keys) nodes[from]->find(k);
    }

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 03: adjacent
// ============================================================
void test_03_adjacent() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    nodes[0] = new Node(0);
    nodes[1] = new Node(1);

    nodes[0]->join(nullptr);
    nodes[1]->join(nodes[0]);

    nodes[0]->prettyPrint();
    nodes[1]->prettyPrint();

    std::vector<std::pair<uint8_t,int>> inserts = {
        {0,0},{1,1},{2,2},{128,128},{254,254},{255,255}
    };
    for (auto& p : inserts) nodes[0]->insert(p.first, p.second);

    nodes[0]->printKeys();
    nodes[1]->printKeys();

    std::vector<uint8_t> lookup_keys = {0,1,2,32,64,96,128,160,192,224,254,255};
    int lookup_from[] = {0, 1};
    for (int from : lookup_from) {
        lookup_header(from);
        for (auto k : lookup_keys) nodes[from]->find(k);
    }

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 04: single
// ============================================================
void test_04_single() {
    allNodes.clear();

    Node* n = new Node(42);
    n->join(nullptr);

    n->prettyPrint();

    std::vector<std::pair<uint8_t,int>> inserts = {
        {0,0},{42,42},{100,100},{255,255}
    };
    for (auto& p : inserts) n->insert(p.first, p.second);

    n->printKeys();

    std::vector<uint8_t> lookup_keys = {0,42,100,255};
    lookup_header(42);
    for (auto k : lookup_keys) n->find(k);

    delete n;
}

// ============================================================
// Test 05: wrap_keys
// ============================================================
void test_05_wrap_keys() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {0, 100, 200};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[0]->join(nullptr);
    nodes[100]->join(nodes[0]);
    nodes[200]->join(nodes[0]);

    nodes[0]->prettyPrint();
    nodes[100]->prettyPrint();
    nodes[200]->prettyPrint();

    std::vector<std::pair<uint8_t,int>> inserts = {
        {201,201},{220,220},{250,250},{255,255},{0,0},{1,1},{50,50},{99,99}
    };
    for (auto& p : inserts) nodes[0]->insert(p.first, p.second);

    nodes[0]->printKeys();
    nodes[100]->printKeys();
    nodes[200]->printKeys();

    std::vector<uint8_t> lookup_keys = {201,220,250,255,0,1,50,99};
    lookup_header(0);
    for (auto k : lookup_keys) nodes[0]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 06: key_eq_node
// ============================================================
void test_06_key_eq_node() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {50, 100, 150, 200};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[50]->join(nullptr);
    nodes[100]->join(nodes[50]);
    nodes[150]->join(nodes[50]);
    nodes[200]->join(nodes[50]);

    for (auto& kv : nodes) kv.second->prettyPrint();

    std::vector<std::pair<uint8_t,int>> inserts = {
        {50,50},{100,100},{150,150},{200,200}
    };
    for (auto& p : inserts) nodes[50]->insert(p.first, p.second);

    for (auto& kv : nodes) kv.second->printKeys();

    std::vector<uint8_t> lookup_keys = {50,100,150,200};
    lookup_header(50);
    for (auto k : lookup_keys) nodes[50]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 07: key_at_boundary
// ============================================================
void test_07_key_at_boundary() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {50, 100, 150, 200};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[50]->join(nullptr);
    nodes[100]->join(nodes[50]);
    nodes[150]->join(nodes[50]);
    nodes[200]->join(nodes[50]);

    for (auto& kv : nodes) kv.second->prettyPrint();

    std::vector<uint8_t> insert_keys = {49,50,51,99,100,101,149,150,151,199,200,201};
    for (auto k : insert_keys) nodes[50]->insert(k);

    for (auto& kv : nodes) kv.second->printKeys();

    lookup_header(50);
    for (auto k : insert_keys) nodes[50]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 08: even_4
// ============================================================
void test_08_even_4() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {0, 64, 128, 192};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[0]->join(nullptr);
    nodes[64]->join(nodes[0]);
    nodes[128]->join(nodes[0]);
    nodes[192]->join(nodes[0]);

    for (auto& kv : nodes) kv.second->prettyPrint();

    for (int k = 0; k < 256; k += 8)
        nodes[0]->insert((uint8_t)k, k);

    for (auto& kv : nodes) kv.second->printKeys();

    std::vector<uint8_t> lookup_keys;
    for (int k = 0; k < 256; k += 16)
        lookup_keys.push_back((uint8_t)k);

    int lookup_from[] = {0, 128};
    for (int from : lookup_from) {
        lookup_header(from);
        for (auto k : lookup_keys) nodes[from]->find(k);
    }

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 09: even_8
// ============================================================
void test_09_even_8() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {0, 32, 64, 96, 128, 160, 192, 224};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[0]->join(nullptr);
    for (int i = 1; i < 8; i++)
        nodes[ids[i]]->join(nodes[0]);

    for (auto& kv : nodes) kv.second->prettyPrint();

    for (int k = 0; k < 256; k++)
        nodes[0]->insert((uint8_t)k, k);

    for (auto& kv : nodes) kv.second->printKeys();

    std::vector<uint8_t> lookup_keys;
    for (int k = 0; k < 256; k += 8)
        lookup_keys.push_back((uint8_t)k);

    lookup_header(0);
    for (auto k : lookup_keys) nodes[0]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 10: dense
// ============================================================
void test_10_dense() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {10,11,12,13,14,15,16,17,18,19,20};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[10]->join(nullptr);
    for (int i = 1; i < 11; i++)
        nodes[ids[i]]->join(nodes[10]);

    for (auto& kv : nodes) kv.second->prettyPrint();

    for (int k = 0; k < 256; k++)
        nodes[10]->insert((uint8_t)k, k);

    for (auto& kv : nodes) kv.second->printKeys();

    std::vector<uint8_t> lookup_keys;
    for (int k = 0; k < 30; k++)
        lookup_keys.push_back((uint8_t)k);

    lookup_header(10);
    for (auto k : lookup_keys) nodes[10]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 11: sparse
// ============================================================
void test_11_sparse() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {1, 254};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[1]->join(nullptr);
    nodes[254]->join(nodes[1]);

    nodes[1]->prettyPrint();
    nodes[254]->prettyPrint();

    std::vector<std::pair<uint8_t,int>> inserts = {
        {0,0},{1,1},{2,2},{127,127},{128,128},{253,253},{254,254},{255,255}
    };
    for (auto& p : inserts) nodes[1]->insert(p.first, p.second);

    nodes[1]->printKeys();
    nodes[254]->printKeys();

    std::vector<uint8_t> lookup_keys = {0,1,2,127,128,253,254,255};
    int lookup_from[] = {1, 254};
    for (int from : lookup_from) {
        lookup_header(from);
        for (auto k : lookup_keys) nodes[from]->find(k);
    }

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 12: join_order_asc
// ============================================================
void test_12_join_order_asc() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {10, 50, 100, 150, 200};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[10]->join(nullptr);
    nodes[50]->join(nodes[10]);
    nodes[100]->join(nodes[10]);
    nodes[150]->join(nodes[10]);
    nodes[200]->join(nodes[10]);

    for (auto& kv : nodes) kv.second->prettyPrint();

    std::vector<uint8_t> insert_keys = {9,10,11,49,50,51,99,100,101,149,150,151,199,200,201};
    for (auto k : insert_keys) nodes[10]->insert(k);

    for (auto& kv : nodes) kv.second->printKeys();

    lookup_header(10);
    for (auto k : insert_keys) nodes[10]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 13: join_order_desc
// ============================================================
void test_13_join_order_desc() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {200, 150, 100, 50, 10};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[200]->join(nullptr);
    nodes[150]->join(nodes[200]);
    nodes[100]->join(nodes[200]);
    nodes[50]->join(nodes[200]);
    nodes[10]->join(nodes[200]);

    // Print in sorted order (std::map is sorted)
    for (auto& kv : nodes) kv.second->prettyPrint();

    std::vector<uint8_t> insert_keys = {9,10,11,49,50,51,99,100,101,149,150,151,199,200,201};
    for (auto k : insert_keys) nodes[10]->insert(k);

    for (auto& kv : nodes) kv.second->printKeys();

    lookup_header(10);
    for (auto k : insert_keys) nodes[10]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 14: join_between
// ============================================================
void test_14_join_between() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    nodes[0]   = new Node(0);
    nodes[100] = new Node(100);

    nodes[0]->join(nullptr);
    nodes[100]->join(nodes[0]);

    for (int k = 1; k <= 100; k++)
        nodes[0]->insert((uint8_t)k, k);

    nodes[0]->printKeys();
    nodes[100]->printKeys();

    nodes[50] = new Node(50);
    nodes[50]->join(nodes[0]);

    nodes[0]->printKeys();
    nodes[50]->printKeys();
    nodes[100]->printKeys();

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 15: bulk_migration
// ============================================================
void test_15_bulk_migration() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    nodes[0]   = new Node(0);
    nodes[200] = new Node(200);

    nodes[0]->join(nullptr);
    nodes[200]->join(nodes[0]);

    for (int k = 1; k <= 199; k++)
        nodes[0]->insert((uint8_t)k, k);

    nodes[0]->printKeys();
    nodes[200]->printKeys();

    nodes[100] = new Node(100);
    nodes[100]->join(nodes[0]);

    nodes[0]->printKeys();
    nodes[100]->printKeys();
    nodes[200]->printKeys();

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 16: no_migration
// ============================================================
void test_16_no_migration() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {0, 100, 200};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[0]->join(nullptr);
    nodes[100]->join(nodes[0]);
    nodes[200]->join(nodes[0]);

    std::vector<std::pair<uint8_t,int>> inserts = {
        {150,150},{175,175},{199,199}
    };
    for (auto& p : inserts) nodes[0]->insert(p.first, p.second);

    nodes[0]->printKeys();
    nodes[100]->printKeys();
    nodes[200]->printKeys();

    nodes[110] = new Node(110);
    nodes[110]->join(nodes[0]);

    nodes[0]->printKeys();
    nodes[100]->printKeys();
    nodes[110]->printKeys();
    nodes[200]->printKeys();

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 17: wrap_migration
// ============================================================
void test_17_wrap_migration() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    nodes[50]  = new Node(50);
    nodes[150] = new Node(150);

    nodes[50]->join(nullptr);
    nodes[150]->join(nodes[50]);

    std::vector<std::pair<uint8_t,int>> inserts = {
        {200,200},{210,210},{220,220},{230,230},{240,240},{250,250}
    };
    for (auto& p : inserts) nodes[50]->insert(p.first, p.second);

    nodes[50]->printKeys();
    nodes[150]->printKeys();

    nodes[230] = new Node(230);
    nodes[230]->join(nodes[50]);

    nodes[50]->printKeys();
    nodes[150]->printKeys();
    nodes[230]->printKeys();

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 18: lookup_local
// ============================================================
void test_18_lookup_local() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {0, 100, 200};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[0]->join(nullptr);
    nodes[100]->join(nodes[0]);
    nodes[200]->join(nodes[0]);

    std::vector<std::pair<uint8_t,int>> inserts = {
        {50,50},{200,200},{0,0}
    };
    for (auto& p : inserts) nodes[0]->insert(p.first, p.second);

    nodes[0]->printKeys();
    nodes[100]->printKeys();
    nodes[200]->printKeys();

    std::vector<uint8_t> lookup_keys = {50,200,0};
    lookup_header(100);
    for (auto k : lookup_keys) nodes[100]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 19: lookup_all_256
// ============================================================
void test_19_lookup_all_256() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {0, 64, 128, 192};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[0]->join(nullptr);
    nodes[64]->join(nodes[0]);
    nodes[128]->join(nodes[0]);
    nodes[192]->join(nodes[0]);

    for (int k = 0; k < 256; k++)
        nodes[0]->insert((uint8_t)k, k);

    for (auto& kv : nodes) kv.second->printKeys();

    lookup_header(0);
    for (int k = 0; k < 256; k++)
        nodes[0]->find((uint8_t)k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 20: leave_middle
// ============================================================
void test_20_leave_middle() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {0, 50, 100, 150, 200};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[0]->join(nullptr);
    nodes[50]->join(nodes[0]);
    nodes[100]->join(nodes[0]);
    nodes[150]->join(nodes[0]);
    nodes[200]->join(nodes[0]);

    for (int k = 1; k <= 200; k++)
        nodes[0]->insert((uint8_t)k, k);

    for (auto& kv : nodes) kv.second->printKeys();

    nodes[100]->leave();

    int ft_ids[] = {0, 50, 150, 200};
    for (int id : ft_ids) nodes[id]->prettyPrint();

    int pk_ids[] = {0, 50, 150, 200};
    for (int id : pk_ids) nodes[id]->printKeys();

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 21: leave_successor
// ============================================================
void test_21_leave_successor() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {0, 50, 100, 150, 200};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[0]->join(nullptr);
    nodes[50]->join(nodes[0]);
    nodes[100]->join(nodes[0]);
    nodes[150]->join(nodes[0]);
    nodes[200]->join(nodes[0]);

    std::vector<std::pair<uint8_t,int>> inserts = {
        {1,1},{25,25},{49,49},{50,50}
    };
    for (auto& p : inserts) nodes[0]->insert(p.first, p.second);

    for (auto& kv : nodes) kv.second->printKeys();

    nodes[50]->leave();

    nodes[0]->prettyPrint();

    int pk_ids[] = {0, 100, 150, 200};
    for (int id : pk_ids) nodes[id]->printKeys();

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 22: leave_all_keys
// ============================================================
void test_22_leave_all_keys() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    nodes[0]   = new Node(0);
    nodes[128] = new Node(128);

    nodes[0]->join(nullptr);
    nodes[128]->join(nodes[0]);

    for (int k = 1; k <= 128; k++)
        nodes[0]->insert((uint8_t)k, k);

    nodes[0]->printKeys();
    nodes[128]->printKeys();

    nodes[128]->leave();

    nodes[0]->prettyPrint();
    nodes[0]->printKeys();

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 23: random_a
// ============================================================
void test_23_random_a() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {7, 31, 67, 113, 163, 199, 233};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[7]->join(nullptr);
    nodes[31]->join(nodes[7]);
    nodes[67]->join(nodes[7]);
    nodes[113]->join(nodes[7]);
    nodes[163]->join(nodes[7]);
    nodes[199]->join(nodes[7]);
    nodes[233]->join(nodes[7]);

    std::vector<uint8_t> inserted_keys;
    for (int i = 0; i < 30; i++) {
        uint8_t k = (uint8_t)((i * 7) % 256);
        nodes[7]->insert(k, i);
        inserted_keys.push_back(k);
    }

    for (auto& kv : nodes) kv.second->prettyPrint();
    for (auto& kv : nodes) kv.second->printKeys();

    lookup_header(7);
    for (auto k : inserted_keys) nodes[7]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 24: random_b
// ============================================================
void test_24_random_b() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {1, 3, 7, 15, 31, 63, 127, 255};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[1]->join(nullptr);
    nodes[3]->join(nodes[1]);
    nodes[7]->join(nodes[1]);
    nodes[15]->join(nodes[1]);
    nodes[31]->join(nodes[1]);
    nodes[63]->join(nodes[1]);
    nodes[127]->join(nodes[1]);
    nodes[255]->join(nodes[1]);

    std::vector<uint8_t> inserted_keys;
    for (int i = 0; i < 40; i++) {
        uint8_t k = (uint8_t)((i * 11) % 256);
        nodes[1]->insert(k, i);
        inserted_keys.push_back(k);
    }

    for (auto& kv : nodes) kv.second->prettyPrint();
    for (auto& kv : nodes) kv.second->printKeys();

    lookup_header(1);
    for (auto k : inserted_keys) nodes[1]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Test 25: random_c
// ============================================================
void test_25_random_c() {
    allNodes.clear();

    std::map<int, Node*> nodes;
    int ids[] = {0, 2, 4, 6, 200, 210, 220};
    for (int id : ids) nodes[id] = new Node(id);

    nodes[0]->join(nullptr);
    nodes[2]->join(nodes[0]);
    nodes[4]->join(nodes[0]);
    nodes[6]->join(nodes[0]);
    nodes[200]->join(nodes[0]);
    nodes[210]->join(nodes[0]);
    nodes[220]->join(nodes[0]);

    std::vector<uint8_t> inserted_keys;
    for (int i = 0; i < 35; i++) {
        uint8_t k = (uint8_t)((i * 13) % 256);
        nodes[0]->insert(k, i);
        inserted_keys.push_back(k);
    }

    for (auto& kv : nodes) kv.second->prettyPrint();
    for (auto& kv : nodes) kv.second->printKeys();

    lookup_header(0);
    for (auto k : inserted_keys) nodes[0]->find(k);

    for (auto& kv : nodes) delete kv.second;
}

// ============================================================
// Main dispatcher
// ============================================================
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <test_id>\n", argv[0]);
        return 1;
    }

    std::string test_id = argv[1];

    if      (test_id == "01") test_01_baseline();
    else if (test_id == "02") test_02_two_nodes();
    else if (test_id == "03") test_03_adjacent();
    else if (test_id == "04") test_04_single();
    else if (test_id == "05") test_05_wrap_keys();
    else if (test_id == "06") test_06_key_eq_node();
    else if (test_id == "07") test_07_key_at_boundary();
    else if (test_id == "08") test_08_even_4();
    else if (test_id == "09") test_09_even_8();
    else if (test_id == "10") test_10_dense();
    else if (test_id == "11") test_11_sparse();
    else if (test_id == "12") test_12_join_order_asc();
    else if (test_id == "13") test_13_join_order_desc();
    else if (test_id == "14") test_14_join_between();
    else if (test_id == "15") test_15_bulk_migration();
    else if (test_id == "16") test_16_no_migration();
    else if (test_id == "17") test_17_wrap_migration();
    else if (test_id == "18") test_18_lookup_local();
    else if (test_id == "19") test_19_lookup_all_256();
    else if (test_id == "20") test_20_leave_middle();
    else if (test_id == "21") test_21_leave_successor();
    else if (test_id == "22") test_22_leave_all_keys();
    else if (test_id == "23") test_23_random_a();
    else if (test_id == "24") test_24_random_b();
    else if (test_id == "25") test_25_random_c();
    else {
        fprintf(stderr, "Unknown test id: %s\n", test_id.c_str());
        return 1;
    }

    return 0;
}
