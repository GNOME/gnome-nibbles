/*
 * This file is part of GNOME Nibbles.
 *
 * Copyright (C) 2015 Iulian-Gabriel Radu <iulian.radu67@gmail.com>
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
#include <list>
#include <forward_list>
#include <unordered_set>
#include <bitset>
#include <functional>
#include <map>
#include <mutex>
//#include <inplace_vector>
#include <queue>

#include "definitions.h"
#include "pseudo_random.h"
#include "map.h"
#include "bonus.h"
#include "position.h"
#include "worm.h"
#include "warp.h"
#include "game.h"

void Worm::play_sound(const char *sound)
{
	game.play_sound(sound);
}

void Worm::reverse_other_worms()
{
	game.reverse_worms(this);
}

const std::forward_list<const Worm*> Worm::get_other_worms(Worm *pSelf)
{
	std::forward_list<const Worm*> worms;
	for(const Worm &worm : game.get_worms())
	{
		if(&worm!=pSelf)
			worms.push_front(&worm);
	}
	return worms;
}

void Worm::do_wall_and_warps(
		const std::vector<std::vector<unsigned char>> &board,
		const std::forward_list<Worm> &worms,
		const Warps &warps,
		WormSet &dead_worms, WormWarpSet &worm_warps)
{
    /* kill worms which are hitting the wall, teleport worms hitting a warp */
    if (!is_still() && !get_positions().is_empty())
    {
		auto position = get_position_after_direction_move(board);
	    auto [found, target, bonus]=warps.get_warp_target(position, *this,
								get_direction(), get_length(),
								!is_human(), worms);
		if(found)
		{
			worm_warps.add(*this, target, bonus);
		}
        else if (!can_move_to(board, worms, position))
        {
            dead_worms.add(*this);
        }
    }
}

WormDirection Worm::uturn(const std::vector<std::vector<unsigned char>> &board,
	const std::forward_list<Worm> &worms,
	WormDirection direction)
{
    /* player has reversed direction */
    Position tmp;
    WormDirection dirA,dirB;
    int length_posA,length_posB;
    length_posA=0; length_posB=0;
    if (direction==eDirection::DOWN || direction==eDirection::UP)
    {
        /* calculate space when we step to the left */
        tmp = positions.get_head();
        dirA = eDirection::LEFT;
        tmp.move(dirA,board.size(),board[0].size());
        for (length_posA=0; length_posA<board[0].size() && can_move_to (board,worms,tmp); length_posA++,tmp.move (direction, board.size(),board[0].size()));
        /* calculate space when we step to the right */
        tmp = positions.get_head();
        dirB = eDirection::RIGHT;
        tmp.move(dirB,board.size(),board[0].size());
        for (length_posB=0; length_posB<board[0].size() && can_move_to (board,worms,tmp); length_posB++,tmp.move (direction, board.size(),board[0].size()));
    }
    else /* direction==eDirection::LEFT || direction==eDirection::RIGHT */
    {
        /* calculate space when we step up */
        tmp = positions.get_head();
        dirA = eDirection::UP;
        tmp.move(dirA,board.size(),board[0].size());
        for (length_posA=0; length_posA<board.size() && can_move_to (board,worms,tmp); length_posA++,tmp.move (direction, board.size(),board[0].size()));
        /* calculate space when we step down */
        tmp = positions.get_head();
        dirB = eDirection::DOWN;
        tmp.move(dirB,board.size(),board[0].size());
        for (length_posB=0; length_posB<board.size() && can_move_to (board,worms,tmp); length_posB++,tmp.move (direction, board.size(),board[0].size()));
    }
    if (length_posA > length_posB)
    {
        LastUturnA=true;
        return dirA;
    }
    else if (length_posA < length_posB)
    {
        LastUturnA=false;
        return dirB;
    }
    else if (length_posA > 0 /*|| length_posB > 0*/)
    {
        if (LastUturnA)
            return dirA;
        else
            return dirB;
    }
    else /* length_posA==0 && length_posB==0 */
        return direction.reverse ();
}


/*\
* * AI
\*/

/* Check whether the worm will be trapped in a dead end. A location
 * within the dead end and the length of the worm is given. This
 * prevents worms getting trapped in a spiral, or in a corner sharper
 * than 90 degrees.  runnumber is a unique number used to update the
 * deadend board. The principle of the deadend board is that it marks
 * all squares previously checked, so the exact size of the deadend
 * can be calculated in O(n) time; to prevent the need to clear it
 * afterwards, a different number is stored in the board each time
 * (the number will not have been previously used, so the board will
 * appear empty).
 */

int Worm::ai_deadend(const std::vector<std::vector<unsigned char>> &board, const Map &worm_map,
	Position position, long length)
{
	const long p_max = 92*66;
	//std::inplace_vector<uint16_t, p_max> p;
	std::vector<uint16_t> p;
	p.emplace_back(position);
    for (unsigned long i = 0; i < p.size() && (p.size() - 1) < length; i++)
    {
        for (unsigned long dir = 4; dir > 0 && (p.size() - 1) < length; dir--)
        {
	        Position new_position = {p[i]>>8, p[i]&0xff};
	        new_position.move((eDirection)dir, board.size(), board[0].size());
	        if (deadend_board[new_position.x, new_position.y] != deadend_board.runnumber
	            && board[new_position.x][new_position.y]==EMPTYCHAR
	            && !worm_map.contain_position(new_position))
	        {
	            deadend_board[new_position.x, new_position.y] = deadend_board.runnumber;
	            if(p.size()<p_max)
					p.emplace_back(new_position);
	        }
        }
    }
    return p.size() - 1 > length ? 0 : length - (p.size() - 1);
}

/* Check a deadend starting from the next square in this direction,
 * rather than from this square. Also block off the squares near worm
 * heads, so that humans can't kill AI players by trapping them
 * against a wall.  The given length is quartered and squared; this
 * allows for the situation where the worm has gone round in a square
 * and is about to get trapped in a spiral. However, it's set to at
 * least BOARDWIDTH, so that on the levels with long thin paths a worm
 * won't start down the path if it'll crash at the other end.
 */
int Worm::ai_deadend_after(const std::vector<std::vector<unsigned char>> &board,
	const std::forward_list<Worm> &worms,
	const Map &worm_map,
	Position old_position, WormDirection direction, long length)
{
    uint8_t width  = (uint8_t)board.size();
    uint8_t height = (uint8_t)board[0].size();
	++deadend_board.runnumber;

	for(const Worm &worm : worms)
	{
	    if (!worm.is_still() && !worm.positions.is_empty())
	    {
	        auto target_x = worm.positions.get_head().x;
	        auto target_y = worm.positions.get_head().y;
	        if (target_x == old_position.x
	         && target_y == old_position.y)
	            continue;

	        if (target_x > 0)           deadend_board [target_x - 1, target_y    ] = deadend_board.runnumber;
	        else                        deadend_board [width    - 1, target_y    ] = deadend_board.runnumber;
	        if (target_y > 0)           deadend_board [target_x    , target_y - 1] = deadend_board.runnumber;
	        else                        deadend_board [target_x    , height   - 1] = deadend_board.runnumber;
	        if (target_x < width - 1)   deadend_board [target_x + 1, target_y    ] = deadend_board.runnumber;
	        else                        deadend_board [0           , target_y    ] = deadend_board.runnumber;
	        if (target_y < height - 1)  deadend_board [target_x    , target_y + 1] = deadend_board.runnumber;
	        else                        deadend_board [target_x    , 0           ] = deadend_board.runnumber;
	    }
	}
	
	Position new_position = old_position;
	new_position.move (direction, width, height);
	deadend_board [old_position.x, old_position.y] = deadend_board.runnumber;
	deadend_board [new_position.x, new_position.y] = deadend_board.runnumber;

	auto cl = (length * length) / 16;
	if (cl < width)
	    cl = width;
	    
	return ai_deadend(board, worm_map, new_position, cl);

}

/* Check to see if another worm's head is too close in front of us;
 * that is, that it's within 3 in the direction we're going and within
 * 1 to the side.
 */
bool Worm::ai_too_close (const std::forward_list<Worm> &worms, WormDirection direction)
{
    for(const Worm &worm : worms)
    {
        if (&worm == this || worm.is_still() || worm.positions.is_empty())
            continue;

        auto dx = (long)positions.get_head().x - (long)worm.positions.get_head().x;
        auto dy = (long)positions.get_head().y - (long)worm.positions.get_head().y;
        switch (direction)
        {
            case eDirection::UP:
                if (dy > 0 && dy <= 3 && dx >= -1 && dx <= 1)
                    return true;
                break;

            case eDirection::DOWN:
                if (dy < 0 && dy >= -3 && dx >= -1 && dx <= 1)
                    return true;
                break;

            case eDirection::LEFT:
                if (dx > 0 && dx <= 3 && dy >= -1 && dy <= 1)
                    return true;
                break;

            case eDirection::RIGHT:
                if (dx < 0 && dx >= -3 && dy >= -1 && dy <= 1)
                    return true;
                break;

            default:
                assert(false);
        }
    }
    return false;
}

bool Worm::ai_can_see_bonus(
	const std::vector<std::vector<unsigned char>> &board,
	const Position origin,
	const Bonus &bonus,
	const WormDirection direction)
{
    if (origin.x == bonus.x && origin.y == bonus.y
        || origin.x == bonus.x + 1 && origin.y == bonus.y
        || origin.x == bonus.x && origin.y == bonus.y + 1
        || origin.x == bonus.x + 1 && origin.y == bonus.y + 1)
        return true;
    Slice slice;
    /* our initial view is set by our direction */
    slice.set_direction_view(direction, board);
    /* narrow our view to this bonus (or set an empty view if the bonus is not within our view) */
    slice.intersection_by_position (origin, bonus.x, bonus.y, 2 /* always 2 for bonus */);
    return !slice.is_empty ();
}
std::pair<long, Bonus::eType> Worm::ai_count_distance_to_a_bonus_in_direction(
	const std::vector<std::vector<unsigned char>> &board,
	const Map &worm_map,
	const Position origin, const WormDirection direction,
	const Bonuses &bonuses)
{
    /*
     * Return the distance to a bonus if it is possible to head in
     * this direction to find a bonus. Otherwise return std::numeric_limits<long>::max().
     */

    /*
     * Look for a bonus in direction.
     *
     *       .
     *     . .
     *   . . .
     * o . . .
     *   . b b
     *     b b
     *       .
     *
     */

    long bonus_distance = std::numeric_limits<long>::max();
    Bonus::eType bonus_type = (Bonus::eType)(-1);

    Slice slice;
    for (Bonus b : bonuses)
    {
        if (bonus_type == Bonus::LIFE && b.type == Bonus::LIFE ||
            bonus_type != Bonus::LIFE && (
                b.type == Bonus::REGULAR || b.type == Bonus::DOUBLE
                || b.type == Bonus::LIFE || b.type == Bonus::REVERSE))
        {
            /* our initial view is set by our direction */
            slice.set_direction_view (direction, board);
            /* narrow our view to this bonus (or set an empty view if the bonus is not within our view) */
            slice.intersection_by_position (origin, b.x, b.y, 2 /* always 2 for bonus */);
            if (!slice.is_empty ())
            {
                /* we have found a bonus within our field of view, check that nothing is in the way */
                long distance = slice.is_visible(origin, board, worm_map, b);
                /* If the bonus is visible, its nearer than previous bonuses and it can still be see
                   if we move in this direction choose it. */
                if (distance < bonus_distance &&
                    ai_can_see_bonus (board, get_position_after_direction_move(board, origin, direction), b, direction))
                {
                    bonus_distance = distance;
                    bonus_type = b;
                }
            }
        }
    }

    return {bonus_distance, bonus_type};
}

/* Determines the direction of the AI worm. */
void Worm::ai_move (
	const std::vector<std::vector<unsigned char>> &board,
	const std::forward_list<Worm> &worms,
	const Bonuses &bonuses,
	bool test_logging)
{
    Map worm_map(worms, board.size(), board[0].size());

    /* We have a look in all directions except behind us for a bonus. */
	long shortest_distance = std::numeric_limits<long>::max();
    auto shortest_dir = direction;
    Bonus::eType shortest_bonus_type = (Bonus::eType)(-1);

    WormDirection dir[] = {direction, direction.turn_left(), direction.turn_right()};
	for (WormDirection direction : dir)
	{
	    auto [d, bonus_type] = ai_count_distance_to_a_bonus_in_direction(board, worm_map, positions.get_head(), direction, bonuses);
	    if (ai_is_bonus_more_attractive(bonus_type, d, shortest_bonus_type, shortest_distance)
	        && can_move_direction(board, worms, direction))
	    {
	        shortest_distance = d;
	        shortest_dir = direction;
	        shortest_bonus_type = bonus_type;
	    }
	}

	if (shortest_distance >= std::numeric_limits<long>::max())
	{
	    // check next step positions
	    WormDirection start_direction[] = {direction, direction, direction.turn_right(), direction.turn_left()};
	    WormDirection look_direction[]  = {direction.turn_left(), direction.turn_right(), direction, direction};
	    for (int i = 0; i < 4; i++)
	    {
	        auto [d, bonus_type] = ai_count_distance_to_a_bonus_in_direction(board, worm_map,
	            get_position_after_direction_move (board, positions.get_head(), start_direction[i]), look_direction[i], bonuses);
	        if (ai_is_bonus_more_attractive(bonus_type, d, shortest_bonus_type, shortest_distance))
	        {
	            shortest_distance = d + 1; /* +1 for the step we have already taken in our logic */
	            shortest_dir = start_direction[i];
	            shortest_bonus_type = bonus_type;
	        }
	    }
	}

	if (shortest_distance >= std::numeric_limits<long>::max())
	{
	    // no bonus is visible, one in thirty chance of turning left or right
	    switch(pseudo_random(0, 60))
	    {
	    	case 0:
	    		shortest_dir = direction.turn_right();
	    		break;
	    	case 1:
	    		shortest_dir = direction.turn_left();
	    		break;
	    	default:
		    	break;
	    }
	}

    /* Avoid walls, dead-ends and other worm's heads. This is done using
     * an evaluation function which is CAPACITY for a wall, 4 if another
     * worm's head is in the too close area, 4 if another worm's head
     * could move to the same location as ours, plus 0 if there's no
     * dead-end, or the amount that doesn't fit for a deadend. olddir's
     * score is reduced by 100, to favour it, but only if its score is 0
     * otherwise; this is so that if we're currently trapped in a dead
     * end, the worm will move in a space-filling manner in the hope
     * that the dead end will disappear (e.g. if it's made from the tail
     * of some worm, as often happens).
     */
    WormDirection bonus_dir = shortest_dir;
    WormDirection best_dir = NONE;
    long best_yet = std::numeric_limits<long>::max();
    std::string logging;
    if(test_logging)
    	logging = std::format("Worm {} direction; ", (unsigned int)get_colour());
	for (WormDirection direction : dir[0].get_space_fill_array())
	{
	    int this_len = 0;
		/* if we are heading for a LIFE bonus don't worry about being trapped */
		if (!(direction == bonus_dir && shortest_bonus_type == Bonus::LIFE))
		{
#if TEST_COMPILE
			assert (can_move_to(board, worms, get_position_after_direction_move (board, positions.get_head(), direction)) ==
			    can_move_to_map (board, worm_map, get_position_after_direction_move (board, positions.get_head(), direction)));
#endif
			if (!can_move_to_map(board, worm_map, get_position_after_direction_move (board, positions.get_head(), direction)))
			{
			    this_len += capacity;
				logging+="*";
			}
			else
				logging+=".";

			if (ai_too_close(worms, direction))
			    this_len += 4;

		    this_len += ai_deadend_after(board, worms, worm_map, positions.get_head(), direction, target_length);
		}
		if (direction == bonus_dir && this_len <= 0)
		    this_len -= 100;

	    /* If the favoured direction isn't appropriate, then choose
	     * another direction at random rather than favouring one in
	     * particular, to stop the worms bunching in the bottom-
	     * right corner of the board.
	     */
		if (this_len <= 0)
		    this_len -= pseudo_random(0, 100);
		if(test_logging)
			logging+=std::format("{}:{} ", direction==eDirection::EAST?"EAST":(direction==eDirection::WEST?"WEST":(direction==eDirection::NORTH?"NORTH":(direction==eDirection::SOUTH?"SOUTH":("NONE")))), this_len);
		if (this_len < best_yet)
		{
		    best_yet = this_len;
		    best_dir = direction;
		}
	}
	if(test_logging)
	{
		logging+=std::format("choose {}\n", best_dir==eDirection::EAST?"EAST":(best_dir==eDirection::WEST?"WEST":(best_dir==eDirection::NORTH?"NORTH":(best_dir==eDirection::SOUTH?"SOUTH":("NONE")))));
		std::cout << logging;
	}

    // set the class variable direction to our desired direction */
    direction = best_dir;
}

