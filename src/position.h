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
	std::unordered_set<uint16_t> set;
public:
	PositionSet()
	{
	}
	PositionSet &operator+=(uint16_t source)
	{
		set.insert(source);
		return *this;
	}
	PositionSet &operator+=(const Position &source)
	{
		set.insert(source);
		return *this;
	}
	void clear()
	{
		set.clear();
	}
	Position remove_one(bool random=false)
	{
		auto i=set.begin();
		std::advance(i, pseudo_random(0, set.size()));
		uint16_t result=*i;
		set.erase(i);
		return {result>>8, result & 0xff};
	}
	bool is_empty() const
	{
		return set.empty();
	}
};

