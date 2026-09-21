# qte-t2t: tick-to-trade FPGA pipeline

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

## Results

(To be filled in: latency in cycles/ns, fmax, utilisation.)