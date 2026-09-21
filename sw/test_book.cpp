#include <cstdio>
#include "book.h"

int main() {
    Book b;
    Features f = b.update(0, 9998, 500);
    printf("%d %d %d %d\n", f.f0, f.f1, f.f2, f.f3);
    f = b.update(1, 10002, 300);
    printf("%d %d %d %d\n", f.f0, f.f1, f.f2, f.f3);
}