# Chord DHT -- Comprehensive Project Report

## Table of Contents

1. [What This Project Is](#1-what-this-project-is)
2. [The Chord Protocol -- Intuition](#2-the-chord-protocol----intuition)
3. [Architecture & File Layout](#3-architecture--file-layout)
4. [The Identifier Ring](#4-the-identifier-ring)
5. [Modular Interval Arithmetic](#5-modular-interval-arithmetic)
6. [Finger Tables -- The Routing Engine](#6-finger-tables----the-routing-engine)
7. [Key Lookup: find_successor & find_predecessor](#7-key-lookup-find_successor--find_predecessor)
8. [Node Join -- The Full Algorithm](#8-node-join----the-full-algorithm)
9. [Key Storage, Insert, and Remove](#9-key-storage-insert-and-remove)
10. [Key Migration on Join](#10-key-migration-on-join)
11. [Lookup with Path Reporting](#11-lookup-with-path-reporting)
12. [Node Leave](#12-node-leave)
13. [Output Formatting](#13-output-formatting)
14. [The Test Harness -- What main.cpp Does](#14-the-test-harness----what-maincpp-does)
15. [Implementation Challenges & Bugs Encountered](#15-implementation-challenges--bugs-encountered)
16. [Lessons Learned](#16-lessons-learned)

---

## 1. What This Project Is

This is a **single-process simulator of the Chord Distributed Hash Table (DHT) protocol**, written in C++17 for CSE250A at UCSC. There is no networking, no threading, no sockets. "Remote procedure calls" between nodes are just direct C++ method calls through pointers.

The project implements the complete Chord protocol as described in the original Stoica et al. paper, using **8-bit identifiers** (ring size 256). The grading criterion is **exact output matching**: the program's stdout must be character-for-character identical to a ground truth file.

**Files produced:**

| File | Lines | Purpose |
|---|---|---|
| `node.h` | 55 | Class declaration for a Chord node |
| `node.cpp` | 272 | All algorithm implementations |
| `main.cpp` | 77 | Immutable test harness (not to be modified) |
| `Makefile` | 12 | Build system with `make test` target |
| `expected_output.txt` | 166 | Ground truth output for diff-based testing |

---

## 2. The Chord Protocol -- Intuition

### The Problem

In a distributed system with N machines, you have data (key-value pairs) spread across those machines. The fundamental question is: **given a key, which machine stores it?**

Naive solutions:
- **Central directory:** One server knows where everything is. Single point of failure, bottleneck.
- **Broadcast:** Ask everyone. O(N) messages per lookup. Doesn't scale.
- **Random placement:** No way to find anything without searching everything.

### Chord's Answer

Chord arranges all machines (nodes) and all keys on a **circular number line** (like a clock). A key belongs to the first node you encounter going clockwise from the key's position. This is called the key's **successor**.

To find any key efficiently, each node maintains a **finger table** -- a set of O(log N) shortcuts that point to nodes at exponentially increasing distances around the ring. This allows any lookup to complete in O(log N) hops, even in a system with millions of nodes.

The genius is that:
- No central authority decides where keys go -- the ring structure determines it.
- Each node only needs to know about O(log N) other nodes, not all of them.
- Nodes can join and leave without disrupting the whole system.

---

## 3. Architecture & File Layout

```
chord-dht/
  node.h              -- Node class declaration
  node.cpp            -- All implementations (~270 lines)
  main.cpp            -- Test harness (immutable)
  Makefile             -- Build: g++ -std=c++17 -Wall -Wextra
  expected_output.txt  -- Ground truth for `make test`
```

### Class Design (node.h)

```cpp
class Node {
public:
    uint8_t  id_;                                    // Position on the ring (0-255)
    Node*    predecessor_;                           // Previous node counter-clockwise
    std::vector<Node*> finger_;                      // finger_[1..8], index 0 unused
    std::vector<std::pair<uint8_t,int>> localKeys_;  // Keys stored at this node

    // Public API
    void join(Node* existing_node);
    void leave();
    void insert(uint8_t key, int value = -1);
    void remove(uint8_t key);
    void find(uint8_t key);

    // Chord internals (public to simulate RPC)
    Node* find_successor(uint8_t id);
    Node* find_predecessor(uint8_t id);
    Node* closest_preceding_finger(uint8_t id);
    void  update_finger_table(Node* s, int i);
    Node* successor() { return finger_[1]; }

    // Output
    void prettyPrint();
    void printKeys();

private:
    void init_finger_table(Node* n_prime);
    void update_others();
    void migrate_keys_on_join();

    // Key helpers (insertion-order preserving)
    void setKey(uint8_t key, int value);
    void eraseKey(uint8_t key);
    int  getKey(uint8_t key);

    // Interval arithmetic (static -- no instance state needed)
    static bool in_range_open(int x, int a, int b);
    static bool in_range_half(int x, int a, int b);
    static bool in_range_closed_open(int x, int a, int b);
};
```

There is also a **global node registry**:

```cpp
extern std::vector<Node*> allNodes;  // Sorted by ID, updated on join/leave
```

This simulates the "network" -- in a real system, nodes would discover each other through the protocol. Here, we maintain a sorted list for the `leave()` operation to walk all nodes.

---

## 4. The Identifier Ring

### Concept

With `m = 8` bits, the identifier space is 0-255. It wraps: after 255 comes 0. Visually:

```
              0
           /     \
        230        30
         |          |
        160        65
           \     /
             110
```

### Key Assignment Rule

A key K is assigned to its **successor**: the first node whose ID is >= K, going clockwise. If K is larger than all node IDs, it wraps to the smallest node.

Examples with ring {0, 30, 65, 110, 160, 230}:

| Key | Reasoning | Assigned to |
|---|---|---|
| 3 | 3 < 30, so first node >= 3 is 30 | Node 30 |
| 45 | 45 < 65, so first node >= 45 is 65 | Node 65 |
| 200 | 200 < 230 | Node 230 |
| 240 | 240 > 230, wraps. First node clockwise = 0 | Node 0 |
| 99 | 99 < 110 (before n6 joins) | Node 110 |

---

## 5. Modular Interval Arithmetic

### Why This Matters

Nearly every Chord algorithm asks: "is value X inside the range (A, B) on the ring?" Because the ring wraps, standard comparison operators don't work. The number 250 IS between 230 and 10 on the ring (going clockwise from 230, through 255, wrapping to 0, up to 10).

### The Three Variants

```
in_range_open(x, a, b)        -- x in (a, b)   excludes both endpoints
in_range_half(x, a, b)        -- x in (a, b]   excludes a, includes b
in_range_closed_open(x, a, b) -- x in [a, b)   includes a, excludes b
```

### Implementation Pattern

All three follow the same logic:

```cpp
bool in_range_half(int x, int a, int b) {
    x &= 0xFF; a &= 0xFF; b &= 0xFF;   // Mask to 8 bits
    if (a == b) return true;              // Degenerate case (see below)
    if (a < b)  return (x > a && x <= b); // Normal range, no wrap
    else        return (x > a || x <= b); // Wraps around 0
}
```

**The non-wrapping case** (`a < b`): Standard comparison. Is x between a and b?

**The wrapping case** (`a > b`): The range goes from a, through 255, wrapping to 0, up to b. A value is in range if it's either past a OR before b.

### The Degenerate Case (a == b) -- Critical

When the endpoints are equal, the interval's meaning depends on which endpoints are included:

| Interval | Ring Semantics | Return |
|---|---|---|
| `(a, a]` | Start from a (exclusive), go all the way around, end at a (inclusive) = **full ring** | `true` |
| `(a, a)` | Full ring minus point a | `x != a` |
| `[a, a)` | Just the point a | `x == a` |

These edge cases were the single biggest source of bugs. Getting them wrong causes infinite loops or incorrect finger tables. More details in the [Challenges](#15-implementation-challenges--bugs-encountered) section.

### Why `int` instead of `uint8_t`

Intermediate computations use `int` to avoid unsigned underflow. The expression `(id - val) % 256` with uint8_t would wrap to a large positive number when `id < val`. Using `int` with `(id - val + 256) % 256` avoids this.

---

## 6. Finger Tables -- The Routing Engine

### Concept

Each node maintains `m = 8` finger table entries, **1-indexed** (finger[1] through finger[8]).

For a node with ID `n`, entry `k` contains:

| Field | Formula | Meaning |
|---|---|---|
| **start** | `(n + 2^(k-1)) % 256` | The position this finger "reaches for" |
| **end** | `(n + 2^k) % 256` (k<8), or `n` (k=8) | Display-only boundary |
| **node** | `successor(start)` | The actual node responsible for that position |

### Why Exponential Spacing

The starts double with each entry: +1, +2, +4, +8, +16, +32, +64, +128. This creates a **binary search structure on the ring**:

- finger[1] reaches 1 position away (your immediate neighbor)
- finger[5] reaches 16 positions away
- finger[8] reaches 128 positions away (halfway around the ring)

For node 0 in the full ring:

```
finger[1]: start=1   --> node 30   (nearby)
finger[2]: start=2   --> node 30
finger[3]: start=4   --> node 30
finger[4]: start=8   --> node 30
finger[5]: start=16  --> node 30
finger[6]: start=32  --> node 65   (farther)
finger[7]: start=64  --> node 65
finger[8]: start=128 --> node 160  (halfway around)
```

With 6 nodes on a 256-position ring, many fingers point to the same node. In a larger network with hundreds of nodes, each finger would point to a different node, and lookups would take at most 8 hops (log2(256) = 8).

### Key Invariant

`finger[1]` is always the **immediate successor**. This is the most important entry -- it's used for the `successor()` shorthand and is the backbone of the ring structure.

---

## 7. Key Lookup: find_successor & find_predecessor

### find_successor(id)

Simple wrapper:

```
find the predecessor of id
return that predecessor's successor
```

The predecessor of a key is the node just before it on the ring. That node's successor is the node responsible for the key.

### find_predecessor(id) -- The Core Routing Loop

```
n' = this (start at current node)
while id NOT IN (n'.id, n'.successor.id]:
    n' = n'.closest_preceding_finger(id)
return n'
```

**Intuition:** We hop around the ring, getting closer and closer to the target. The loop exits when we land on a node whose successor "owns" the target ID. That node is the predecessor.

The interval check uses **half-open (a, b]**: the target must be between the current node and its successor, including the successor. This is correct because the successor is the "owner" of all keys in that range.

### closest_preceding_finger(id) -- Choosing the Best Hop

```
for i = 8 down to 1:
    if finger[i].node is in (my_id, id):  // open interval
        return finger[i].node
return this
```

**Intuition:** Scan the finger table from the largest jump to the smallest. The first finger that lands between "me" and the target (without overshooting) is the best hop. We check largest first because we want to make maximum progress.

The interval is **open (a, b)**: the finger must strictly be between us and the target. If no finger qualifies, return self (we're already the closest).

---

## 8. Node Join -- The Full Algorithm

### Step 0: First Node Bootstrap

```cpp
if (n_prime == nullptr) {
    for (int i = 1; i <= M; i++) finger_[i] = this;
    predecessor_ = this;
}
```

The very first node to join has all fingers pointing to itself and is its own predecessor. It's a ring of one.

### Step 1: init_finger_table(n_prime)

**Goal:** Build the joining node's finger table using an existing node `n_prime` as a helper.

```cpp
// 1. Find our successor (who comes after us on the ring?)
finger_[1] = n_prime->find_successor((id_ + 1) % 256);

// 2. Insert ourselves into the predecessor/successor chain
predecessor_ = successor()->predecessor_;
successor()->predecessor_ = this;

// 3. Fill remaining fingers, with an optimization
for (int i = 1; i <= M - 1; i++) {
    start_next = (id_ + (1 << i)) % 256;
    if (start_next is in [id_, finger_[i]->id_)):
        finger_[i+1] = finger_[i];          // Reuse -- skip the lookup
    else:
        finger_[i+1] = n_prime->find_successor(start_next);  // Full lookup
}
```

**The optimization:** If the next finger's start falls between our ID and the current finger's node, then the current finger is also valid for the next entry. This avoids redundant `find_successor` calls.

### Step 2: update_others()

**Goal:** Tell existing nodes that they might need to update their finger tables to include the new node.

```cpp
for (int i = 1; i <= M; i++) {
    val = (id_ - 2^(i-1) + 256) % 256;
    p = find_predecessor(val);
    p->update_finger_table(this, i);
}
```

**Intuition for the formula:** `val = id - 2^(i-1)` is the farthest counter-clockwise position whose i-th finger could reach us. The predecessor of that position is the first node that might need updating.

**update_finger_table** propagates backward through predecessors:

```cpp
void update_finger_table(Node* s, int i) {
    if (s->id_ == id_) return;  // Don't update self
    if (finger_[i] == this || in_range_closed_open(s->id_, id_, finger_[i]->id_)) {
        finger_[i] = s;
        predecessor_->update_finger_table(s, i);  // Propagate backward
    }
}
```

The recursion stops when: (a) the new node doesn't improve the current finger, (b) we've gone all the way around, or (c) we hit the new node itself.

### Step 3: migrate_keys_on_join()

After joining, the new node is responsible for keys in `(predecessor, self]`. Those keys currently sit at the successor. Transfer them:

```cpp
for each key at successor:
    if key is in (predecessor->id_, id_]:
        move key from successor to self
        print "migrate key K from node OLD to node NEW"
```

---

## 9. Key Storage, Insert, and Remove

### Storage: Insertion-Order Vector

Keys are stored as `std::vector<std::pair<uint8_t, int>>` rather than `std::map`. This is because the ground truth output shows keys in **insertion order**, not sorted order. For example, node 65 shows `{45: 3, 60: 10, 50: 8}` -- sorted would be `{45, 50, 60}`.

Three helpers provide map-like operations with O(n) linear scans:

- **`setKey(key, value)`**: Linear scan to update existing or append new.
- **`eraseKey(key)`**: `remove_if` + `erase` idiom.
- **`getKey(key)`**: Linear scan, returns -2 if not found (distinguishing from -1 which means "key exists with no value").

### Insert

```cpp
void insert(uint8_t key, int value) {
    Node* target = find_successor(key);  // Find the responsible node
    target->setKey(key, value);          // Store the key there
}
```

Note: `insert` can be called from **any** node. The Chord routing finds the correct destination regardless of where you start.

### Remove

```cpp
void remove(uint8_t key) {
    Node* target = find_successor(key);
    target->eraseKey(key);
}
```

Same routing -- find the node, delete the key.

---

## 10. Key Migration on Join

When node 100 joins the ring {0, 30, 65, 110, 160, 230}:

**Before join:** Node 110 is responsible for keys 66-110. It holds keys 99 and 100.

**After join:** Node 100 is now responsible for keys 66-100. Keys 99 and 100 should move from node 110 to node 100.

The output:
```
******************************
migrate key 99 from node 110 to node 100
migrate key 100 from node 110 to node 100
```

Keys are migrated in their storage order within the successor's key list.

---

## 11. Lookup with Path Reporting

```cpp
void find(uint8_t key) {
    Node* responsible = find_successor(key);

    // Path is [origin, destination] or [origin] if local
    if (responsible == this)
        path = "[self_id]";
    else
        path = "[self_id,responsible_id]";

    // Get the value
    value = responsible->getKey(key);
    // Print: "Look-up result of key K from node N with path P value is V"
}
```

The path format is `[origin,destination]` with no spaces around the comma. If the key is stored locally (the requesting node IS the responsible node), the path is just `[origin]`.

Example outputs:
```
Look-up result of key 3 from node 0 with path [0,30] value is 3
Look-up result of key 240 from node 0 with path [0] value is 8
```

Key 240 is local to node 0, so the path has only one element.

---

## 12. Node Leave

When node 65 leaves the ring:

### Step 1: Transfer Keys to Successor

Node 65 holds keys {45: 3, 60: 10, 50: 8}. These all move to its successor (node 100 after the earlier join).

### Step 2: Stitch the Ring

```
predecessor (30) --> successor pointer updated to 100
successor (100) --> predecessor pointer updated to 30
```

The ring goes from `... -> 30 -> 65 -> 100 -> ...` to `... -> 30 -> 100 -> ...`

### Step 3: Update All Finger Tables

Walk every remaining node. Any finger that points to the departing node (65) gets replaced with its successor (100).

For example:
- Node 0's finger[6] and finger[7] pointed to 65, now point to 100
- Node 30's fingers[1-7] pointed to 65, now point to 100

### Step 4: Remove from Registry

Remove self from the sorted `allNodes` vector.

---

## 13. Output Formatting

### Finger Table Header

```
----------------Node id:X----------------
Successor: Y Predecessor: Z
FingerTables:
| k = 1 [ 1 , 2 )     succ. = 30 |
```

### Column Alignment Challenge

The spacing between `)` and `succ.` varies per row because the interval numbers have different digit counts. But the `succ.` column must be aligned within each node's table.

After analysis, the alignment formula is:

```cpp
int padWidth = 13 + digits_in_node_id;
// 1-digit ID (0-9):     padWidth = 14
// 2-digit ID (10-99):   padWidth = 15
// 3-digit ID (100-255): padWidth = 16
```

The interval string `[ START , END )` is left-aligned within `padWidth` characters using `printf("%-*s", padWidth, interval)`.

### Key Distribution Format

```
----------------Node id:X----------------
{key1: val1, key2: val2}
                                          <-- blank line after closing brace
```

Values of -1 print as `None`. Empty nodes print `{}`. Keys appear in insertion order.

---

## 14. The Test Harness -- What main.cpp Does

### Phase 1: Build the Ring (6 nodes)

```cpp
Node n0(0), n1(30), n2(65), n3(110), n4(160), n5(230);
n0.join(nullptr);  // First node -- bootstraps the ring
n1.join(&n0);      // Each subsequent node joins via an existing node
n2.join(&n1);
n3.join(&n2);
n4.join(&n3);
n5.join(&n4);
```

After this, the ring is: `0 -> 30 -> 65 -> 110 -> 160 -> 230 -> 0`

### Phase 2: Print All 6 Finger Tables

48 finger entries total. Each must match the expected output exactly.

### Phase 3: Insert 12 Keys

```cpp
n0.insert(3, 3);      // Key 3, value 3  --> lands at node 30
n1.insert(200);        // Key 200, no value --> lands at node 230
n2.insert(123);        // Key 123          --> lands at node 160
n3.insert(45, 3);      // Key 45, value 3  --> lands at node 65
n4.insert(99);         // Key 99           --> lands at node 110
n2.insert(60, 10);     // Key 60, value 10 --> lands at node 65
n0.insert(50, 8);      // Key 50, value 8  --> lands at node 65
n3.insert(100, 5);     // Key 100          --> lands at node 110
n3.insert(101, 4);     // Key 101          --> lands at node 110
n3.insert(102, 6);     // Key 102          --> lands at node 110
n5.insert(240, 8);     // Key 240          --> lands at node 0 (wraps!)
n5.insert(250, 10);    // Key 250          --> lands at node 0 (wraps!)
```

Note that `insert` can be called from any node -- the routing finds the correct destination.

### Phase 3.1: Print Key Distribution

Shows which keys ended up at which nodes:

```
Node 0:   {240: 8, 250: 10}
Node 30:  {3: 3}
Node 65:  {45: 3, 60: 10, 50: 8}     <-- insertion order, not sorted!
Node 110: {99: None, 100: 5, 101: 4, 102: 6}
Node 160: {123: None}
Node 230: {200: None}
```

### Phase 3.2: Node 100 Joins -- Key Migration

```cpp
Node n6(100);
n6.join(&n0);
```

Ring becomes: `0 -> 30 -> 65 -> 100 -> 110 -> 160 -> 230 -> 0`

Keys 99 and 100 migrate from node 110 to node 100 (they now fall in node 100's range).

### Phase 4: 36 Lookups from 3 Different Nodes

All 12 keys are looked up from nodes 0, 65, and 100. This verifies:
- Correct routing from different starting points
- Correct path reporting
- Correct value retrieval
- Local key detection (path = `[self]`)

### Phase 5: Node 65 Leaves (+20 bonus points)

```cpp
n2.leave();
```

Ring becomes: `0 -> 30 -> 100 -> 110 -> 160 -> 230 -> 0`

Then print updated finger tables for nodes 0 and 30, and key distribution for all remaining nodes. Node 100 now holds keys {99, 100, 45, 60, 50} -- the first two from the earlier migration, the last three transferred from the departing node 65.

---

## 15. Implementation Challenges & Bugs Encountered

### Challenge 1: Degenerate Interval Cases (a == b)

**The bug:** The textbook Chord algorithm's interval functions return `false` or `x == a` when `a == b`. On a ring, `(a, a]` should mean "the full ring" (start just past a, go all the way around, end at a inclusive). Returning `false` caused `find_predecessor` to loop infinitely when a single-node ring tried to look up any ID.

**The fix:** `in_range_half(x, a, a)` returns `true` (full ring). This correctly models that a single node is responsible for everything.

Similarly, `in_range_open(x, a, a)` returns `x != a` (everything except point a). This was needed for `closest_preceding_finger` when a node looks up its own ID -- without this, the function would find no valid finger and return self forever.

`in_range_closed_open(x, a, a)` returns `x == a` (just the point). This conservative behavior is correct for `update_finger_table`.

### Challenge 2: Self-Pointing Fingers During Small-Ring Joins

**The bug:** When the first node joins, all its fingers point to itself. When the second node joins and runs `update_others`, the `update_finger_table` check `in_range_closed_open(s->id_, id_, finger_[i]->id_)` evaluates to `[0, 0)` which returns `(x == 0)`. For a new node with ID 30, `30 == 0` is false, so the finger never gets updated. Node 0's finger table stays stuck pointing to itself forever.

**The fix:** Added `finger_[i] == this` as an alternative condition in `update_finger_table`:

```cpp
if (finger_[i] == this || in_range_closed_open(s->id_, id_, finger_[i]->id_))
```

If a finger points to self, **any** other node is an improvement (it's closer than going the full ring back to yourself). This allows the bootstrap fingers to be progressively corrected as nodes join.

### Challenge 3: Node Updating Its Own Finger to Itself

**The bug:** During `update_others`, `find_predecessor` can return the joining node itself (common in 2-3 node rings). This causes `update_finger_table` to be called with `s == this`, potentially setting a finger to point to itself incorrectly.

**The fix:** Added a guard at the top of `update_finger_table`:

```cpp
if (s->id_ == id_) return;  // Don't update self
```

A node should never update its own finger table during its own `update_others` -- its fingers were already correctly set by `init_finger_table`.

### Challenge 4: Output Column Alignment

**The bug:** The expected output has variable spacing between the interval `)` and `succ.`, with the `succ.` column aligned per-node but varying between nodes.

**The discovery:** After extracting the exact column positions from all 96 finger table lines in the expected output, the pattern was:

```
succ_column = 8 + 13 + digits_in_node_ID
            = 22 for 1-digit IDs
            = 23 for 2-digit IDs
            = 24 for 3-digit IDs
```

**The fix:** Two-pass prettyPrint: compute `padWidth = 13 + idDigits`, then format each interval left-aligned to that width.

### Challenge 5: Insertion-Order Key Display

**The issue:** `std::map` sorts keys, but the ground truth shows insertion order (e.g., `{45: 3, 60: 10, 50: 8}` not `{45: 3, 50: 8, 60: 10}`).

**The fix:** Used `std::vector<std::pair<uint8_t, int>>` with linear-scan helpers instead of `std::map`.

---

## 16. Lessons Learned

1. **Modular arithmetic is deceptively hard.** The three interval functions are only ~20 lines total, but getting the edge cases right (especially `a == b`) determines whether the entire system works or infinite-loops.

2. **Small rings expose algorithm assumptions.** The Chord paper's algorithms implicitly assume a ring with enough nodes that certain degenerate cases don't arise. A 1-node or 2-node ring breaks assumptions that the textbook code relies on.

3. **Output-matching grading is unforgiving.** A single extra space or wrong key order fails the entire test. This forces attention to details that wouldn't matter in production code.

4. **Test incrementally.** The most productive debugging approach was isolating exactly which operation hung (which join, which lookup, which key) rather than trying to reason about the whole system at once.

5. **The elegance of Chord is real.** Despite the implementation complexity, the core idea -- exponential shortcuts on a ring -- is genuinely beautiful. Once the finger tables are correct, every lookup converges in at most O(log N) hops, and the protocol is fully decentralized.
