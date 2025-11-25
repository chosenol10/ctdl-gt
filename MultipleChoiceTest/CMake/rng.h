// rng.h
#ifndef RNG_H
#define RNG_H

#include <ctime>
#include <cstdlib>

// Seed RAND 1 lần
void rng_seed_once();

// random integer [a,b]
int rand_in(int a, int b);

#endif // RNG_H
