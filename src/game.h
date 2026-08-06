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

#pragma once

#include <utility>

#define EMPTYCHAR	'a'
#define WARPCHAR	'W'

class Worm;

struct HumanAction
{
	eWormColour colour;
	eDirection direction;
};

class Game
{
public:
	/* sub-classes */
	enum Progress
	{
		SEQUENTIAL,
		RANDOM,
		FIXED,
		TEST
	};
	

	/* constructor */
	Game(std::function<void(const char *)> play_sound,
		std::function<eWormColour(unsigned long)> get_worm_settings_colour,
		std::function<void(eWormColour, unsigned long)> life_change,
		std::function<void(eWormColour, unsigned long)> score_change,
		Progress progress) :
			_play_sound(play_sound), _get_worm_settings_colour(get_worm_settings_colour),
			_life_change(life_change), _score_change(score_change),
			progress(progress), warps(board)
	{
	}
	Game(uint8_t max_bonuse_count) :
		_play_sound(nullptr), _get_worm_settings_colour(nullptr),
		_life_change(nullptr), _score_change(nullptr),
		progress(TEST), warps(board), bonuses(max_bonuse_count)
	{
	}
	virtual ~Game() = default;

	bool load_board_from_file(const char *path, unsigned long level);
/*	bool load_board_from_file(const Glib::ustring &path, unsigned long level)
	{
		return load_board_from_file(path.c_str(),level);
	}
	
	bool load_board(const Glib::ustring &string)
	{
		board.clear();
		width=height=0;// we don't yet know the width or height
		const char *s=string.c_str();
		const char *e=s+string.length();
		uint32_t u32;
		
		while(s<e)
		{
			u32=0xff & *s;
			auto extra=unichar_extra_width(*s++);
			for(;e-s<=extra;s++)
			{
				u32<<=8;
				u32|=0xff & *s;
			}
			build_board(to_board_char(u32), board);
		}
		height=board[board.size()-1].size();
		return verify_load();
	}
*/
	bool load_board(const std::span<const std::string_view> &strings)
	{
		level = 1; /* set this for scoreing calculations */
		board.clear();
		starts.clear();
		width=height=0;// we don't yet know the width or height
		const char *s,*e;
		for(const auto &string : strings)
		{
			s=string.data();
			e=s+string.length();
			uint32_t u32;
			
			while(s<e)
			{
				u32=0xff & *s;
				auto extra=unichar_extra_width(*s++);
				for(;s<e && extra>0;s++,extra--)
				{
					u32<<=8;
					u32|=0xff & *s;
				}
				build_board(u32, board);
			}
			if(width==0)
				width=board.size();
		}
		height=board[board.size()-1].size();
		return verify_load();
	}

	void set_map(std::vector<std::vector<unsigned char>> &map)
	{
		board = std::move(map);
	}
	const unsigned char& operator[](unsigned int x, unsigned int y) const
	{
		return board[x][y];
	}
	unsigned int get_width() {return width;}
	unsigned int get_height() {return height;}
	void create_worms(unsigned long human_count, unsigned long ai_count)
	{
		worms.clear();
		assert(human_count+ai_count<=6);
		for(unsigned long i=0;i<human_count+ai_count;i++)
		{
			if(_get_worm_settings_colour==nullptr)
			{
				Worm worm(*this, level, i<human_count, (eWormColour)i, get_width(), get_height());
				worms.push_front(worm);
			}
			else
			{
				Worm worm(*this, level, i<human_count, _get_worm_settings_colour(i), get_width(), get_height());
				worms.push_front(worm);
			}
		}
		starting_human_count = human_count;
		starting_ai_count = ai_count;
	}
	void spawn_worms(bool force_materialize=false)
	{
		/* spawn worms for a new board */
		auto it=starts.begin();
		for(Worm &worm : worms)
		{
			worm.spawn(*it++, board,bonuses,force_materialize);
		}
		/* clear bonuses */
		bonuses_to_replace = 0;
		bonuses.clear();
	}
	void reverse_worms(Worm *ignore)
	{
		for(Worm &worm : worms)
		{
			if(&worm!=ignore)
				worm.reverse();
		}		
	}
/*	void start (bool add_initial_bonus)
	{
		if (add_initial_bonus)
			add_bonus (true);

		is_running = true;

		main_id = Timeout.add (speed == 1 ? gamedelay * 3 / 2 : gamedelay * speed, () => {
				bonus_cycle = (bonus_cycle + 1) % 3;
				if (bonus_cycle == 0)
					add_bonus (false);
				return main_loop_cb ();
			});
		Source.set_name_by_id (main_id, "[Nibbles] main_loop_cb");
	}*/
	bool add_bonus(bool regular);
	void move_worms();
	const std::forward_list<Worm> &get_worms()
	{
		return worms;
	}
	void play_sound(const char *sound)
	{
		static std::mutex mtx;
		std::lock_guard<std::mutex> lock(mtx);

		if(_play_sound!=nullptr)
			_play_sound(sound);
		else if(progress==TEST)
			std::cout << "play sound " << sound << std::endl;
	}
	void score_change(eWormColour colour, unsigned long score)
	{
		if(_score_change!=nullptr)
			_score_change(colour,score);
		else if(progress==TEST)
			std::cout << "worm " << colour << " score " << score << std::endl;
	}
	void life_change(eWormColour colour, unsigned long count)
	{
		if(_life_change!=nullptr)
			_life_change(colour,count);
		else if(progress==TEST)
			std::cout << "worm " << colour << " lives " << count << std::endl;
	}
	const Warps &get_warps() const
	{
		return warps;
	}
	const Bonuses &get_bonuses() const
	{
		return bonuses;
	}
	/* The Game Status enumerated type, returned by get_game_status ()*/
	enum eStatus {GAMEOVER, VICTORY, NEWROUND, ACTIVE};

	eStatus get_game_status() const
	{
		auto [human_alive, ai_alive]=count_alive_worms();

		if(progress==TEST)
		{
			/* A test is over if all the regular bonuses have been consumed
			   or there are no worms left alive. */
			if(human_alive+ai_alive==0 || bonuses.last_regular_bonus())
				return GAMEOVER;
			else
				return ACTIVE;
		}
	
		if(ai_alive>0)
		{
			if(human_alive>0)
			{
				if(progress != FIXED && bonuses.last_regular_bonus())
					return NEWROUND;
				else
					return ACTIVE;
			}
			else /* no human worms alive */
			{
				if(starting_human_count>0)
					return GAMEOVER;
				else
					return ACTIVE;
			}
		}
		else /* no ai worms alive */
		{
			if(starting_ai_count>0)
				return VICTORY; /* killed all the ai worms */
			else
			{
				if(human_alive>=2)
					return ACTIVE;
				else
					return VICTORY; /* killed all the other human worms */
			}
		}
	}
	uint8_t get_regular_bonuses_consumed()
	{
		return bonuses.get_regular_bonuses_consumed();
	}
	void human_action(HumanAction action)
	{
		for(Worm &worm : worms)
		{
			if(worm.get_colour()==action.colour)
			{
				worm.queue_direction(action.direction);
				break;
			}
		}
	}
	std::vector<WormScore> get_worm_scores() const
	{
		std::vector<WormScore> result;
		for(const Worm &worm : worms)
		{
			if(worm.is_human())
				result.emplace_back(WormScore(worm.get_colour(),worm.get_score()));
		}
		return result;
	}
	void print_board() const;
private:
	std::function<void(const char *)> _play_sound;
	std::function<eWormColour(unsigned long)> _get_worm_settings_colour;
	std::function<void(eWormColour, unsigned long)> _life_change;
	std::function<void(eWormColour, unsigned long)> _score_change;
	const Progress progress;
	Warps warps;
	enum class WarpType {NONE,SOURCE,TARGET};
	unsigned int width,height;
	std::vector<std::vector<unsigned char>> board;
	std::forward_list<Start> starts;
	unsigned long level;
	bool fakes;
	std::forward_list<Worm> worms;
	Bonuses bonuses;
	uint8_t bonuses_to_replace;
	unsigned long starting_human_count,starting_ai_count;

	unsigned int unichar_extra_width(char c)
	{
		if((c & 0x80) == 0)
			return 0;
		else if((c & 0xE0) == 0xC0)
			return 1;
		else if((c & 0xF0) == 0xE0)
			return 2;
		else /*if((c & 0xF8) == 0xF0)*/
			return 3;
	}
	bool get_unichar(std::ifstream &stream, uint32_t &u32);
	void build_board(uint32_t u32, std::vector<std::vector<unsigned char>> &board)
	{
		auto [c, warp, start_direction]=to_board_char(u32);
		if(!c)
		{
			if(width==0)
				width=board.size();
		}
		else
		{
			Position p;
			if(width==0)
			{
				std::vector<unsigned char> column;
				p.y=0;
				column.push_back(c);
				p.x=board.size();
				board.push_back(column);
			}
			else if(board.size()<width)
			{
				if(board.capacity()<width)
					board.reserve(width);
				std::vector<unsigned char> column;
				if(height>0)
					column.reserve(height);
				p.y=0;
				column.push_back(c);
				p.x=board.size();
				board.push_back(column);
			}
			else
			{
				unsigned int column_height=board[0].size();
				bool added=false;
				for(uint8_t x=1;x<width;x++)
				{
					if(board[x].size()<column_height)
					{
						p.x=x;
						p.y=board[x].size();
						board[x].push_back(c);
						added=true;
						break;
					}
				}
				if(!added)
				{
					p.x=0;
					p.y=board[0].size();
					board[0].push_back(c);
				}
			}
			if(warp==WarpType::SOURCE)
			{
				warps.add_warp_source(u32,p);
				if(progress==TEST)
					std::cout << "warp source: " << p.x << "," << p.y << std::endl;
			}
			else if(warp==WarpType::TARGET)
			{
				warps.add_warp_target(u32-('a'-'A'),p);
				if(progress==TEST)
					std::cout << "warp target: " << p.x << "," << p.y << std::endl;
			}
			else if(start_direction!=eDirection::NONE)
			{
				starts.push_front(Start(start_direction,p));
				if(progress==TEST)
					std::cout << "start position: " << p.x << "," << p.y << std::endl;
			}
		}
	}
	std::tuple<unsigned char, WarpType, WormDirection> to_board_char(uint32_t u32);
	bool verify_load()
	{
		if(width==0)
			return false;
		for(unsigned int v=0;v<width;v++)
		{
			if(board[v].size()!=height)
				return false;
		}
		return true;
	}
	void _add_bonus (uint8_t x, uint8_t y, Bonus::eType bonus_type, bool fake, uint16_t countdown)
	{
		Bonus bonus(x, y, bonus_type, fake, countdown);
		bonuses.add(bonus);
		if(bonus.type != Bonus::REGULAR)
			play_sound("appear");
	}
	bool two_or_more_worms()
	{
		if(worms.empty())
			return false;
		auto it=worms.begin();
		++it;
		return it!=worms.end();
	}
	std::pair<unsigned long, unsigned long> count_alive_worms() const
	{
		unsigned long ai=0;
		unsigned long human=0;
		for(const auto &worm : worms)
		{
			if(worm.get_lives()>0)
			{
				if(worm.is_human())
					human++;
				else
					ai++;
			}
		}
		return {human,ai};
	}
};

