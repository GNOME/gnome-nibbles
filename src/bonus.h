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

class Bonus
{
public:
	enum eType
	{
        REGULAR,
        HALF,
        DOUBLE,
        LIFE,
        REVERSE,
        WARP
    };
	const uint8_t x,y; /*top left position of bonus*/
	const eType type;
	const bool fake;
	uint16_t countdown;
	bool pending_removal;

	/* constructor */
	Bonus(uint8_t x, uint8_t y, eType type, bool fake, uint16_t countdown) :
		x(x), y(y), type(type), fake(fake), countdown(countdown)
	{
		pending_removal=false;
	}
	Bonus(const Bonus &copy) :
		x(copy.x), y(copy.y), type(copy.type), fake(copy.fake), countdown(copy.countdown)
	{
		pending_removal=false;
	}

	operator uint16_t() const
	{
		return ((uint16_t)x)<<8 | y;
	}
	operator eType() const
	{
		return type;
	}
	void set_to_remove()
	{
		pending_removal=true;
	}
	/*
    struct Hash
    {
		size_t operator()(const Bonus& bonus) const noexcept
		{
		    return ((size_t)bonus.x) << 8 | bonus.y;
		}
	};*/
};

class Bonuses
{
public:
	Bonuses(uint8_t maximum=15) : maximum(maximum)
	{
		regular_left=maximum;
		missed = 0;
	}
	Bonus *operator[](uint8_t x,uint8_t y)
	{
		for(auto &b : bonus_list)
		{
			if(b.x == x && b.y == y ||
			   b.x+1 == x && b.y == y ||
			   b.x+1 == x && b.y+1 == y ||
			   b.x == x && b.y+1 == y)
			   return &b;
		}
		return nullptr;
	}
	const Bonus *operator[](uint8_t x,uint8_t y) const
	{
		for(auto &b : bonus_list)
		{
			if(b.x == x && b.y == y ||
			   b.x+1 == x && b.y == y ||
			   b.x+1 == x && b.y+1 == y ||
			   b.x == x && b.y+1 == y)
			   return &b;
		}
		return nullptr;
	}
	uint8_t new_regular_bonus_eaten()
	{
        //reset_missed();
        if (regular_left > 0)
            return maximum - (regular_left - 1);
        else
            return maximum - regular_left;
	}
	bool too_many_missed()
	{
		return missed > 2;
	}
	unsigned long single_move()
	{
		unsigned long missed_bonuses_to_replace=0;
		auto previous = bonus_list.before_begin();
		for (auto bonus = bonus_list.begin();bonus != bonus_list.end();)
		{
		    if (bonus->countdown > 0)
		    {
				bonus->countdown--;
				previous = bonus;
				bonus++;
			}
		    else
		    {
		        if (bonus->type==Bonus::REGULAR && !bonus->fake)
		        {
		            missed++;
		            missed_bonuses_to_replace++;
		        }
		    	bonus=bonus_list.erase_after(previous);
		    }
		}
		return missed_bonuses_to_replace;
	}
	void add(const Bonus &bonus)
	{
		bonus_list.push_front(bonus);
	}
	void reduce_regular()
	{
		if(regular_left>0)
			--regular_left;
	}
	inline bool last_regular_bonus() const
	{
		return regular_left == 0;
	}
	unsigned long do_pending_removes()
	{
		unsigned long real_bonuses_to_replace = 0;
		auto previous = bonus_list.before_begin();
		for (auto bonus = bonus_list.begin();bonus != bonus_list.end();)
		{
		    if (bonus->pending_removal)
		    {
				if(!last_regular_bonus() && bonus->type == Bonus::REGULAR && !bonus->fake)
					real_bonuses_to_replace++;
		    	bonus=bonus_list.erase_after(previous);
		    }
		    else
		    {
				previous = bonus;
		    	bonus++;
		    }
		}
		return real_bonuses_to_replace;
	}
	uint8_t get_regular_bonuses_consumed() const
	{
		return maximum-regular_left;
	}
	void clear()
	{
		bonus_list.clear();
		regular_left=maximum;
		missed = 0;
	}
	inline auto begin() const { return bonus_list.begin(); }
	inline auto end() const   { return bonus_list.end(); }
	inline auto cbegin() const { return bonus_list.cbegin(); }
	inline auto cend() const   { return bonus_list.cend(); }
private:
	const uint8_t maximum;
	uint8_t regular_left;
	//std::unordered_set<Bonus,Bonus::Hash> bonus_set; /*top left position of bonus*/
	std::forward_list<Bonus> bonus_list;
	unsigned long missed;
};

