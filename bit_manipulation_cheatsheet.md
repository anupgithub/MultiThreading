
# 🧠 Bit Manipulation Cheatsheet

---

## 🔸 Single Bit Operations

```cpp
x & (1 << i)         // Check if bit i is set (returns non-zero if set)
x & ~(1 << i)        // Clear bit i (set it to 0)
x | (1 << i)         // Set bit i (set it to 1)
x ^ (1 << i)         // Toggle bit i (flip 0 ↔ 1)
```

---

## 🔸 Bit Range Mask

```cpp
int mask = ((1 << (r - l + 1)) - 1) << l;
// Example: l = 2, r = 4 → mask = 0b00011100
```

---

## 🛠 Apply the Mask

```cpp
x |= mask;           // Set bits from l to r
x &= ~mask;          // Clear bits from l to r
x ^= mask;           // Toggle bits from l to r
```

---

## 🔸 Get Value of Range [l, r]

```cpp
int val = (x >> l) & ((1 << (r - l + 1)) - 1);
// Extracts bits l to r (inclusive), shifted to LSB
```

---

## 🔸 Set Value of Range [l, r] to `val`

```cpp
int mask = ((1 << (r - l + 1)) - 1) << l;  // Range mask
x = (x & ~mask) | ((val << l) & mask);
// 1. Clear range bits
// 2. Set with new val aligned to l
```

---
