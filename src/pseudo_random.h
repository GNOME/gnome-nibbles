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

#if INTPTR_MAX == INT64_MAX
uint64_t pseudo_random();

inline uint64_t pseudo_random(uint64_t min_inclusive, uint64_t max_exclusive)
{
	return min_inclusive+(pseudo_random() % (max_exclusive - min_inclusive));
}

inline uint64_t pseudo_random(uint64_t max_exclusive)
{
	return pseudo_random() % max_exclusive;
}

void set_seed(uint64_t seed);
#else /* 32-bit compiler */
uint32_t pseudo_random();

inline uint32_t pseudo_random(uint32_t min_inclusive, uint32_t max_exclusive)
{
	return min_inclusive+(pseudo_random() % (max_exclusive - min_inclusive));
}

inline uint32_t pseudo_random(uint32_t max_exclusive)
{
	return pseudo_random() % max_exclusive;
}

void set_seed(uint32_t seed);
#endif
