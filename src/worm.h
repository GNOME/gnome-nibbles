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

#if defined(CAN_USE_int128_t)
	#include <cstdint>
	typedef int128_t int128;
#elif defined(CAN_USE___int128)
	typedef __int128 int128;
#else
	#define use_internal_int128
#endif

#if !defined(EMPTYCHAR)
	#define EMPTYCHAR 'a'
#endif

struct SignedPosition
{
    int64_t x; /* x increases going right (or east) */
    int64_t y; /* y increases going down (or south) */
    /* used by wrap functions */
    int64_t x_max=0;
    int64_t y_max=0;
    
    uint8_t wrap_x() const
    {
        assert (x_max > 0 && x_max<=92); /* call set_wrapping (x, y) first */
        if (x >= x_max)
            return x % x_max;
        else if (x < 0)
            return ((x % x_max) + x_max) % x_max;
        else
            return (uint8_t)x;
    }

    uint8_t wrap_y() const
    {
        assert (y_max > 0 && y_max<=66); /* call set_wrapping (x, y) first */
        if (y >= y_max)
            return y % y_max;
        else if (y < 0)
      		return ((y % y_max) + y_max) % y_max;
        else
            return (uint8_t)y;
    }

    uint16_t wrap_xy () const
    {
        return ((uint16_t)wrap_x() << 8) | wrap_y ();
    }
};

/* an enumerated type that represents one quarter of the board */
enum Quarter {Q0,Q1,Q2,Q3};

/*
 * Structure to store any angle. e.g 45° (PI / 4) or 2½° (PI / 72).
 * The angle is stored as a ratio of x (opposite) over y (adjacent)
 * so that we can use integers in all our operations.
 * To convert to degrees use (std::atan2((x,-y) / std::numbers::pi * 180).
 *
 */
class Angle
{
	#if defined(use_internal_int128)
	/* A cut down 128 bit integer with only a subset of operators. */
	struct int128
	{
		uint64_t low;
		int64_t hi;
	public:
		explicit int128(int i)
		{
		    low = (uint64_t)i;
		    hi = (i < 0) ? -1 : 0;
		}
		int128(int64_t i = 0)
		{
		    low = (uint64_t)i;
		    hi = (i < 0) ? -1 : 0;
		}
		int128 operator+=(const int128& rhs)
		{
			uint64_t new_low = low + rhs.low;
			int64_t carry = (new_low < low);  // overflow from low 64 bits
			hi = hi + rhs.hi + carry;
			low = new_low;
			return *this;
		}
		int128 operator+(const int128& rhs) const
		{
		    int128 result=*this;
		    result+=rhs;
		    return result;
		}    
		int128 operator-=(const int128& rhs)
		{
			hi -= rhs.hi + (rhs.low > low);
			low -= rhs.low;
			return *this;
		}
		int128 operator-(const int128& rhs) const
		{
		    int128 result=*this;
		    result-=rhs;
		    return result;
		}    
		int128 operator-() const
		{
		    return int128(0) - *this;
		}
		int128 operator<<=(int shift)
		{
		    if (shift >= 64)
		    {
		        hi = low;
		        low = 0;
		        shift -= 64;
		    }
		    if (shift > 0)
		    {
		        hi <<= shift;
		        hi |= low >> (64 - shift);
		        low <<= shift;
		    }
		    return *this;
		}
		int128 operator<<(int shift) const
		{
			int128 result=*this;
			result<<=shift;
			return result;
		}
		int128 operator<<(long shift) const
		{
			int128 result=*this;
			result<<=shift;
			return result;
		}
		int128 operator>>=(int shift)
		{
		    if (shift >= 64)
		    {
		        low = hi;
		        hi = 0;
		        shift -= 64;
		    }
		    if (shift > 0)
		    {
		        low >>= shift;
		        low |= hi << (64 - shift);
		        hi >>= shift;
		    }
		    return *this;
		}
		int128 operator>>(int shift) const
		{
			int128 result=*this;
			result>>=shift;
			return result;
		}
		int128 operator>>(long shift) const
		{
			int128 result=*this;
			result>>=shift;
			return result;
		}
		bool operator>(const int128& rhs) const
		{
			if(hi!=rhs.hi)
				return hi>rhs.hi;
			else
				return low>rhs.low;
		}
		bool operator>(int rhs) const
		{
			return *this > int128(rhs);
		}
		bool operator<=(const int128& rhs) const
		{
			return !(*this>rhs);
		}
		bool operator<=(int rhs) const
		{
			return !(*this>int128(rhs));
		}
		bool operator<(const int128& rhs) const
		{
			if(hi!=rhs.hi)
				return hi<rhs.hi;
			else
				return low<rhs.low;
		}
		bool operator<(int rhs) const
		{
			return *this<int128(rhs);
		}
		bool operator>=(const int128& rhs) const
		{
			return !(*this<rhs);
		}
		bool operator>=(int rhs) const
		{
			return *this>=int128(rhs);
		}
		bool operator==(const int128& rhs) const
		{
			return hi==rhs.hi && low==rhs.low;
		}
		int128 operator/(const int128 &rhs) const
		{
			assert(!(rhs.hi==0 && rhs.low==0));

			// Handle sign
			bool neg = (hi < 0) ^ (rhs.hi < 0);

			int128 a = *this;
			int128 b = rhs;

			if(a.hi < 0)
				a = -a;
			if(b.hi < 0)
				b = -b;

			int128 quotient = 0;
			int128 current = 0;

			for(long i = 127; i >= 0; --i)
			{
				current <<= 1;
				if(((a >> i) & 1) > 0)
				    current.low |= 1;
				if(current >= b)
				{
				    current -= b;
				    quotient += int128(1) << i;
				}
			}

			return neg ? -quotient : quotient;
		}
		int128 operator*(const int128 &rhs) const
		{
			bool neg = (hi < 0) ^ (rhs.hi < 0);

			int128 a = *this;
			int128 b = rhs;

			if (a.hi < 0)
				a = -a;
			if (b.hi < 0)
				b = -b;

			int128 result = 0;
			int128 base = a;

			for(long i = 0; i < 128; i++)
			{
				if((b & 1) > 0)
				    result+= base;
				base <<= 1;
				b >>= 1;
			}

			return neg ? -result : result;
		}	
		int128 operator&=(const int128 &rhs)
		{
			hi &= rhs.hi;
			low &= rhs.low;
			return *this;
		}
		int128 operator&(const int128 &rhs) const
		{
			int128 result=*this;
			result &= rhs;
			return result;
		}
		int128 operator&(int rhs) const
		{
			int128 result=*this;
			result &= rhs;
			return result;
		}
		operator int64_t() const
		{
			return low;
		}
	};
	#endif

public:
    /* variables */
    int128 x; /* x increases going right (or east) */
    int128 y; /* y increases going down (or south) */
private:
    /* variables used by @get */
    static const int step_multiplier_2n = 38;
    static const int64_t step_multiplier = 274877906944; /* 2 to the power of 38 */
    int64_t origin_x;
    int64_t origin_y;
public:
    /* variables used by wrap functions in struct SignedPosition */
    int64_t x_max; /* one more that the max */
    int64_t y_max; /* one more that the max */

private:
	bool _set;

public:
    /* public functions */
	Angle(int64_t x, int64_t y, int64_t x_max=0, int64_t y_max=0) : x(x), y(y), x_max(x_max), y_max(y_max), _set(true)
	{
	}

	Angle() : x(0), y(0), x_max(0), y_max(0), _set(false)
	{
	}

	Angle(const Angle &o) : x(o.x), y(o.y), origin_x(o.origin_x), origin_y(o.origin_y), x_max(o.x_max), y_max(o.y_max), _set(o._set)
	{
	}
/*
	Angle(std::initializer_list<int64_t> minmax) :
		x(*(minmax.begin())),
		y(*(minmax.begin()+1)),
		x_max(*(minmax.begin()+2)),
		y_max(*(minmax.begin()+3)), _set(true)
	{
		assert(minmax.size()==4);
	}
*/
	void set(const Angle &o)
	{
		x=o.x;
		y=o.y;
		_set=true;
	}

	bool is_set() const
	{
		return _set;
	}
/*
	const Angle &operator=(const Angle &o)
	{
        x = o.x;
        y = o.y;
        origin_x = o.origin_x;
        origin_y = o.origin_y;
        x_max = o.x_max;
        y_max = o.y_max;
        return *this;
	}
*/
	bool operator<=(const Angle &o) const
    {
        return !(*this > o);
    }

	bool operator>(const Angle &o) const
    {
        Quarter q=o.get_quarter();
        switch (get_quarter ())
        {
            case Q0:
                if (q == Q3)
                    return true;
                else if (q == Q1)
                    return false;
                else
                    return x*o.y < o.x*y;
            case Q1:
                if (q == Q0)
                    return true;
                else if (q == Q2)
                    return false;
                else
                    return x*o.y < o.x*y;
            case Q2:
                if (q == Q1)
                    return true;
                else if (q == Q3)
                    return false;
                else
                    return x*o.y < o.x*y;
            case Q3:
                if (q == Q2)
                    return true;
                else if (q == Q0)
                    return false;
                else
                    return x*o.y < o.x*y;
            default:
                return false;
        }
    }

    bool operator<(const Angle &o) const
    {
        return !(*this==o) && *this<=o;
    }

	bool operator>=(const Angle &o) const
    {
        Quarter q=o.get_quarter ();
        switch (get_quarter ())
        {
            case Q0:
                if (q == Q3)
                    return true;
                else if (q == Q1)
                    return false;
                else if (q == Q2)
                    return x*o.y < o.x*y;
                else
                    return x*o.y <= o.x*y;
            case Q1:
                if (q == Q0)
                    return true;
                else if (q == Q2)
                    return false;
                else if (q == Q3)
                    return x*o.y < o.x*y;
                else
                    return x*o.y <= o.x*y;
            case Q2:
                if (q == Q1)
                    return true;
                else if (q == Q3)
                    return false;
                else if (q == Q0)
                    return x*o.y < o.x*y;
                else
                    return x*o.y <= o.x*y;
            case Q3:
                if (q == Q2)
                    return true;
                else if (q == Q0)
                    return false;
                else if (q == Q1)
                    return x*o.y < o.x*y;
                else
                    return x*o.y <= o.x*y;
            default:
                return false;
        }
    }

    bool operator==(const Angle &o) const
    {
        return (get_quarter () == o.get_quarter ())
            && x*o.y == o.x*y;
    }

    void set_origin (Position origin)
    {
        origin_x = (origin.x * 2 + 1) * (step_multiplier / 2);
        origin_y = (origin.y * 2 + 1) * (step_multiplier / 2);
    }

    void set_wrapping (int x, int y)
    {
        x_max = x;
        y_max = y;
    }

    bool step_along_x() const
    {
        return (x < 0 ? -x : x) > (y < 0 ? -y : y);//std::abs(x) > std::abs(y)
    }

    /* get the i position along the angle */
    SignedPosition get(uint64_t i) const
    {
        /* step along x or y axis depending on which is the larger */
        int64_t delta_x = step_along_x() ?
        	set_delta_x_sign (step_multiplier):
            set_delta_x_sign ( (x << step_multiplier_2n) / y);
        int64_t delta_y = step_along_x() ?
        	set_delta_y_sign ( (y << step_multiplier_2n) / x):
            set_delta_y_sign (step_multiplier);

        /* calculate new x,y position */
        int64_t x_i = origin_x + delta_x * (int64_t)i;
        int64_t y_i = origin_y + delta_y * (int64_t)i;

        return { (x_i >> step_multiplier_2n),
            (y_i >> step_multiplier_2n), x_max, y_max};
    }

	SignedPosition begin() const
	{
		return get(0);
	}

	SignedPosition end() const
	{
		return get(step_along_x() ? x_max : y_max);
	}

    /* private functions */
private:
    /* -x                     +x
     *             |              -y
     *  quarter 3  |  quarter 0
     *             |              
     * ------------+-------------
     *             |
     *  quarter 2  |  quarter 1
     *             |              +y
     *
     * + is a x=0,y=0
     */
    Quarter get_quarter () const
    {
        if (x >= 0 && y < 0)
            return Q0;
        else if (x > 0 && y >= 0)
            return Q1;
        if (x <= 0 && y > 0)
            return Q2;
        else
            return Q3;
    }

    /* set the parameter to the same sign as x */
    int64_t set_delta_x_sign (const int128 &_x) const
    {
    	if(x<0 ^ _x<0)
    		return -(int64_t)_x;
    	else
    		return (int64_t)_x;
    }

    /* set the parameter to the same sign as y */
    int64_t set_delta_y_sign (const int128 &_y) const
    {
    	if(y<0 ^ _y<0)
    		return -(int64_t)_y;
    	else
    		return (int64_t)_y;
    }
};

class WormPositions
{
public:
    void append_position (Position p)
    {
        list.push_back(((uint16_t)p.x) << 8 | p.y);
    }
    Position get_head() const
    {
        auto head=list.front();
        return {(uint8_t)(head >> 8), (uint8_t)head};
    }
    Position get_head_adjacent() const
    {
    	auto iterator=list.begin();
    	std::advance(iterator, 1);
        return {(uint8_t)(*iterator >> 8), (uint8_t)*iterator};
    }
    void set_head(Position p)
    {
    	list.front()=((uint16_t)p.x) << 8 | p.y;
    }
    void prepend_position(Position p)
    {
    	list.push_front(((uint16_t)p.x) << 8 | p.y);
    }
    uint16_t remove_tail()
    {
    	uint16_t r=list.back();
        list.pop_back();
        return r;
    }
	auto begin() const
	{
		return list.cbegin();
	}
	auto end() const
	{
		return list.cend();
	}
	bool is_empty() const
	{
		return list.empty();
	}
	void reverse()
	{
		list.reverse();
	}
	auto get_length() const
	{
		return list.size();
	}
	bool contains(Position position) const
	{
		for(const auto &p : list)
		{
			if(p==position)
				return true;
		}
		return false;
	}
	void clear()
	{
		list.clear();
	}
private:
	std::list<uint16_t> list;
};

class Warps;/* forward reference */
class Game;/* forward reference */
class WormSet;/* forward reference */
class WormWarpSet;/* forward reference */

class Worm
{
public:
	/*
	 * An array to store the positions of worms.
	 * The contains function is the equivalent of the function
	 * is_position_clear_of_materialized_worms but much faster.
	 */
	class Map : SimpleMap
	{
	public:
		/* constructor */
		Map(const std::forward_list<Worm> &worms, uint8_t map_width, uint8_t map_height) :
			SimpleMap(map_width, map_height)
		{
		    add(worms);
		}
		Map(const std::forward_list<const Worm*> &worms, uint8_t map_width, uint8_t map_height) :
			SimpleMap(map_width, map_height)
		{
		    add(worms);
		}
		bool contain(uint16_t p) const
		{
		    return test(p);
		}
		bool contain_position(Position p) const
		{
		    return contain(((uint16_t)p.x) << 8 | p.y);
		}
		bool contain_position(uint8_t x, uint8_t y) const
		{
		    return contain(((uint16_t)x) << 8 | y);
		}
		bool contains(WormPositions positions)
		{
			for(const auto &p : positions)
		        if (contain (p))
		            return true;
		    return false;
		}
		/* private functions */
	private:
		void add(const std::forward_list<Worm> &worms)
		{
		    for(const auto &worm : worms)
		    {
		        if (worm.is_materialized())
		        {
		            for(const auto &p : worm.get_positions())
		            {
		            	set(p);
		            }
		        }
		    }
		}
		void add(const std::forward_list<const Worm*> &worms)
		{
		    for(const auto &worm : worms)
		    {
		        if (worm->is_materialized())
		        {
		            for(const auto &p : worm->get_positions())
		            {
		            	set(p);
		            }
		        }
		    }
		}
	};
	void do_wall_and_warps(
		const std::vector<std::vector<unsigned char>> &board,
		const std::forward_list<Worm> &worms,
		const Warps &warps,
		WormSet &dead_worms, WormWarpSet &worm_warps);

private:
	Game &game;
	const unsigned long current_level;
	const bool human;
	const eWormColour colour;
	const unsigned long capacity;
	Start start;
	/*
	 * A queue that allows no adjacent duplicates.
	 */
	class DeadEndBoard
	{
	private:
		const unsigned long width,height;
		std::vector<unsigned long> board;
	public:
		unsigned long runnumber;
		DeadEndBoard(unsigned long width, unsigned long height) : width(width), height(height)
		{
			runnumber=0;
			board.resize(height*width, runnumber/*initial value*/);
		}
		unsigned long &operator[](unsigned long x, unsigned long y)
		{
			return board[y*width+x];
		}
		unsigned long get_width() {return width;}
		unsigned long get_height() {return height;}
	} deadend_board;
	class DirectionQueue
	{
	private:
		mutable std::mutex mtx;
		std::queue<eDirection> q;
	public:
		DirectionQueue() noexcept = default;
		~DirectionQueue() = default;
		
		void add(eDirection direction)
		{
			std::lock_guard<std::mutex> lock(mtx);
			if(q.empty() || q.back()!=direction)
				q.emplace(direction);
		}
		std::pair<bool, eDirection> remove()
		{
			std::lock_guard<std::mutex> lock(mtx);
			if (q.empty())
				return {false, {}}; // Return false if queue is empty (avoids garbage value)
				
			eDirection front_element = q.front();
			q.pop();
			return {true, front_element};
		}
		void prepend(eDirection direction)
		{
			std::lock_guard<std::mutex> lock(mtx);
			if(q.empty())
				q.emplace(direction);
			else if(q.front()!=direction)
			{
				std::queue<eDirection> swap;
				swap.emplace(direction);
				for(;!q.empty();)
				{
					eDirection d = q.front();
					q.pop();
					swap.emplace(d);
				}
				q.swap(swap);
			}
		}
	} direction_queue;
public:
	Worm(Game &game, unsigned long current_level, bool human, eWormColour colour,
		unsigned long width, unsigned long height) :
		game(game), current_level(current_level), human(human), colour(colour),
		capacity(height*width), start(start), deadend_board(width,height)
	{
		positions.append_position(start.position);
		direction = start.direction;
		rounds_to_stay_dematerialized=1;
		lives=6;
		score=0;
		score_changed=true;
	}
	~Worm() = default;
	Worm(const Worm &copy) :
		game(copy.game), current_level(copy.current_level), human(copy.human), colour(copy.colour),
		capacity(copy.capacity), deadend_board(copy.deadend_board)
	{
		positions = copy.positions;
		direction = copy.direction;
		rounds_to_stay_dematerialized=copy.rounds_to_stay_dematerialized;
		lives=copy.lives;
		score=copy.score;
		score_changed=true;
	}
	bool is_materialized() const
	{
		return rounds_to_stay_dematerialized==0;
	}
	bool is_still() const
	{
		return rounds_to_stay_still>0;
	}
	const WormPositions &get_positions() const
	{
		return positions;
	}
	void spawn(const std::vector<std::vector<unsigned char>> &board, Bonuses &bonuses, bool force_materialize=false)
	{
		bonus_eaten.clear(); /* forget all the bonuses we have eaten */
		positions.clear();
		positions.append_position(start.position);
	    direction = start.direction;

		const unsigned long STARTING_LENGTH=5;
		if(!positions.is_empty())
		{
			rounds_to_stay_still=0;
			rounds_to_stay_dematerialized=STARTING_LENGTH;
			target_length=STARTING_LENGTH;
			for(;positions.get_length()<target_length;)
			{
				move1(board);
				move2(board, bonuses, force_materialize);
			}
		}
	}
	void spawn(const Start &_start, const std::vector<std::vector<unsigned char>> &board, Bonuses &bonuses, bool force_materialize=false)
	{
		start=_start;
		spawn(board,bonuses,force_materialize);
	}
    void reverse()
    {
        if (!is_still() && !positions.is_empty())
        {
            positions.reverse();

            auto head=positions.get_head();
            auto adjacent=positions.get_head_adjacent();
            if (head.y==adjacent.y)
                direction = (head.x > adjacent.x) ? eDirection::RIGHT : eDirection::LEFT;
            else
                direction = (head.y > adjacent.y) ? eDirection::DOWN : eDirection::UP;
        }
    }
    bool decrement_still()
    {
        if (rounds_to_stay_still > 0)
        {
            --rounds_to_stay_still;
            return true;
        }
        else
        	return false;
    }
    unsigned long get_rounds_to_stay_still() const
    {
    	return rounds_to_stay_still;
    }
	bool decrement_score()
    {
        if (score > 0)
        {
	        --score;
	        score_changed=true;
	        return true;
	    }
        else
        	return false;
    }

	int ai_deadend (const std::vector<std::vector<unsigned char>> &board,
		const Map &worm_map,
		Position position, long length);

	int ai_deadend_after (const std::vector<std::vector<unsigned char>> &board,
		const std::forward_list<Worm> &worms,
		const Map &worm_map,
		Position old_position, WormDirection direction, long length);

	bool ai_too_close (const std::forward_list<Worm> &worms, WormDirection direction);

	bool ai_is_bonus_more_attractive(Bonus::eType b0, long d0, Bonus::eType b1, long d1)
	{
		/* A LIFE bonus is a more attractive bonus than any other bonus. */
		return (b0 == Bonus::LIFE && b1 == Bonus::LIFE
		    || b0 != Bonus::LIFE && b1 != Bonus::LIFE) && d0 < d1
		    || b0 == Bonus::LIFE && b1 != Bonus::LIFE;
	}

	bool ai_can_see_bonus(
		const std::vector<std::vector<unsigned char>> &board,
		const Position origin,
		const Bonus &bonus,
		const WormDirection direction);

	std::pair<long, Bonus::eType> ai_count_distance_to_a_bonus_in_direction(
		const std::vector<std::vector<unsigned char>> &board,
		const Map &worm_map,
		const Position origin, const WormDirection direction,
		const Bonuses &bonuses);

	void ai_move(
		const std::vector<std::vector<unsigned char>> &board,
		const std::forward_list<Worm> &worms,
		const Bonuses &bonuses,
		bool test_logging);

	void queue_direction(eDirection direction)
	{
		direction_queue.add(direction);
	}

    WormDirection uturn(const std::vector<std::vector<unsigned char>> &board,
    	const std::forward_list<Worm> &worms,
    	WormDirection direction);

    void direction_change(
		const std::vector<std::vector<unsigned char>> &board,
		const std::forward_list<Worm> &worms,
		const Bonuses &bonuses,
		bool test_logging)
    {
        if (!is_still() && !positions.is_empty())
       	{
            if (human)
            {
            	auto [b, dir]=direction_queue.remove();
		        if (b)
		        {
		            switch(dir)
		            {
		            	case eDirection::UP:
						    if (direction == DOWN)
						    {
						        direction = uturn(board,worms,UP);
						        if (direction != UP && direction != DOWN)
						            direction_queue.prepend(UP);
						    }
						    else if (can_move_direction(board,worms,UP))
						        direction = UP;
			            	break;
		            	case eDirection::DOWN:
						    if (direction == UP)
						    {
						        direction = uturn(board,worms,DOWN);
						        if (direction != DOWN && direction != UP)
						            direction_queue.prepend (DOWN);
						    }
						    else if (can_move_direction(board,worms,DOWN))
						        direction = DOWN;
			            	break;
		            	case eDirection::LEFT:
						    if (direction == RIGHT)
						    {
						        direction = uturn(board,worms,LEFT);
						        if (direction != LEFT && direction != RIGHT)
						            direction_queue.prepend (LEFT);
						    }
						    else if (can_move_direction(board,worms,LEFT))
						        direction = LEFT;
						    break;
		            	case eDirection::RIGHT:
						    if (direction == LEFT)
						    {
						        direction = uturn(board,worms,RIGHT);
						        if (direction != RIGHT && direction != LEFT)
						            direction_queue.prepend (RIGHT);
						    }
						    else if (can_move_direction(board,worms,RIGHT))
						        direction = RIGHT;
			            	break;
			            default:
				            break;
		            }
		        }
                
			}
            else
            {
                ai_move(board, worms, bonuses, test_logging); /* make AIs decide what direction they will go */
            }
		}
    }
    static void do_parallel_worm_work(
    	Worm &self,
		const std::vector<std::vector<unsigned char>> &board,
		const std::forward_list<Worm> &worms,
		const Warps &warps,
		const Bonuses &bonuses,
		bool test_logging,
		WormSet &dead_worms,
		WormWarpSet &worm_warps
		)
    {
    	self.direction_change(board, worms, bonuses, test_logging);/* change direction? */
    	self.do_wall_and_warps(board, worms, warps, dead_worms, worm_warps);
    }
	Position get_position_after_direction_move(const std::vector<std::vector<unsigned char>> &board) const
	{
        Position position = positions.get_head();
        position.move(direction, board.size(), board[0].size());
        return position;
	}
	Position get_position_after_direction_move(const std::vector<std::vector<unsigned char>> &board,
		const Position origin, const WormDirection direction) const
	{
        Position position = origin;
        position.move(direction, board.size(), board[0].size());
        return position;
	}
	bool is_position_clear_of_materialized_worms (const std::forward_list<Worm> &worms, Position position) const
    {
        for(const Worm &worm : worms)
            if (worm.is_materialized() && worm.positions.contains(position))
                return false;
        return true;
    }
    bool can_move_to(const std::vector<std::vector<unsigned char>> &board,
    	const std::forward_list<Worm> &worms, Position position) const
    {
    	if(board[position.x][position.y] != EMPTYCHAR)
            return false;
        else if (!is_position_clear_of_materialized_worms(worms,position))
            return !is_materialized();/* can move over other worm if I'm not materialized */
        else
            return true;
    }
	bool can_move_to_map(const std::vector<std::vector<unsigned char>> &board,
		const Map &worm_map, Position position)
    {
    	if(board[position.x][position.y] != EMPTYCHAR)
            return false;
        else if (worm_map.contain_position (position))
            return !is_materialized();/* can move over other worm if I'm not materialized */
        else
            return true;
    }
	bool can_move_direction(const std::vector<std::vector<unsigned char>> &board,
    	const std::forward_list<Worm> &worms, WormDirection direction)
    {
        Position position = positions.get_head (); /* head position */
        position.move(direction, board.size(), board[0].size());
        return can_move_to(board, worms, position);
    }
	Position move1(const std::vector<std::vector<unsigned char>> &board)
	{
		auto head=positions.get_head();
		head.move(direction, board.size(), board[0].size());
		positions.prepend_position(head);/* Add a new body piece to the head of the list. */
		return head;
	}
	void move2(const std::vector<std::vector<unsigned char>> &board, Bonuses &bonuses, bool force_materialize=false)
	{
		if(target_length>=positions.get_length())
		{
			/* Add to the worm's size. */
		}
        else
        {
            assert(!positions.is_empty());
            /* Remove a body piece from the tail of the list. */
            bonus_eaten.erase(positions.remove_tail());
			if(target_length>2 && target_length<positions.get_length())
	            bonus_eaten.erase(positions.remove_tail());
        }
        /* Check for bonus, do nothing if there isn't a bonus */
        auto head=positions.get_head();
        auto bonus=bonuses[head.x,head.y];
        if(bonus)
        {
        	bonus_eaten.insert(head);
        	auto b=calculate_bonus(*bonus, bonuses);
        	score+=b;//calculate_bonus(*bonus, bonuses);
	        score_changed=true;
        	bonus->set_to_remove();
        }
        
        if(force_materialize)
        {
        	rounds_to_stay_dematerialized=0;
        }
        else
        {
		    /* If we are dematerialized reduce the rounds dematerialized by one. */
		    if (rounds_to_stay_dematerialized > 1)
		        rounds_to_stay_dematerialized -= 1;
		    /* Try and dematerialize if our rounds are up. */
		    if (rounds_to_stay_dematerialized == 1)
		        materialize(board);
		}
	}
	void move2(const std::vector<std::vector<unsigned char>> &board, Bonuses &bonuses,
		const Position &WarpMove)
	{
		/*teleport the head to the new warped position*/
		positions.set_head(WarpMove);
		move2(board, bonuses);
	}
	const WormDirection &get_direction() const
	{
		return direction;
	}
	unsigned long get_length() const
	{
		return positions.get_length();
	}
	unsigned long get_target_length() const
	{
		return target_length;
	}
	auto is_human() const
	{
		return human;
	}
	void add_score(long increase)
	{
		score+=increase;
        score_changed=true;
	}
	void reduce_score_by_percentage(unsigned long percent)
	{
		score*=percent;
		score/=100;
        score_changed=true;
	}
	bool has_lives() const
	{
		return lives>0;
	}
	unsigned long get_lives() const
	{
		return lives;
	}
	unsigned long get_score() const
	{
		return score;
	}

	void reset(const std::vector<std::vector<unsigned char>> &board, Bonuses &bonuses,
		int dematerialize_rounds)
	{
		rounds_to_stay_dematerialized = 0;
		--lives;
		positions.clear();
		bonus_eaten.clear(); /* forget all the bonuses we have eaten */
		if(lives > 0)
		{
		    spawn(board,bonuses);
			rounds_to_stay_dematerialized = dematerialize_rounds;
			rounds_to_stay_still = 2;
		}
	}
	eWormColour get_colour() const
	{
		return colour;
	}
	bool was_bonus_eaten_at_this_position(uint16_t position) const
	{
		return bonus_eaten.contains(position);
	}
	unsigned long pseudo_random(unsigned long min_inclusive, unsigned long max_exclusive)
	{
		const auto a = 6364136223846793005ULL; /*multiplier*/ 
		const auto c = 1442695040888963407ULL; /*increment*/
		pseudo_random_seed = a * pseudo_random_seed + c;
		return min_inclusive+(pseudo_random_seed % (max_exclusive - min_inclusive));
	}	
	bool do_score_change()
	{
		auto r=score_changed;
		score_changed=false;
		return r;
	}
private:
//	unsigned long change; /*when >0 the worms tail is not removed and change is decremented*/
	unsigned long rounds_to_stay_dematerialized;
	unsigned long rounds_to_stay_still;
	WormPositions positions;
	WormDirection direction;
	std::unordered_set<uint16_t> bonus_eaten;/*position of previously eaten bonuses*/
	unsigned long target_length;
	unsigned long score;
	bool score_changed;
	unsigned long lives;
	unsigned long pseudo_random_seed = 2ULL; /*seed*/
	bool LastUturnA = false;



	
private:
	void play_sound(const char *sound);
	void reverse_other_worms();
    long calculate_bonus(const Bonus &bonus, Bonuses &bonuses)
    {
        if (bonus.fake)
        {
            reverse();
            return 0;
        }
        else
        {
            long score_delta = 0;
            switch (bonus.type)
            {
                case Bonus::REGULAR:
                	{
		                long nth_bonus = bonuses.new_regular_bonus_eaten();
		                target_length += nth_bonus * 4;
		                score_delta = nth_bonus * current_level;
		            }
                    bonus_eaten.insert(bonus);
                    bonuses.reduce_regular();
                    play_sound ("gobble");
                    break;
                case Bonus::DOUBLE:
                    score_delta = target_length * current_level;
                    target_length*=2;
                    play_sound ("bonus");
                    break;
                case Bonus::HALF:
                    if (target_length > 2)
                    {
                        target_length/=2;
                        score_delta = target_length * current_level;
                    }
                    play_sound ("bonus");
                    break;
                case Bonus::LIFE:
                    lives++;
                    play_sound ("life");
                    break;
                case Bonus::REVERSE:
                	reverse_other_worms();
                    play_sound ("reverse");
                    break;
                case Bonus::WARP:
                    break;
            }
            return score_delta;
        }
    }
	const std::forward_list<const Worm*> get_other_worms(Worm *pSelf);
	Position get_position_after_direction_move(Position origin, WormDirection direction,
		const std::vector<std::vector<unsigned char>> &board)
	{
        Position position = {origin.x, origin.y};
        position.move (direction, board.size(), board[0].size());
        return position;
	}
	void materialize(const std::vector<std::vector<unsigned char>> &board)
    {
        /*
         * A worm can only materialise if it is not crossing another worm and
         * the next 12 locations in front of it don’t contain a materialised
         * worm. Stop checking the locations for materialised worms if we find
         * an obstacle on the board.
         */
        Map worm_map(get_other_worms(this), board.size(), board[0].size());
        if (worm_map.contains(positions))
        {
            rounds_to_stay_dematerialized += 1; /* wait until to next round to try to materialise */
        }
        else
        {
		    Position position = positions.get_head();
		    for (int i = 12; i > 0 ; i--)
		    {
		        position = get_position_after_direction_move(position, direction, board);
		        if (board[position.x][position.y] != EMPTYCHAR)
		        {
		            rounds_to_stay_dematerialized = 0; /* materialise now */
		            return;
		        }
		        if (worm_map.contain_position(position))
		        {
		            rounds_to_stay_dematerialized += 1; /* wait until to next round to try to materialise */
		            return;
		        }
		    }
		    rounds_to_stay_dematerialized = 0; /* materialise now */
		}
    }
};

class WormSet 
{
private:
    std::forward_list<Worm*> list;
	mutable std::mutex mtx;
	inline bool contains_no_lock(Worm &worm) const
	{
		for(Worm *w : list)
		{
			if(&worm==w)
				return true;
		}
		return false;
	}
	inline void prepend(Worm &worm)
	{
		list.push_front(&worm);
	}
public:
	inline void add(Worm &worm)/* thread safe */
	{
		std::lock_guard<std::mutex> lock(mtx);
		/* add the worm if it isn't already a member */
		if(!contains_no_lock(worm))
			prepend(worm);
	}
	inline void add(const Worm &worm)/* thread safe */
	{
		std::lock_guard<std::mutex> lock(mtx);
		/* add the worm if it isn't already a member */
		if(!contains_no_lock((Worm &)worm))
			prepend((Worm &)worm);
	}
	inline bool contains(Worm &worm) const/* thread safe */
	{
		std::lock_guard<std::mutex> lock(mtx);
		return contains_no_lock(worm);
	}
	inline bool is_empty() const
	{
		return list.empty();
	}
	inline auto begin() const { return list.begin(); }
	inline auto end() const   { return list.end(); }
	inline auto cbegin() const { return list.cbegin(); }
	inline auto cend() const   { return list.cend(); }
};

class WormWarpSet
{
private:
    struct sWarp
    {
    	Position position;
    	bool bonus;
    };
    std::map<const Worm*, sWarp> map;
	std::mutex mtx;
public:
	void add(const Worm &worm, Position target_position, bool bonus)
	{
		std::lock_guard<std::mutex> lock(mtx);
		map[&worm].position = target_position;
		map[&worm].bonus = bonus;
	}
	bool find(const Worm &worm, Position &target_position, bool &bonus)
	{
		if(map.contains(&worm))
		{
			target_position = map[&worm].position;
			bonus = map[&worm].bonus;
			return true;
		}
		else
			return false;
	}
};

/*
 * Class to store a slice (of cake).
 * 0° is the same as north on a compass.
 * 180° is the same as south on a compass.
 * e.g a slice between 45° and 90°
 * would be between north east and east and would be
 * one eighth of the whole (cake).
 *
 *        /
 *       /
 *      /
 *     /  slice
 *    /
 *   o-----
 *
 * We always go clockwise therefore in this example
 * the min angle is 45° and the max angle 90°.
 * If the max angle is less than or equal to the min
 * angle we have an empty slice.
 *
 */
class Slice
{
public:
    /* variables */
    Angle min;
    Angle max;
//    bool min_set;
//    bool max_set;

    /* constructor */
    Slice()
    {
//        min_set = false;
//        max_set = false;
    }
    Slice(const Slice &copy) : min(copy.min), max(copy.max)
    {
    }

    /* public functions */
/*    void assign(const Slice &o)
    {
        min = o.min;
        max = o.max;
        min_set = o.min_set;
        max_set = o.max_set;
    }*/

    void set_direction_view(eDirection dir, const std::vector<std::vector<unsigned char>> &board)
    {
        /*
         * For the code to work the ratio x / y should be > 0° and <= 45°
         * 369665159/-14116942878 is approximately 1½° (Math.atan2 (x,-y) / Math.PI * 180))
         */
        const int64_t x = 369665159;
        const int64_t y = -14116942878;
        switch(dir)
        {
            case eDirection::EAST:
                /*
                 *      .
                 *     ..
                 *    ...
                 *   ....
                 *  o....
                 *   ....
                 *    ...
                 *     ..
                 *      .
                 */
                min={-y, -x, board.size(), board[0].size()}; // mirror on 0° line (x axis) and rotate +90°
                max={-y, +x, board.size(), board[0].size()}; // rotate +90°print
                break;
            case eDirection::SOUTH:
                /*
                 *      o
                 *     ...
                 *    .....
                 *   .......
                 *  .........
                 */
                min={+x, -y, board.size(), board[0].size()}; // mirror on 0° line (x axis) and rotate 180°
                max={-x, -y, board.size(), board[0].size()}; // rotate 180°
                break;
            case eDirection::WEST:
                /*
                 *  .
                 *  ..
                 *  ...
                 *  ....
                 *  ....o
                 *  ....
                 *  ...
                 *  ..
                 *  .
                 */
                min={+y, +x, board.size(), board[0].size()}; // mirror on 0° line (x axis) and rotate -90°
                max={+y, -x, board.size(), board[0].size()}; // rotate -90°
                break;
            case eDirection::NORTH:
                /*
                 *  .........
                 *   .......
                 *    .....
                 *     ...
                 *      o
                 */
                min={-x, +y, board.size(), board[0].size()}; // mirror on 0° line (x axis)
                max={+x, +y, board.size(), board[0].size()};
                break;
            default:
                break;
        }
    }

    void add_angle(Angle a)
    {
        /*
         * Each angle added makes the slice bigger.
         */
        if (!min.is_set() && !max.is_set())
        {
            min.set(a);
        }
        else if (!min.is_set())
        {
            if (a >= max)
            {
                min.set(max);
                max.set(a);
            }
            else
                min.set(a);
        }
        else if (!max.is_set())
        {
            if (a >= min)
                max.set(a);
            else
            {
                max.set(min);
                min.set(a);
            }
        }
        else // min.is_set() && max.is_set()
        {
            if (a < min)
                min.set(a);
            else if (a > max)
                max.set(a);
        }
    }

	Slice(Position origin, int64_t x, int64_t y, int size)
    {
        /*
         * Set this slice to be the slice created by an object at x,y. When
         * viewed from the origin.
         * For normal objects the size is 1 for bonuses it is 2.
         */

        /*
         * Example for a bonus.
         *
         *
         *       BB
         *      /BB
         *     / /
         *    //
         *   O
         *
         */

        // our origin is in the centre of position e.g. x + 0.5, y + 0.5
        add_angle ({x * 2 - (origin.x * 2 + 1),  y * 2 - (origin.y * 2 + 1)});
        add_angle ({ (x + 1 * size)* 2  - (origin.x * 2 + 1), y * 2 - (origin.y * 2 + 1)});
        add_angle ({x * 2 - (origin.x * 2 + 1), (y + 1 * size)* 2 - (origin.y * 2 + 1)});
        add_angle ({ (x + 1 * size)* 2 - (origin.x * 2 + 1), (y + 1 * size)* 2 - (origin.y * 2 + 1)});
    }

    void intersection_by_position(Position origin, uint8_t _x, uint8_t _y, int size)
    {
        /* Take the slice created by an object at x,y when viewed
         * from the origin and set this to be the overlap with between
         * the created slice and this slice.
         */

        int64_t x = _x;
        int64_t y = _y;
        if (min.x >= 0 && max.x >= 0)
        {
            if (!(x + (size -1) > origin.x))
                x += min.x_max;
        }
        else if (min.x < 0 && max.x < 0)
        {
            if (!(x < origin.x))
                x -= min.x_max;
        }
        else if (min.y >= 0 && max.y >= 0)
        {
            if (!(y + (size -1) > origin.y))
                y += min.y_max;
        }
        else if (min.y < 0 && max.y < 0)
        {
            if (!(y < origin.y))
                y -= min.y_max;
        }
        Angle old_min(min);
        Angle old_max(max);
        *this=Slice(origin, x, y, size);
        if (old_min > min)
            min = old_min;
        if (old_max < max)
            max = old_max;
    }

    bool is_empty () const
    {
        /* Return true if the slice is empty (covers 0°). */
        return !min.is_set() || !max.is_set() || min >= max;
    }

    bool is_bonus_at(uint8_t x, uint8_t y, Bonus b)
    {
        return b.x == x && b.y == y
            || b.x + 1 == x && b.y == y
            || b.x == x && b.y + 1 == y
            || b.x + 1 == x && b.y + 1 == y;
    }

    bool is_position_occupied(Position p, const std::vector<std::vector<unsigned char>> &board, const Worm::Map &worm_map)
    {
    	assert(p.x<board.size() && p.y<board[0].size());
        return board[p.x][p.y] != 'a' || worm_map.contain_position(p);
    }

    int64_t is_visible(Position origin, const std::vector<std::vector<unsigned char>> &board, const Worm::Map &worm_map, Bonus bonus)
    {
        /*
         * Return the distance to a bonus if it is possible to see
         * the bonus. Otherwise return int64.MAX.
         */

        /* remember the positions we have already checked in this array */
        std::unordered_set<uint16_t> checked_positions;

        /* follow the min line, looking for a bonus or a blockage (e.g. wall) */
        for (;!is_empty ();)
        {
            int64_t distance = 0;
            min.set_origin (origin);
            min.set_wrapping (board.size(), board[0].size());
            for (;;)
            {
            	const SignedPosition p(min.get(distance));
                distance++;
                if(!checked_positions.contains(p.wrap_xy()))
                {
                    if(is_bonus_at (p.wrap_x(), p.wrap_y(), bonus))
                    {
                    	if(min.step_along_x())
                    	{
                    		auto dy=origin.y-p.y;
                    		if(dy<0)
                    			return distance-dy;
                    		else
	                    		return distance+dy;
                    	}
                    	else
                    	{
                    		auto dx=origin.x-p.x;
                    		if(dx<0)
	                    		return distance-dx;
	                    	else
	                    		return distance+dx;
                    	}
                    }
                    else if(distance > (min.step_along_x() ? board.size() : board[0].size()) * 2
                      || is_position_occupied ({p.wrap_x(), p.wrap_y()}, board, worm_map))
                    {
                        checked_positions.insert(p.wrap_xy());
                        /* subtract the slice of the blocked position */
                        Slice s(origin, p.x, p.y, 1);
                        min=s.max;
                        break;
                    }
                }
            }
        }
        return std::numeric_limits<int64_t>::max();
    }
};



