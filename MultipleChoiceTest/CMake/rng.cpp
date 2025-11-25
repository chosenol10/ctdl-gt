// rng.cpp
#include "rng.h"
static int g_rng_seeded = 0;

void rng_seed_once() {
    if (!g_rng_seeded) {
        g_rng_seeded = 1;
        std::srand((unsigned int)std::time(NULL));
    }
}

int rand_in(int a, int b) {
    if (a > b) { int t = a; a = b; b = t; }
    rng_seed_once();
    int range = b - a + 1;
    return a + (std::rand() % range);
}
