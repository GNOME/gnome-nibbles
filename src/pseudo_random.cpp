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

#include "pseudo_random.h"


static auto last = 2ULL; /*seed*/

/* a pseudo random number between 0 and 2^64-1 inclusive */
unsigned long pseudo_random()
{
	const auto a = 6364136223846793005ULL; /*multiplier*/ 
	const auto c = 1442695040888963407ULL; /*increment*/
	last = a * last + c;
	return last;
}

void set_seed(unsigned long seed)
{
	last=seed;
}
