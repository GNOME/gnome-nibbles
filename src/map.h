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

class SimpleMap
{
private:
	/* constants */
	const uint8_t bits;
	const int map_width_physical;

	/* variables */
	std::vector<uint64_t> map;
	//std::inplace_vector<uint64_t, ((92 - 1) / (sizeof (uint64_t) * 8) + 1) * 66> map; /* max size is 92 by 66 */

	/* public functions */
public:
	/* constructor */
	SimpleMap(uint8_t map_width, uint8_t map_height) :
		bits(sizeof (uint64_t) * 8),
		map_width_physical((map_width - 1) / bits + 1),
		map(map_width_physical * map_height)
	{
	    /*
	     * The width of this array is determined by the number of bits
	     * in the array type and the width of the map.
	     * As a location can only be empty or occupied we need one
	     * bit to represent each location on the map.
	     *
	     * If we had a map width of 64 and an array type of
	     * uint64 we would need one uint64 to store the
	     * information.
	     * The math:
	     * (map_width – 1) / bits + 1
	     * (64 – 1) / 64 + 1
	     * 63 / 64 + 1
	     * 0 + 1
	     * 1
	     *
	     * If we had a map width of 65 and an array type of
	     * uint64 we would need two uint64s to store the
	     * information.
	     * The math:
	     * (map_width – 1) / bits + 1
	     * (65 – 1) / 64 + 1
	     * 64 / 64 + 1
	     * 1 + 1
	     * 2
	     */
	}
	
	inline bool test(uint16_t p) const
	{
	    /*
	     * To test if the position p is occupied we need to locate
	     * the position within the array and the position
	     * within the unsigned integer.
	     * As each unsigned integer contains the same number of
	     * bits we can simply divide p.x by this number bits to
	     * determine the position within the array.
	     * The remainder from the above division is the bit position
	     * we want within the unsigned integer.
	     *
	     */
	    const auto x = p>>8;
	    const auto y = p & 0xff;
	    auto [quotient, remainder] = std::div(x, bits);
	    return (map[y * map_width_physical + quotient] >> remainder & 1) > 0;
	}

	inline void set(uint8_t x, uint8_t y)
	{
		auto [quotient, remainder] = std::div(x, bits);
        map[y * map_width_physical + quotient] |= 1ull << remainder;
	}
	
	inline void set(uint16_t p)
	{
		const auto x = p>>8;
		const auto y = p & 0xff;
		set(x,y);
	}
};
