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

void set_seed(uint64_t seed_a,uint64_t seed_b);

uint64_t pseudo_random();
uint64_t pseudo_random_thread_safe();

inline uintsys pseudo_random(uintsys max_exclusive)
{
	return static_cast<uintsys>(pseudo_random() % max_exclusive);
}

void set_test_prohibit(bool);
