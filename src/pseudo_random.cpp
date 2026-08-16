/*
 * This file is part of GNOME Nibbles.
 *
 * Copyright (C) 2026 Ben Corby
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdint>
#include <cassert>
#include "system_integer.h"
#include "pseudo_random.h"

bool prohibit=false;
uint64_t last[2] = {0x2, 0x2};

void set_seed(uint64_t seed_a, uint64_t seed_b)
{
	if(seed_a==0 && seed_b==0)
	{
		last[0]=0x2;
		last[1]=0x2;
	}
	else
	{
		last[0]=seed_a;
		last[1]=seed_b;
	}
}

void set_test_prohibit(bool state)
{
	prohibit=state;
}

uint64_t pseudo_random()
{
	assert(!prohibit); /* When working in parallel worms must not use this
						  otherwise the test are not consistent. */

	/* Linear Congruential Generator */
	//const uint64_t a = 6364136223846793005ULL; /*multiplier*/
	//const uint64_t c = 1442695040888963407ULL; /*increment*/
	//pseudo_random_seed = a * pseudo_random_seed + c;
	//return pseudo_random_seed;

	/* Xorshift64* algorithm */
	//last ^= last >> 12; // a
	//last ^= last << 25; // b
	//last ^= last >> 27; // c
	//return last * 0x2545F4914F6CDD1D;

	/* Xorshift128+ algorithm */
    uint64_t s1 = last[0];
    const uint64_t s0 = last[1];
    const uint64_t result = s0 + s1; /* The "+" non-linear scrambler step */
    last[0] = s0;
    s1 ^= s1 << 23; // a
    last[1] = s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26); // b, c
    return result>>1;/* absolute lowest bit has a linear recurrence structure so avoid it */
}
