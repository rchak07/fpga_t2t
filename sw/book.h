#pragma once

inline int clip(int x) {
    // return 0 if x < 0, 255 if x > 255, otherwise x
    if(x<0){
        return 0;
    }
    else if(x>255){
        return 255;
    }
    else{
        return x;
    }
}

struct Features {
    int f0, f1, f2, f3;
};

struct Book {
    int bid = 10000, ask = 10000;
    int bid_qty = 0, ask_qty = 0;
    int mid_prev = 10000;

    Features update(int side, int price, int qty) {
        // 1. side 0: set bid and bid_qty. side 1: set ask and ask_qty.
        if(side==0){
            bid = price;
            bid_qty = qty;
        }
        else{
            ask = price;
            ask_qty = qty;
        }
        // 2. mid = (bid + ask) >> 1
        int mid  = (bid+ask) >> 1;
        Features features;
        features.f0 = clip(ask - bid);
        features.f1 = clip(128 + ((bid_qty-ask_qty) >> 3));
        features.f2 = clip(128 + (mid-mid_prev));
        mid_prev = mid;
        features.f3 = clip((bid_qty + ask_qty) >> 3);
        // 3. compute f0..f3 as in the README
        // 4. mid_prev = mid (after computing f2, not before)
        // 5. return the features
        return features;
    }
};