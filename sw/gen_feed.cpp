#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include "book.h"

int main(int argc, char** argv) {
    // args: seed count prefix, e.g. ./build/gen_feed 1 50000 data/train
    unsigned seed = atoi(argv[1]);
    int count = atoi(argv[2]);
    std::string prefix = argv[3];

    std::mt19937 rng(seed);
    Book book;
    int mid = 10000;                       // hidden "true" mid price
    Features prev{};
    bool have_prev = false;

    std::ofstream hex(prefix + ".hex", std::ios::binary);
    std::ofstream csv(prefix + ".csv", std::ios::binary);

    auto wb = [&](int b) {                 // write one byte as "%02x\n"
        char buf[4];
        snprintf(buf, sizeof buf, "%02x\n", b & 0xFF);
        hex << buf;
    };

    for (int i = 0; i < count; i++) {
        // 1. imb = book.bid_qty - book.ask_qty
        double p_up;
        int imb = book.bid_qty - book.ask_qty;
        if(imb > 0){
            p_up = 0.65;
        }
        else if(imb < 0){
            p_up = 0.35;
        }
        else{
            p_up = 0.5;
        }
        //    p_up = 0.65 if imb > 0, 0.35 if imb < 0, else 0.5
        //    up = ((rng() & 0xFFFF) / 65536.0) < p_up
        bool up = ((rng() & 0xFFFF) / 65536.0) < p_up;
        // 2. mid += up ? 1 : -1
        mid  += up ? 1 : -1;
        // 3. if have_prev: write csv row "f0,f1,f2,f3,up" using prev
        if(have_prev){
            csv << prev.f0 << ","
                << prev.f1 << ","
                << prev.f2 << ","
                << prev.f3 << ","
                << up << "\n";
        }
        // 4. side = rng() % 2, offset = 1 + rng() % 3, qty = 1 + rng() % 1000
        int side = rng() % 2;
        int offset = 1 + (rng()%3);
        int qty = 1 + (rng()%1000);
        //    price = (side == 0) ? mid - offset : mid + offset
        int price = (side == 0) ? mid-offset : mid+offset;
        // 5. write 8 bytes: 0x51, side, price (4 bytes), qty (2 bytes)
        wb(0x51);
        wb(side);
        wb(price >> 24);
        wb(price >> 16);
        wb(price >> 8);
        wb(price);
        wb(qty >> 8);
        wb(qty);
        // 6. prev = book.update(side, price, qty); have_prev = true
        prev = book.update(side, price, qty);
        have_prev = true;
    }
}