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
 *
 * Algorithm based on 
 * "LXM: Better Splittable Pseudorandom Number Generators (and Almost as Fast)"
 *
 * If you improve this code make sure to run it against
 * TestU01 BigCrush test suite, BigCrush_TestU01.cpp.
 *
 */

class LXM_GENERATION_ALGORITHM
{
private:
	const uint128 a;/* fixed increment, must be odd and each instance should have a different value */
	uint128 s; /* LCG state */
	uint64_t x[4]; /* XBG states */
	static constexpr uint128 m = (static_cast<uint128>(0x1) << 64) | 0xd605bbb58c8abbfd;/* fixed multiplier */
public:
	uint128 next128()
	{
		// Combining operation
		uint128 z = s + (static_cast<uint128>(x[0]) | (static_cast<uint128>(x[1])<<64));

		// 128 bit mixing function
		z = (z ^ (z >> 64)) * 0xd2b74407b1ce6e93;
		z = (z ^ (z >> 64)) * 0xd2b74407b1ce6e93;
		z = z ^ (z >> 64);

		// Update the LCG subgenerator
		s = (m * s) + a;

		// Update the XBG subgenerator
		uint64_t q0 = x[0];
		uint64_t q1 = x[1];
		uint64_t q2 = x[2];
		uint64_t q3 = x[3];
		uint64_t t = q1 << 17;
		q2 ^= q0; q3 ^= q1; q1 ^= q2; q0 ^= q3; q2 ^= t;
		q3 = std::rotl(q3, 45);
		x[0] = q0; x[1] = q1; x[2] = q2; x[3] = q3;

		return z;
	}

	LXM_GENERATION_ALGORITHM(uint128 fixed_increment, uint128 lcg_seed, uint128 xbg_seed_A, uint128 xbg_seed_B) :
		a(fixed_increment | 0x1) /* a must be odd */
	{
		init(lcg_seed, xbg_seed_A, xbg_seed_B);
	}

	auto operator()()
	{
		return next128();
	}

	// LXM implementation of splitting
	LXM_GENERATION_ALGORITHM split()
	{
#if defined(TESTS)
		return LXM_GENERATION_ALGORITHM(
			1,
			1,
			0,
			0);
#else
		// generate fresh random states for the child
		return LXM_GENERATION_ALGORITHM(
			next128(),
			next128(),
			next128(),
			next128());
#endif
	}
private:
	void init(uint128 lcg_seed, uint128 xbg_seed_A, uint128 xbg_seed_B)
	{
		/*
		std::string fixed_multiplier_str = std::format("{:016x}", static_cast<uint64_t>(m>>64)) +
			std::format("{:016x}", static_cast<uint64_t>(m));
		std::string fixed_increment_str = std::format("{:016x}", static_cast<uint64_t>(a>>64)) +
			std::format("{:016x}", static_cast<uint64_t>(a));
		std::string lcg_seed_str = std::format("{:016x}", static_cast<uint64_t>(lcg_seed>>64)) +
			std::format("{:016x}", static_cast<uint64_t>(lcg_seed));
		std::string xbg_seed_A_str = std::format("{:016x}", static_cast<uint64_t>(xbg_seed_A>>64)) +
			std::format("{:016x}", static_cast<uint64_t>(xbg_seed_A));
		std::string xbg_seed_B_str = std::format("{:016x}", static_cast<uint64_t>(xbg_seed_B>>64)) +
			std::format("{:016x}", static_cast<uint64_t>(xbg_seed_B));
		std::cout << fixed_multiplier_str << ": " << fixed_increment_str << ", " << lcg_seed_str << ", " << xbg_seed_A_str << ", " << xbg_seed_B_str << "\n";
		*/
		s = lcg_seed;
		if(xbg_seed_A==0 && xbg_seed_B==0) /* used by the tests and on other rare occasions */
		{
			x[0] = 0x4924924924924924;
			x[1] = 0x4924924924924924;
			x[2] = 0xa5a5a5a5a5a5a5a5;
			x[3] = 0xa5a5a5a5a5a5a5a5;
		}
		else
		{
			x[0] = static_cast<uint64_t>(xbg_seed_A); /* low part */
			x[1] = static_cast<uint64_t>(xbg_seed_A >> 64); /* hi part */
			x[2] = static_cast<uint64_t>(xbg_seed_B); /* low part */
			x[3] = static_cast<uint64_t>(xbg_seed_B >> 64); /* hi part */
		}
	}
};
