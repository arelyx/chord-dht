#!/usr/bin/env python3
"""
Test oracle for Chord DHT (8-bit identifiers, ring size 256, m=8).
Computes expected output using only arithmetic — no Chord protocol logic.
"""

import sys

M = 8
RING = 256


# ---------------------------------------------------------------------------
# Arithmetic primitives
# ---------------------------------------------------------------------------

def successor(x, nodes):
    """First node >= x going clockwise. nodes is a sorted list."""
    x = x % RING
    for n in sorted(nodes):
        if n >= x:
            return n
    return sorted(nodes)[0]


def finger_start(node_id, k):
    """Start of finger entry k (1-indexed)."""
    return (node_id + (1 << (k - 1))) % RING


def finger_succ(node_id, k, nodes):
    return successor(finger_start(node_id, k), nodes)


def predecessor(node_id, nodes):
    s = sorted(nodes)
    for n in reversed(s):
        if n < node_id:
            return n
    return s[-1]


def in_range_half(x, a, b):
    """x in (a, b] on mod-256 ring."""
    x, a, b = x % RING, a % RING, b % RING
    if a == b:
        return True
    if a < b:
        return x > a and x <= b
    else:
        return x > a or x <= b


# ---------------------------------------------------------------------------
# TestRunner — tracks node and key state
# ---------------------------------------------------------------------------

class TestRunner:
    def __init__(self):
        self.nodes = []
        self.keys = {}  # node_id -> [(key, value)] in insertion order

    def join(self, node_id):
        self.nodes = sorted(self.nodes + [node_id])
        self.keys.setdefault(node_id, [])

    def insert(self, key, value):
        owner = successor(key, self.nodes)
        for i, (k, v) in enumerate(self.keys[owner]):
            if k == key:
                self.keys[owner][i] = (key, value)
                return
        self.keys[owner].append((key, value))

    def next_node(self, node_id):
        s = self.nodes
        idx = s.index(node_id)
        return s[(idx + 1) % len(s)]

    def prev_node(self, node_id):
        s = self.nodes
        idx = s.index(node_id)
        return s[(idx - 1) % len(s)]

    def format_finger_tables(self, node_ids=None):
        if node_ids is None:
            node_ids = list(self.nodes)
        lines = []
        for nid in node_ids:
            succ = self.next_node(nid)
            pred = self.prev_node(nid)
            lines.append(f"----------------Node id:{nid}----------------")
            lines.append(f"Successor: {succ} Predecessor: {pred}")
            lines.append("FingerTables:")
            id_digits = len(str(nid))
            pad_width = 13 + id_digits
            for k in range(1, M + 1):
                start = finger_start(nid, k)
                end = finger_start(nid, k + 1) if k < M else nid
                succ_k = finger_succ(nid, k, self.nodes)
                interval = f"[ {start} , {end} )"
                lines.append(
                    f"| k = {k} {interval:<{pad_width}}succ. = {succ_k} |"
                )
        return "\n".join(lines)

    def format_key_distribution(self, node_ids=None):
        if node_ids is None:
            node_ids = list(self.nodes)
        lines = []
        for nid in node_ids:
            lines.append(f"----------------Node id:{nid}----------------")
            pairs = self.keys.get(nid, [])
            if not pairs:
                lines.append("{}")
            else:
                inner = ", ".join(
                    f"{k}: None" if v is None else f"{k}: {v}"
                    for k, v in pairs
                )
                lines.append("{" + inner + "}")
            lines.append("")  # blank line after closing brace
        return "\n".join(lines)

    def join_new_node(self, new_id):
        """Join new_id, returning migration output string."""
        self.join(new_id)
        pred = self.prev_node(new_id)
        new_succ = self.next_node(new_id)

        migrated = []
        remaining = []
        for key, val in self.keys.get(new_succ, []):
            if in_range_half(key, pred, new_id):
                migrated.append((key, val))
            else:
                remaining.append((key, val))

        output_lines = []
        if migrated:
            output_lines.append("*" * 30)
            for key, val in migrated:
                output_lines.append(
                    f"migrate key {key} from node {new_succ} to node {new_id}"
                )
            self.keys[new_succ] = remaining
            self.keys[new_id] = migrated

        return "\n".join(output_lines)

    def format_lookup(self, origin, key):
        owner = successor(key, self.nodes)
        if owner == origin:
            path = f"[{origin}]"
        else:
            path = f"[{origin},{owner}]"
        val = None
        for k, v in self.keys.get(owner, []):
            if k == key:
                val = v
                break
        val_str = "None" if val is None else str(val)
        return (
            f"Look-up result of key {key} from node {origin} "
            f"with path {path} value is {val_str}"
        )

    def format_lookups(self, origin, keys):
        lines = [
            f"--------------------------node {origin}--------------------------"
        ]
        for key in keys:
            lines.append(self.format_lookup(origin, key))
        return "\n".join(lines)

    def leave(self, node_id):
        succ_id = self.next_node(node_id)
        # Transfer keys to successor (append in order)
        for key, val in self.keys.get(node_id, []):
            found = False
            for i, (k, v) in enumerate(self.keys[succ_id]):
                if k == key:
                    self.keys[succ_id][i] = (key, val)
                    found = True
                    break
            if not found:
                self.keys[succ_id].append((key, val))
        del self.keys[node_id]
        self.nodes.remove(node_id)


# ---------------------------------------------------------------------------
# Test cases
# ---------------------------------------------------------------------------

def test_01_baseline():
    t = TestRunner()
    for nid in [0, 30, 65, 110, 160, 230]:
        t.join(nid)

    print(t.format_finger_tables())

    for key, val in [
        (3, 3), (200, None), (123, None), (45, 3), (99, None),
        (60, 10), (50, 8), (100, 5), (101, 4), (102, 6),
        (240, 8), (250, 10),
    ]:
        t.insert(key, val)

    print(t.format_key_distribution())

    migration = t.join_new_node(100)
    if migration:
        print(migration)

    for origin in [0, 65, 100]:
        keys = [3, 200, 123, 45, 99, 60, 50, 100, 101, 102, 240, 250]
        print(t.format_lookups(origin, keys))

    t.leave(65)

    print(t.format_finger_tables([0, 30]))
    print(t.format_key_distribution([0, 30, 100, 110, 160, 230]))


def test_02_two_nodes():
    t = TestRunner()
    for nid in [0, 128]:
        t.join(nid)

    print(t.format_finger_tables())

    for key, val in [
        (0, 0), (1, 1), (64, 64), (127, 127),
        (128, 128), (129, 129), (200, 200), (255, 255),
    ]:
        t.insert(key, val)

    print(t.format_key_distribution())

    for origin in [0, 128]:
        keys = [0, 1, 64, 127, 128, 129, 200, 255]
        print(t.format_lookups(origin, keys))


def test_03_adjacent():
    t = TestRunner()
    for nid in [0, 1]:
        t.join(nid)

    print(t.format_finger_tables())

    for key, val in [(0, 0), (1, 1), (2, 2), (128, 128), (254, 254), (255, 255)]:
        t.insert(key, val)

    print(t.format_key_distribution())

    for origin in [0, 1]:
        keys = [0, 1, 2, 32, 64, 96, 128, 160, 192, 224, 254, 255]
        print(t.format_lookups(origin, keys))


def test_04_single():
    t = TestRunner()
    t.join(42)

    print(t.format_finger_tables())

    for key, val in [(0, 0), (42, 42), (100, 100), (255, 255)]:
        t.insert(key, val)

    print(t.format_key_distribution())

    keys = [0, 42, 100, 255]
    print(t.format_lookups(42, keys))


def test_05_wrap_keys():
    t = TestRunner()
    for nid in [0, 100, 200]:
        t.join(nid)

    print(t.format_finger_tables())

    for key, val in [
        (201, 201), (220, 220), (250, 250), (255, 255),
        (0, 0), (1, 1), (50, 50), (99, 99),
    ]:
        t.insert(key, val)

    print(t.format_key_distribution())

    keys = [201, 220, 250, 255, 0, 1, 50, 99]
    print(t.format_lookups(0, keys))


def test_06_key_eq_node():
    t = TestRunner()
    for nid in [50, 100, 150, 200]:
        t.join(nid)

    print(t.format_finger_tables())

    for key, val in [(50, 50), (100, 100), (150, 150), (200, 200)]:
        t.insert(key, val)

    print(t.format_key_distribution())

    keys = [50, 100, 150, 200]
    print(t.format_lookups(50, keys))


def test_07_key_at_boundary():
    t = TestRunner()
    for nid in [50, 100, 150, 200]:
        t.join(nid)

    print(t.format_finger_tables())

    for key, val in [
        (49, None), (50, None), (51, None),
        (99, None), (100, None), (101, None),
        (149, None), (150, None), (151, None),
        (199, None), (200, None), (201, None),
    ]:
        t.insert(key, val)

    print(t.format_key_distribution())

    keys = [49, 50, 51, 99, 100, 101, 149, 150, 151, 199, 200, 201]
    print(t.format_lookups(50, keys))


def test_08_even_4():
    t = TestRunner()
    for nid in [0, 64, 128, 192]:
        t.join(nid)

    print(t.format_finger_tables())

    for k in range(0, 256, 8):
        t.insert(k, k)

    print(t.format_key_distribution())

    for origin in [0, 128]:
        keys = list(range(0, 256, 16))
        print(t.format_lookups(origin, keys))


def test_09_even_8():
    t = TestRunner()
    for nid in [0, 32, 64, 96, 128, 160, 192, 224]:
        t.join(nid)

    print(t.format_finger_tables())

    for k in range(256):
        t.insert(k, k)

    print(t.format_key_distribution())

    keys = list(range(0, 256, 8))
    print(t.format_lookups(0, keys))


def test_10_dense():
    t = TestRunner()
    for nid in [10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]:
        t.join(nid)

    print(t.format_finger_tables())

    for k in range(256):
        t.insert(k, k)

    print(t.format_key_distribution())

    keys = list(range(0, 30))
    print(t.format_lookups(10, keys))


def test_11_sparse():
    t = TestRunner()
    for nid in [1, 254]:
        t.join(nid)

    print(t.format_finger_tables())

    for key, val in [
        (0, 0), (1, 1), (2, 2), (127, 127),
        (128, 128), (253, 253), (254, 254), (255, 255),
    ]:
        t.insert(key, val)

    print(t.format_key_distribution())

    for origin in [1, 254]:
        keys = [0, 1, 2, 127, 128, 253, 254, 255]
        print(t.format_lookups(origin, keys))


def test_12_join_order_asc():
    t = TestRunner()
    for nid in [10, 50, 100, 150, 200]:
        t.join(nid)

    print(t.format_finger_tables())

    inserts = [
        (9, None), (10, None), (11, None),
        (49, None), (50, None), (51, None),
        (99, None), (100, None), (101, None),
        (149, None), (150, None), (151, None),
        (199, None), (200, None), (201, None),
    ]
    for key, val in inserts:
        t.insert(key, val)

    print(t.format_key_distribution())

    keys = [k for k, v in inserts]
    print(t.format_lookups(10, keys))


def test_13_join_order_desc():
    t = TestRunner()
    for nid in [200, 150, 100, 50, 10]:
        t.join(nid)

    print(t.format_finger_tables())

    inserts = [
        (9, None), (10, None), (11, None),
        (49, None), (50, None), (51, None),
        (99, None), (100, None), (101, None),
        (149, None), (150, None), (151, None),
        (199, None), (200, None), (201, None),
    ]
    for key, val in inserts:
        t.insert(key, val)

    print(t.format_key_distribution())

    keys = [k for k, v in inserts]
    print(t.format_lookups(10, keys))


def test_14_join_between():
    t = TestRunner()
    for nid in [0, 100]:
        t.join(nid)

    for k in range(1, 101):
        t.insert(k, k)

    print(t.format_key_distribution())

    migration = t.join_new_node(50)
    if migration:
        print(migration)

    print(t.format_key_distribution())


def test_15_bulk_migration():
    t = TestRunner()
    for nid in [0, 200]:
        t.join(nid)

    for k in range(1, 200):
        t.insert(k, k)

    print(t.format_key_distribution())

    migration = t.join_new_node(100)
    if migration:
        print(migration)

    print(t.format_key_distribution())


def test_16_no_migration():
    t = TestRunner()
    for nid in [0, 100, 200]:
        t.join(nid)

    for key, val in [(150, 150), (175, 175), (199, 199)]:
        t.insert(key, val)

    print(t.format_key_distribution())

    migration = t.join_new_node(110)
    if migration:
        print(migration)

    print(t.format_key_distribution())


def test_17_wrap_migration():
    t = TestRunner()
    for nid in [50, 150]:
        t.join(nid)

    for key, val in [
        (200, 200), (210, 210), (220, 220),
        (230, 230), (240, 240), (250, 250),
    ]:
        t.insert(key, val)

    print(t.format_key_distribution())

    migration = t.join_new_node(230)
    if migration:
        print(migration)

    print(t.format_key_distribution())


def test_18_lookup_local():
    t = TestRunner()
    for nid in [0, 100, 200]:
        t.join(nid)

    for key, val in [(50, 50), (200, 200), (0, 0)]:
        t.insert(key, val)

    print(t.format_key_distribution())

    keys = [50, 200, 0]
    print(t.format_lookups(100, keys))


def test_19_lookup_all_256():
    t = TestRunner()
    for nid in [0, 64, 128, 192]:
        t.join(nid)

    for k in range(256):
        t.insert(k, k)

    print(t.format_key_distribution())

    keys = list(range(256))
    print(t.format_lookups(0, keys))


def test_20_leave_middle():
    t = TestRunner()
    for nid in [0, 50, 100, 150, 200]:
        t.join(nid)

    for k in range(1, 201):
        t.insert(k, k)

    print(t.format_key_distribution())

    t.leave(100)

    print(t.format_finger_tables([0, 50, 150, 200]))
    print(t.format_key_distribution([0, 50, 150, 200]))


def test_21_leave_successor():
    t = TestRunner()
    for nid in [0, 50, 100, 150, 200]:
        t.join(nid)

    for key, val in [(1, 1), (25, 25), (49, 49), (50, 50)]:
        t.insert(key, val)

    print(t.format_key_distribution())

    t.leave(50)

    print(t.format_finger_tables([0]))
    print(t.format_key_distribution([0, 100, 150, 200]))


def test_22_leave_all_keys():
    t = TestRunner()
    for nid in [0, 128]:
        t.join(nid)

    for k in range(1, 129):
        t.insert(k, k)

    print(t.format_key_distribution())

    t.leave(128)

    print(t.format_finger_tables([0]))
    print(t.format_key_distribution([0]))


def test_23_random_a():
    t = TestRunner()
    for nid in [7, 31, 67, 113, 163, 199, 233]:
        t.join(nid)

    inserted_keys = []
    for i in range(30):
        key = (i * 7) % 256
        t.insert(key, i)
        inserted_keys.append(key)

    print(t.format_finger_tables())
    print(t.format_key_distribution())

    print(t.format_lookups(7, inserted_keys))


def test_24_random_b():
    t = TestRunner()
    for nid in [1, 3, 7, 15, 31, 63, 127, 255]:
        t.join(nid)

    inserted_keys = []
    for i in range(40):
        key = (i * 11) % 256
        t.insert(key, i)
        inserted_keys.append(key)

    print(t.format_finger_tables())
    print(t.format_key_distribution())

    print(t.format_lookups(1, inserted_keys))


def test_25_random_c():
    t = TestRunner()
    for nid in [0, 2, 4, 6, 200, 210, 220]:
        t.join(nid)

    inserted_keys = []
    for i in range(35):
        key = (i * 13) % 256
        t.insert(key, i)
        inserted_keys.append(key)

    print(t.format_finger_tables())
    print(t.format_key_distribution())

    print(t.format_lookups(0, inserted_keys))


# ---------------------------------------------------------------------------
# Main dispatcher
# ---------------------------------------------------------------------------

def main():
    test_id = sys.argv[1]
    tests = {
        "01_baseline": test_01_baseline,
        "02_two_nodes": test_02_two_nodes,
        "03_adjacent": test_03_adjacent,
        "04_single": test_04_single,
        "05_wrap_keys": test_05_wrap_keys,
        "06_key_eq_node": test_06_key_eq_node,
        "07_key_at_boundary": test_07_key_at_boundary,
        "08_even_4": test_08_even_4,
        "09_even_8": test_09_even_8,
        "10_dense": test_10_dense,
        "11_sparse": test_11_sparse,
        "12_join_order_asc": test_12_join_order_asc,
        "13_join_order_desc": test_13_join_order_desc,
        "14_join_between": test_14_join_between,
        "15_bulk_migration": test_15_bulk_migration,
        "16_no_migration": test_16_no_migration,
        "17_wrap_migration": test_17_wrap_migration,
        "18_lookup_local": test_18_lookup_local,
        "19_lookup_all_256": test_19_lookup_all_256,
        "20_leave_middle": test_20_leave_middle,
        "21_leave_successor": test_21_leave_successor,
        "22_leave_all_keys": test_22_leave_all_keys,
        "23_random_a": test_23_random_a,
        "24_random_b": test_24_random_b,
        "25_random_c": test_25_random_c,
    }
    if test_id in tests:
        tests[test_id]()
    else:
        print(f"Unknown test: {test_id}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
