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

class Warps
{
public:
	Warps(const std::vector<std::vector<unsigned char>> &board) : board(board)
	{
	}
	struct Warp
	{
		uint16_t source=0xffff;/* bottom right corner of warp source */
		uint16_t target=0xffff;/* bottom right corner of warp target */
		bool operator==(Position p) const
		{
			return source==p || source==p+(1<<8) || source==p+1 || source==p+1+(1<<8);
		}
		bool operator==(uint16_t p) const
		{
			return source==p || source==p+(1<<8) || source==p+1 || source==p+1+(1<<8);
		}
		int16_t x_delta() const
		{
			int16_t a=target>>8;
			int16_t b=source>>8;
			return a-b;
		}
		int16_t y_delta() const
		{
			int16_t a=target & 0xff;
			int16_t b=source & 0xff;
			return a-b;
		}
		bool no_target() const
		{
			return target==0xffff;
		}
		Position get_source_top_left() const
		{
			return Position(source>>8 - 1, source & 0xff - 1);
		}
	};
	/* methods */
	void add_warp_source(unsigned long id, Position position)
	{
		if(warps.contains(id))
		{
			auto source=warps[id].source;
			if(source==0xffff)
			{
				warps[id].source=position;
			}
			else
			{
				warps[id].target=position;
				// two sources so this warp is bi-directional
				warps[id | 0x100]=Warp(position,source);
			}
		}
		else
		{
			warps[id]=Warp(position,0xffff);
		}
	}
	bool is_warp_source_position(uint8_t _x, uint8_t y) const
	{
		uint16_t x=_x;
		for(auto [id,warp] : warps)
		{
			if(warp.source  == ( x<<8    | y )   ||
				warp.source == ( x+1 <<8 | y )   ||
				warp.source == ( x<<8    | y+1 ) ||
				warp.source == ( x+1 <<8 | y+1 ))
				return true;
		}
		return false;
	}
	void add_warp_target(unsigned long id, Position position)
	{
		if(warps.contains(id))
		{
			assert(warps[id].target==0xffff);
			warps[id].target=position;
		}
		else
		{
			warps[id]=Warp(0xffff,position);
		}
	}
	std::tuple<bool, Position, bool> get_warp_target(Position worm_position, Worm worm,
		WormDirection worm_direction,
		int worm_length, bool ai_worm,
		const std::forward_list<Worm> &worms) const
	{
		for(auto [id,warp] : warps)
		{
			if(warp == worm_position)
			{
				if(warp.no_target())
				{
					return {true,random_position(worm, worm_direction, worms, ai_worm, worm_length),true};
				}
				else
				{
					Position target_position;
					target_position.x=worm_position.x;
					target_position.x+=warp.x_delta();
					target_position.y=worm_position.y;
					target_position.y+=warp.y_delta();
					switch(worm_direction)
					{
						case eDirection::EAST:
							target_position.x+=2;
							break;
						case eDirection::WEST:
							target_position.x-=2;
							break;
						case eDirection::NORTH:
							target_position.y-=2;
							break;
						case eDirection::SOUTH:
							target_position.y+=2;
							break;
						default:
							assert(false);
							break;
					}
					if(board[target_position.x][target_position.y]!=EMPTYCHAR)
					{
						if((warp.target >> 8) == target_position.x)
							target_position.x--;
						else if((warp.target >> 8) == target_position.x + 1)
							target_position.x++;
						else if((warp.target & 0xff) == target_position.y)
							target_position.y--;
						else if((warp.target & 0xff) == target_position.y + 1)
							target_position.y++;
					}
					return {true,target_position,false};
				}
			}
		}
	    return {false,Position(),false};
	}

    // Non-const iterators (allows modifying items during iteration)
    auto begin() { return warps.begin(); }
    auto end()   { return warps.end(); }

    // Const iterators (required for const Team objects)
    auto begin() const { return warps.begin(); }
    auto end()   const { return warps.end(); }
private:
	const std::vector<std::vector<unsigned char>> &board;
    std::map<unsigned long, Warp> warps;
    
    inline void increment_clear(const long x, const long y, const long clear_count, long &clear, PositionSet &positions, unsigned long &longest_clear_count) const
    {
		clear++;
		if(clear>=clear_count)
		{
			if(longest_clear_count<clear_count)
			{
				longest_clear_count=clear_count;
				positions.clear();
			}
			positions+=x<<8 | y;
		}
		else if(clear>longest_clear_count)
		{
			longest_clear_count=clear;
			positions.clear();
			positions+=x<<8 | y;
		}
		else if(clear==longest_clear_count)
			positions+=x<<8 | y;
    }
    
    inline bool is_empty(uint16_t p, const Worm::Map &worm_map) const
    {
    	if(board[p >> 8][p & 0xff]!=EMPTYCHAR)
    		return false;
    	if(worm_map.contain(p))
    		return false;
    	for(const auto &warp : warps)
    	{
    		if(warp.second == p)
    			return false;
    	}
    	return true;
    }
    
	Position random_position(Worm worm, WormDirection direction,
		const std::forward_list<Worm> &worms, bool ai_worm, int worm_length) const
	{
	    Worm::Map worm_map(worms, board.size(), board[0].size());
#if TEST_COMPILE
        /* the test are done assuming the worm is a player */
        auto clear_count = 12;
#else
        /* ai worm's don't need a long clear streatch to help them stay alive */
        auto clear_count = ai_worm?2:12;
#endif
		const uint8_t width=board.size();
		const uint8_t height=board[0].size();
		
		unsigned long longest_clear_count=0;
		PositionSet positions;
		long x,y;
		long clear=-1;
		switch(direction.reverse())
		{
			case eDirection::NORTH:
				for(x=0;x<width;x++)
				{
					for(y=height-1;y>=0;y--)
					{
						if(!is_empty(x<<8 | y, worm_map))
							clear=-1; /* start a new clear run count */
						else
							increment_clear(x, y, clear_count, clear, positions, longest_clear_count);
					}
				}
				break;
			case eDirection::SOUTH:
				for(x=0;x<width;x++)
				{
					for(y=0;y<height;y++)
					{
						if(!is_empty(x<<8 | y, worm_map))
							clear=-1; /* start a new clear run count */
						else
							increment_clear(x, y, clear_count, clear, positions, longest_clear_count);
					}
				}
				break;
			case eDirection::EAST:
				for(y=0;y<height;y++)
				{
					for(x=0;x<width;x++)
					{
						if(!is_empty(x<<8 | y, worm_map))
							clear=-1; /* start a new clear run count */
						else
							increment_clear(x, y, clear_count, clear, positions, longest_clear_count);
					}
				}
				break;
			case eDirection::WEST:
				for(y=0;y<height;y++)
				{
					for(x=width-1;x>=0;x--)
					{
						if(!is_empty(x<<8 | y, worm_map))
							clear=-1; /* start a new clear run count */
						else
							increment_clear(x, y, clear_count, clear, positions, longest_clear_count);
					}
				}
				break;
		}
		
        int lowest_deadend = std::numeric_limits<int>::max();
        Position lowest_deadend_position;
        for (;!positions.is_empty();)
        {
        	auto position=positions.remove_one(true);
            if (ai_worm)
            {
            	return position;
            }
            else /* human worm */
            {
				auto deadend = worm.ai_deadend_after(board, worms, worm_map, position, direction, worm_length);
				if (deadend <= 0)
				{
                	return position;
				}
				if (deadend < lowest_deadend)
				{
					lowest_deadend = deadend;
					lowest_deadend_position = position;
				}
            }
        }
        return lowest_deadend_position;
	}
};

