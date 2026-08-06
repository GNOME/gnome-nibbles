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

enum eDirection
{
	NONE,
	RIGHT,
	EAST = RIGHT,
	DOWN,
	SOUTH = DOWN,
	LEFT,
	WEST = LEFT,
	UP,
	NORTH = UP
};
class WormDirection
{
private:
	eDirection direction;
public:
	WormDirection()
	{
		direction=eDirection::NONE;
	}
	WormDirection(const eDirection &copy) :
		direction(copy)
	{
	}
	eDirection turn_left() const
	{
		switch (direction)
		{
			case EAST:
				return NORTH;
			case NORTH:
				return WEST;
			case WEST:
				return SOUTH;
			case SOUTH:
				return EAST;
			default:
				assert(false);
		}
	}
	eDirection turn_right() const
	{
		switch (direction)
		{
			case EAST:
				return SOUTH;
			case SOUTH:
				return WEST;
			case WEST:
				return NORTH;
			case NORTH:
				return EAST;
			default:
				assert(false);
		}
	}
	std::vector<eDirection> get_space_fill_array() const
	{
		switch (direction)
		{
			case WEST:
				return {SOUTH,WEST,NORTH};
			case NORTH:
				return {EAST,WEST,NORTH};
			case EAST:
				return {EAST,SOUTH,NORTH};
			case SOUTH:
				return {EAST,SOUTH,WEST};
			default:
				assert(false);
		}
	}
	eDirection reverse() const
	{
		switch (direction)
		{
			case EAST:
				return WEST;
			case SOUTH:
				return NORTH;
			case WEST:
				return EAST;
			case NORTH:
				return SOUTH;
			default:
				assert(false);
		}
	}
	operator eDirection() const
	{
		return direction;
	}
};

struct Position
{
	uint8_t x,y;
	void move(eDirection d, uint8_t board_width, uint8_t board_height)
	{
		switch(d)
		{
			case UP:
				if(y>0)
					y--;
				else
					y=board_height-1;
				break;
			case EAST:
				if(x<board_width-1)
					x++;
				else
					x=0;
				break;
			case DOWN:
				if(y<board_height-1)
					y++;
				else
					y=0;
				break;
			case WEST:
				if(x>0)
					x--;
				else
					x=board_width-1;
				break;
			default:
				break;
		}
	}
	operator uint16_t() const
	{
		return ((uint16_t)x)<<8 | y;
	}
};

struct Start
{
	WormDirection direction;
	Position position;
};

class PositionSet
{
private:
	std::unordered_set<uint16_t> s;/* max size is 92 by 66 */
public:
	PositionSet()
	{
		s.reserve(92*66);
	}
	void set(uint8_t x, uint8_t y)
	{
		s.emplace(x*66+y);
	}
	void clear()
	{
		s.clear();
	}
	Position remove_one()
	{
		return remove_one(pseudo_random());
	}
	Position remove_one(unsigned long random_number)
	{
		auto i=s.begin();
		std::advance(i, random_number % s.size());
		uint16_t result=*i;
		s.erase(i);
		return {result/66, result % 66};
	}
	std::pair<uint8_t, uint8_t> remove_one_bonus()
	{
		auto i=s.begin();
		std::advance(i, pseudo_random(0, s.size()));
		uint16_t result=*i;
		s.erase(i);
		s.erase(result+1);
		s.erase(result+66);
		s.erase(result+66+1);
		return {result/66, result % 66};
	}
	bool is_empty() const
	{
		return s.empty();
	}
};

