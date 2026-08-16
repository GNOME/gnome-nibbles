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
#include "pseudo_random.h"

bool prohibit=false;

#if INTPTR_MAX == INT64_MAX
static uint64_t last = 2; /*seed*/

/* a pseudo random number between 0 and 2^64-1 inclusive */
uint64_t pseudo_random()
{
	assert(!prohibit);/* must not be called from parallel threads or the tests fail */
	const auto a = 6364136223846793005ULL; /*multiplier*/
	const auto c = 1442695040888963407ULL; /*increment*/
	last = a * last + c;
	return last;
}

void set_seed(uint64_t seed)
{
	last=seed;
}
#else /* 32-bit compiler */
static uint32_t last = 2; /*seed*/

/* a pseudo random number between 0 and 2^64-1 inclusive */
uint32_t pseudo_random()
{
	assert(!prohibit);/* must not be called from parallel threads or the tests fail */
	const uint32_t a = 1103515245; /*multiplier*/
	const uint32_t c = 12345; /*increment*/
	last = a * last + c;
	return last;
}

void set_seed(uint32_t seed)
{
	last=seed;
}
#endif

void set_test_prohibit(bool state)
{
	prohibit=state;
}
