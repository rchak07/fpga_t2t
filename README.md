# fpga-t2t: tick-to-trade FPGA pipeline

Simulation-only low-latency pipeline: market data parser -> top-of-book
tracker -> decision-tree signal -> order generator.
Built as a personal project.
The market feed is synthetic (an imbalance signal is injected
deliberately), so the focus is the hardware, not the alpha.

## Message format (8 bytes, big-endian)

| Byte | Field                        |
|------|------------------------------|
| 0    | type, always 0x51            |
| 1    | side (0 = bid, 1 = ask)      |
| 2-5  | price (uint32, ticks)        |
| 6-7  | qty (uint16)                 |

## Reset state

bid = ask = 10000, bid_qty = ask_qty = 0, mid_prev = 10000.
Every update produces a feature vector (no warm-up).

## Features (each clipped to 0-255)

After each update, mid = (bid + ask) >> 1.

- f0 = clip(ask - bid, 0, 255)
- f1 = clip(128 + ((bid_qty - ask_qty) >> 3), 0, 255)
- f2 = clip(128 + (mid - mid_prev), 0, 255), then mid_prev = mid
- f3 = clip((bid_qty + ask_qty) >> 3, 0, 255)

All arithmetic is signed, with an arithmetic right shift.

## Order rule

If the tree predicts class 1, buy at the current ask, qty 100.
Class 0 means no order.

## Debugging

The `majority_class` function used `uint8_t` counters instead of `int`. On
leaves with more than 255 samples, the counters silently overflowed and
wrapped around, causing the wrong class to be assigned as the majority —
even when one class clearly outnumbered the other. This was found by
printing each leaf's true class counts alongside its assigned class
during training: one leaf had 14474 class-1 samples and 7839 class-0
samples, but was assigned class 0. Fixed by changing the counters to
`int`, which fixed the overflow and raised test accuracy from ~43%
(worse than random) to ~64%.

## Results

(To be filled in: latency in cycles/ns, fmax, utilisation.)
