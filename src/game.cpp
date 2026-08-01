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

#include <iostream>
#include <cassert>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <thread>
#include <functional>
#include <map>
#include <mutex>
//#include <inplace_vector>
#include <queue>

#include <gtkmm.h>
#include <gsk/gsk.h>
//#include <source_location>/* for std::source_location::current().file_name() */
//#include <unordered_set>/* for std::unordered_set */
#include <fstream>/* for std::ifstream */
#include <vector>/* for std::vector */
#include <string>/* for std::string */
/* language */
#include <locale>
#include <glib/gi18n.h>

#include "definitions.h"
#include "pseudo_random.h"
#include "map.h"
#include "bonus.h"
#include "position.h"
#include "worm.h"
#include "warp.h"
#include "game.h"
//#include "view.h"


bool Game::load_board_from_file(const char *path, unsigned long _level)
{
	level = _level;
	std::ifstream map_file(path/*,std::ios::binary*/);
	if (map_file.is_open())
	{		
		board.clear();
		starts.clear();
		width=92;// boards loaded from file are a fix width
		height=66;// boards loaded from file are a fix height

		uint32_t u32;
		while(get_unichar(map_file,u32))
		{
			build_board(u32, board);
			if(board.size()==width && board[board.size()-1].size()==height)
				return true; // we have loaded a full board
		}
	}
	return false;
}

bool Game::get_unichar(std::ifstream &stream, uint32_t &u32)
{
	char c;
	if(stream.get(c))
	{
		u32=c & 0xff;
		unsigned int read_more_characters=unichar_extra_width(c);
		for(;read_more_characters>0;read_more_characters--)
		{
			if(stream.get(c))
			{
				u32<<=8;
				u32|=c & 0xff;
			}
			else
				return false;
		}
		return true;
	}
	else
		return false;
}

std::tuple<unsigned char, Game::WarpType, WormDirection> Game::to_board_char(uint32_t u32)
{
    switch (u32)
    {
    	case '\n': // new line
    	case '\r': // carrage return
    		return {'\0',WarpType::NONE,eDirection::NONE};
    		break;
        // readable empty tile, but the game internals use an 'a'
        case '.':
        case '+':
    		return {'a',WarpType::NONE,eDirection::NONE};
            break;

        // readable walls, but the game internals use ASCII chars
        case '┃':
    		return {'b',WarpType::NONE,eDirection::NONE};
            break;
        case '━':
    		return {'c',WarpType::NONE,eDirection::NONE};
            break;
        case '┗':
    		return {'d',WarpType::NONE,eDirection::NONE};
            break;
        case '┛':
    		return {'e',WarpType::NONE,eDirection::NONE};
            break;
        case '┏':
    		return {'f',WarpType::NONE,eDirection::NONE};
            break;
        case '┓':
    		return {'g',WarpType::NONE,eDirection::NONE};
            break;
        case '┻':
    		return {'h',WarpType::NONE,eDirection::NONE};
            break;
        case '┣':
    		return {'i',WarpType::NONE,eDirection::NONE};
            break;
        case '┫':
    		return {'j',WarpType::NONE,eDirection::NONE};
            break;
        case '┳':
    		return {'k',WarpType::NONE,eDirection::NONE};
            break;
        case '╋':
    		return {'l',WarpType::NONE,eDirection::NONE};
            break;

        // start positions
        case '▲':
        case 'm':
        	//starts.push_front(Start(WormDirection::NORTH,position));
    		return {'a',WarpType::NONE,eDirection::NORTH};
            //if (count < numworms)
            //{
            //    worms[count].set_start (j, i, WormDirection.UP);
            //    count++;
            //}
            break;
        case '◀':
        case 'n':
    		return {'a',WarpType::NONE,eDirection::WEST};
            //if (count < numworms)
            //{
            //    worms[count].set_start (j, i, WormDirection.LEFT);
            //    count++;
            //}
            break;
        case '▼':
        case 'o':
    		return {'a',WarpType::NONE,eDirection::SOUTH};
            //if (count < numworms)
            //{
            //    worms[count].set_start (j, i, WormDirection.DOWN);
            //    count++;
            //}
            break;
        case '▶':
        case 'p':
    		return {'a',WarpType::NONE,eDirection::EAST};
            //if (count < numworms)
            //{
            //    worms[count].set_start (j, i, WormDirection.RIGHT);
            //    count++;
            //}
            break;

        // warps
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
            //if (j == 0 || i == 0)
            //    return false;

            //warp_manager.add_warp_source (board[j, i], j - 1, i - 1, char_value == 'Q');

            //board[j - 1, i - 1] = NibblesGame.WARPCHAR;
            //board[j    , i - 1] = NibblesGame.WARPCHAR;
            //board[j - 1, i    ] = NibblesGame.WARPCHAR;
            //board[j    , i    ] = NibblesGame.WARPCHAR;

            //warp_added (j - 1, i - 1);
    		return {EMPTYCHAR/*(unsigned char)u32*/,WarpType::SOURCE,eDirection::NONE};
            break;

        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
            // do not use the up () method: it depends on the locale, and that could have some weird results ("i".up () is either I or İ, for example)
            //warp_manager.add_warp_target ((int) char_value - (int) 'a' + (int) 'A', j, i);
    		return {EMPTYCHAR/*(unsigned char)u32*/,WarpType::TARGET,eDirection::NONE};
            break;

        // old walls, kept for compatibility
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
    		return {(unsigned char)u32,WarpType::NONE,eDirection::NONE};
            break;

        default:
    		return {'\0',WarpType::NONE,eDirection::NONE};
        	break;
    }
}

std::pair<uint8_t, uint8_t> Game::remove_bonus_location(std::unordered_set<uint16_t> &locations)
{
	/* get a random value from set of positions */
	auto pick = locations.cbegin();
	std::advance(pick, pseudo_random(0,locations.size()));
	uint8_t x=*pick >> 8;
	uint8_t y=*pick & 0xff;
	locations.erase(pick);
	locations.erase((((uint16_t)x)+1)<<8 | y);
	locations.erase(((uint16_t)x)<<8 | y+1);
	locations.erase((((uint16_t)x)+1)<<8 | y+1);
	return {x,y};
}

bool Game::add_bonus(bool regular)
{
	uint8_t x,y; /* max size is 92 by 66 */
	std::unordered_set<uint16_t> free_locations;
	free_locations.reserve(0x10000);
    Worm::Map worm_map(worms, board.size(), board[0].size());

	/* non regular bonuses have a chance of 1 in 50 of appearing */
	if (!regular)
	{
		if (pseudo_random(0, 50) != 0)
		    return true;
	}

	/* build the set of positions that can take a bonus */
	for(y=0;y<board[0].size()-1;y++)
	{
		for(x=0;x<board.size()-1;x++)
		{
			if(board[x][y]==EMPTYCHAR && !worm_map.contain_position(x,y) && bonuses[x,y]==nullptr && !warps.is_warp_source_position(x,y) &&
				board[x+1][y]==EMPTYCHAR && !worm_map.contain_position(x+1,y) && bonuses[x+1,y]==nullptr && !warps.is_warp_source_position(x+1,y) &&
				board[x][y+1]==EMPTYCHAR && !worm_map.contain_position(x,y+1) && bonuses[x,y+1]==nullptr && !warps.is_warp_source_position(x,y+1) &&
				board[x+1][y+1]==EMPTYCHAR && !worm_map.contain_position(x+1,y+1) && bonuses[x+1,y+1]==nullptr && !warps.is_warp_source_position(x+1,y+1))
				free_locations.emplace(((uint16_t)x)<<8 | y);
		}
	}

	if(regular)
	{
		if(!free_locations.empty())
		{
			std::tie(x, y)=remove_bonus_location(free_locations);/* get a random value from the set of positions */
			_add_bonus(x, y, Bonus::REGULAR, false, 300);
		}
		else
			return false;
		
		if(!free_locations.empty() && fakes && pseudo_random(0, 7)==0)
		{
			std::tie(x, y)=remove_bonus_location(free_locations);
		    _add_bonus(x, y, Bonus::REGULAR, true, 300);
		}
	}
	else if(!bonuses.too_many_missed())
	{
		bool good;
		if (pseudo_random(0, 7)!=0)
		    good = false;
		else
		    good = true;

		if (good && !fakes)
		    return true;

		switch (pseudo_random(0, 21))
		{
		    case 0:
		    case 1:
		    case 2:
		    case 3:
		    case 4:
		    case 5:
		    case 6:
		    case 7:
		    case 8:
		    case 9:
				if(!free_locations.empty())
				{
					std::tie(x, y)=remove_bonus_location(free_locations);
				    _add_bonus(x, y, Bonus::HALF, good, 200);
				}
		        break;
		    case 10:
		    case 11:
		    case 12:
		    case 13:
		    case 14:
				if(!free_locations.empty())
				{
					std::tie(x, y)=remove_bonus_location(free_locations);
				    _add_bonus(x, y, Bonus::DOUBLE, good, 150);
				}
		        break;
		    case 15:
				if(!free_locations.empty())
				{
					std::tie(x, y)=remove_bonus_location(free_locations);
				    _add_bonus(x, y, Bonus::LIFE, good, 100);
				}
		        break;
		    case 16:
		    case 17:
		    case 18:
		    case 19:
		    case 20:
		        if (!free_locations.empty() && two_or_more_worms())
		        {
					std::tie(x, y)=remove_bonus_location(free_locations);
		            _add_bonus(x, y, Bonus::REVERSE, good, 150);
				}
		        break;
		}
	}
	return true;
}

void Game::move_worms()
{
	// manage still worms
    for(Worm &worm : worms)
    {
    	if(worm.decrement_still())
    	{
    		if(progress==TEST)
    			std::cout << "Worm " << (unsigned long)worm.get_colour() << 
    				" still counter decremented to " << worm.get_rounds_to_stay_still() << std::endl;
    	}
    }

	// reduce all worms score if bonuses are not being taken
    if (bonuses.too_many_missed())
    {
        for(Worm &worm : worms)
        {
	    	if(worm.decrement_score())
	    	{
				if(progress==TEST)
	    			std::cout << "Worm " << (unsigned long)worm.get_colour() << 
					" score decremented to " << worm.get_score() << std::endl;
	    	}
        }
    }

    for (;bonuses_to_replace>0; --bonuses_to_replace)
    {
		bool r=add_bonus(true);
		if(progress==TEST)
		{
			if(r)
				std::cout << "Added a bonus we couldn't add before because of lack of space" << std::endl;
		}
		if(!r)
			break;
    }

	unsigned long missed_bonuses_to_replace=bonuses.single_move();
    for (;missed_bonuses_to_replace>0; --missed_bonuses_to_replace)
    {
		bool r=add_bonus(true);
		if(progress==TEST)
		{
			if(r)
				std::cout << "Added missed bonus" << std::endl;
			else
				std::cout << "No room to add missed bonus" << std::endl;
		}
		if(!r)
		{
			bonuses_to_replace+=missed_bonuses_to_replace;
			break;
		}
    }

    WormSet dead_worms; /* returned by do_parallel_worm_work*/
    WormWarpSet worm_warps; /* returned by do_parallel_worm_work*/

	if(progress==TEST)
		std::cout << "do_parallel_worm_work" << std::endl;
		
    std::forward_list<std::thread> moving_worms;
    for(Worm &worm : worms)
    {
    	std::thread t(Worm::do_parallel_worm_work,
    		std::ref(worm), std::ref(board), std::ref(worms), std::ref(warps), std::ref(bonuses), progress==TEST,
    		std::ref(dead_worms), std::ref(worm_warps));
    	moving_worms.push_front(std::move(t));
    }
	if(progress==TEST)
		std::cout << "wait for parallel worms" << std::endl;
		
    /* wait until all thread have compleated */
    for(auto &t : moving_worms)
    {
    	if(t.joinable())
	    	t.join();
	}
	if(progress==TEST)
		std::cout << "parallel worm work finished" << std::endl;

	if(!dead_worms.is_empty())
		play_sound("crash");

    /* move worms */
    for(Worm &worm : worms)
    {
        if (worm.is_still() || worm.get_positions().is_empty() || dead_worms.contains(worm))
        {
        	if(progress==TEST)
        	{
        		std::cout << "Worm " << (unsigned long)worm.get_colour() << " ";
        		if(worm.is_still())
        			std::cout << "is still, ";
        		if(worm.get_positions().is_empty())
        			std::cout << "has no length, ";
        		if(dead_worms.contains(worm))
        			std::cout << "has died";
        		std::cout << std::endl;
        	}
            continue;
        }

        Position n=worm.move1(board);
        if(progress==TEST)
        {
        	std::cout << "Worm " << (unsigned long)worm.get_colour() << (worm.is_materialized()?"":"(dematerialized)") <<
        		" moves to " << (unsigned long)n.x << "," << (unsigned long)n.y << std::endl;
        }
        Position target_position;
        bool warp_bonus;
        if(worm_warps.find(worm, target_position, warp_bonus))
        {
            worm.move2(board, bonuses, target_position);
            if(warp_bonus)
            {
                worm.add_score((worm.get_length() * level) / 2);
                play_sound("bonus");
            }
        }
        else
            worm.move2(board, bonuses);
    }

	/* kill worms on heads collision */
    for(Worm &worm : worms)
	{
        for(Worm &other_worm : worms)
        {
            if (&worm != &other_worm
             && !other_worm.is_still()
             && other_worm.get_length()>0
             && worm.get_positions().get_head() == other_worm.get_positions().get_head())
            {
                dead_worms.add(worm);
                dead_worms.add(other_worm);
		        if(progress==TEST)
	        		std::cout << "Worm " << (unsigned long)worm.get_colour() << "(" << (unsigned long)worm.get_positions().get_head().x <<
	        			"," << (unsigned long)worm.get_positions().get_head().y << ") and " << (unsigned long)other_worm.get_colour() <<
	        			"(" << (unsigned long)other_worm.get_positions().get_head().x << "," << (unsigned long)other_worm.get_positions().get_head().y <<
	        			") have had a head on collision" << std::endl;
            }
        }
	}

    auto real_bonuses_to_replace=bonuses.do_pending_removes();
    for(;real_bonuses_to_replace>0;real_bonuses_to_replace--)
    {
    	bool r=add_bonus(true);
		if(progress==TEST)
		{
			if(r)
				std::cout << "Added replacement bonus" << std::endl;
			else
				std::cout << "No room to add replacement bonus" << std::endl;
		}
		if(!r)
		{
			bonuses_to_replace+=real_bonuses_to_replace;
			break;
		}
    }

    /* remove dead worms */
    for(Worm *worm : dead_worms)
    {
        if (two_or_more_worms())
        	worm->reduce_score_by_percentage(70);

        if (worm->has_lives())
            worm->reset (board, bonuses, level == 25 ? 9 : 3);
            
        life_change(worm->get_colour(), worm->get_lives());
    }

	/* inform the view a score has changed */
    for(Worm &worm : worms)
    {
    	if(worm.do_score_change())
			score_change(worm.get_colour(), worm.get_score());
    }

    /* refresh the screen */
    //redraw (true);
}

void Game::print_board() const
{
	Position p;
	for(p.y=0;p.y<board[0].size();p.y++)
	{
		for(p.x=0;p.x<board.size();p.x++)
		{
			bool done=false;
			/* worm */
			for(const Worm &worm : worms)
			{
				if(worm.get_positions().contains(p))
				{
					std::cout << (char)(worm.get_colour()+'0');
					done=true;
					break;
				}
			}
			if(!done)
			{
				/* bonus */
				if(bonuses[p.x,p.y])
				{
					std::cout << "B";
					done=true;
				}
			}
			if(!done)
			{
				/* warp */
				for(const auto &warp : warps)
				{
					if(warp.second==p)
					{
						if(warp.first>='Q' && warp.first<='Z')
							std::cout << (char)warp.first;
						else if((warp.first & 0xff)>='Q' && (warp.first & 0xff)<='Z')
							std::cout << (char)(warp.first & 0xff);
						else
							std::cout << "W";
						done=true;
						break;
					}
				}
			}
			if(!done)
			{
				if(board[p.x][p.y]>='a' && board[p.x][p.y]<='l')
				{
					const char *a[]={" ","┃","━","┗","┛","┏","┓","┻","┣","┫","┳","╋"};
					std::cout << a[board[p.x][p.y]-'a'];
				}
				else
					std::cout << (char)(board[p.x][p.y]);
			}
		}
		std::cout << std::endl;
	}
}

