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
 
#include <cstdint>

#if INTPTR_MAX == INT64_MAX
	typedef uint64_t uintsys;
	typedef int64_t  intsys;
#else /* 32-bit compiler */
	typedef uint32_t uintsys;
	typedef int32_t  intsys;
#endif

#if defined(__SIZEOF_INT128__)

using int128 = __int128_t;
using uint128 = __uint128_t;

#elif defined(CAN_USE__BitInt)

using int128 = _BitInt(128);
using uint128 = unsigned _BitInt(128);

#else

#warning Using a compiler that supports _BitInt(128) in C++ code (e.g. clang++) will give better performance
struct int128
{
	uint64_t lo;
	uint64_t hi;/* top bit set indicates a negative int128 */

	int128(int64_t v = 0) : lo(static_cast<uint64_t>(v)), hi(v < 0 ? UINT64_MAX : 0) {}

	explicit int128(int v) : int128(static_cast<int64_t>(v)) {}

	bool negative() const
	{
		return (hi >> 63) != 0;
	}

	int128& operator+=(const int128& rhs)
	{
		uint64_t old = lo;
		lo += rhs.lo;
		hi += rhs.hi + (lo < old);
		return *this;
	}

	int128 operator+(const int128& rhs) const
	{
		int128 r = *this;
		r += rhs;
		return r;
	}

	int128& operator-=(const int128& rhs)
	{
		uint64_t old = lo;
		lo -= rhs.lo;
		hi -= rhs.hi + (old < rhs.lo);
		return *this;
	}

	int128 operator-(const int128& rhs) const
	{
		int128 r = *this;
		r -= rhs;
		return r;
	}

	int128 operator-() const
	{
		int128 r;
		r.lo = ~lo + 1;
		r.hi = ~hi + (r.lo == 0);
		return r;
	}

	int128& operator<<=(int n)
	{
		if(n <= 0)
			return *this;

		if(n >= 128)
		{
			lo = hi = 0;
		}
		else if(n >= 64)
		{
			hi = lo << (n - 64);
			lo = 0;
		}
		else
		{
			hi = (hi << n) | (lo >> (64 - n));
			lo <<= n;
		}

		return *this;
	}

	int128 operator<<(int n) const
	{
		int128 r = *this;
		r <<= n;
		return r;
	}

	int128 operator<<(long n) const
	{
		return *this << static_cast<int>(n);
	}

	int128& operator>>=(int n)
	{
		if(n <= 0)
			return *this;

		const uint64_t sign = negative() ? UINT64_MAX : 0;

		if(n >= 128)
		{
			lo = hi = sign;
		}
		else if(n >= 64)
		{
			lo = (hi >> (n - 64)) |
				 (sign << (128 - n));
			hi = sign;
		}
		else
		{
			lo = (lo >> n) | (hi << (64 - n));
			hi = (hi >> n) | (sign << (64 - n));
		}

		return *this;
	}

	int128 operator>>(int n) const
	{
		int128 r = *this;
		r >>= n;
		return r;
	}

	int128 operator>>(long n) const
	{
		return *this >> static_cast<int>(n);
	}

	int128& operator&=(const int128& rhs)
	{
		lo &= rhs.lo;
		hi &= rhs.hi;
		return *this;
	}

	int128 operator&(const int128& rhs) const
	{
		int128 r = *this;
		r &= rhs;
		return r;
	}

	int128 operator&(int rhs) const
	{
		return *this & int128(rhs);
	}

	bool operator==(const int128& rhs) const
	{
		return lo == rhs.lo && hi == rhs.hi;
	}

	bool operator!=(const int128& rhs) const
	{
		return !(*this == rhs);
	}

	bool operator<(const int128& rhs) const
	{
		const bool a_neg = negative();
		const bool b_neg = rhs.negative();

		if(a_neg != b_neg)
			return a_neg;

		if(hi != rhs.hi)
			return hi < rhs.hi;

		return lo < rhs.lo;
	}

	bool operator>(const int128& rhs) const
	{
		return rhs < *this;
	}

	bool operator<=(const int128& rhs) const
	{
		return !(*this > rhs);
	}

	bool operator>=(const int128& rhs) const
	{
		return !(*this < rhs);
	}

	bool operator<(int rhs) const
	{
		return *this < int128(rhs);
	}

	bool operator>(int rhs) const
	{
		return *this > int128(rhs);
	}

	bool operator<=(int rhs) const
	{
		return *this <= int128(rhs);
	}

	bool operator>=(int rhs) const
	{
		return *this >= int128(rhs);
	}

	/*
	 * Simple shift/add multiplication.
	 *
	 * Everything is done modulo 2^128, which is what we need
	 * for two's-complement integer arithmetic.
	 */
	int128 operator*(const int128& rhs) const
	{
		const bool neg = negative() ^ rhs.negative();

		int128 a = *this;
		int128 b = rhs;

		if(a.negative())
			a = -a;
		if(b.negative())
			b = -b;

		int128 result = 0;
		for(int i = 0; i < 128; ++i)
		{
			if(b & 1)
				result += a;
			a <<= 1;
			b >>= 1;
		}
		return neg ? -result : result;
	}

	/*
	 * Simple binary long division.
	 */
	int128 operator/(const int128& rhs) const
	{
		assert(rhs);

		const bool neg = negative() ^ rhs.negative();

		int128 a = *this;
		int128 b = rhs;

		if(a.negative())
			a = -a;
		if(b.negative())
			b = -b;

		int128 quotient = 0;
		int128 remainder = 0;
		for(int i = 127; i >= 0; --i)
		{
			remainder <<= 1;
			if((a >> i) & 1)
				remainder.lo |= 1;
			if(remainder >= b)
			{
				remainder -= b;
				quotient += int128(1) << i;
			}
		}
		return neg ? -quotient : quotient;
	}

	explicit operator int64_t() const
	{
		return static_cast<int64_t>(lo);
	}

	explicit operator bool() const
	{
		return lo!=0 || hi!=0;
	}
};

struct uint128
{
	uint64_t lo;
	uint64_t hi;

	constexpr uint128(uint64_t v = 0) : lo(v), hi(0) {}

	template<std::integral T>
	constexpr explicit uint128(T v)
		: lo(static_cast<uint64_t>(v)),
		  hi(std::is_signed_v<T> && v < 0 ? UINT64_MAX : 0) {}

	constexpr uint128& operator<<=(int n)
	{
		if(n <= 0)
			return *this;

		if(n >= 128)
		{
			lo = hi = 0;
		}
		else if(n >= 64)
		{
			hi = lo << (n - 64);
			lo = 0;
		}
		else
		{
			hi = (hi << n) | (lo >> (64 - n));
			lo <<= n;
		}

		return *this;
	}

	constexpr uint128 operator<<(int n) const
	{
		uint128 r = *this;
		r <<= n;
		return r;
	}

	constexpr uint128& operator>>=(int n)
	{
		if(n <= 0)
			return *this;

		if(n >= 128)
		{
			lo = hi = 0;
		}
		else if(n >= 64)
		{
			lo = (hi >> (n - 64));
			hi = 0;
		}
		else
		{
			lo = (lo >> n) | (hi << (64 - n));
			hi = (hi >> n);
		}

		return *this;
	}

	constexpr uint128 operator>>(int n) const
	{
		uint128 r = *this;
		r >>= n;
		return r;
	}

	constexpr uint128& operator&=(uint128 rhs)
	{
		lo&=rhs.lo;
		hi&=rhs.hi;
		return *this;
	}

	constexpr uint128 operator&(uint128 rhs) const
	{
		uint128 r=*this;
		r&=rhs;
		return r;
	}

	constexpr uint128& operator|=(uint128 rhs)
	{
		lo|=rhs.lo;
		hi|=rhs.hi;
		return *this;
	}

	constexpr uint128 operator|(uint128 rhs) const
	{
		uint128 r=*this;
		r|=rhs;
		return r;
	}

	constexpr uint128& operator^=(uint128 rhs)
	{
		lo^=rhs.lo;
		hi^=rhs.hi;
		return *this;
	}

	constexpr uint128 operator^(uint128 rhs) const
	{
		uint128 r=*this;
		r^=rhs;
		return r;
	}

	constexpr uint128 operator~() const
	{
		uint128 r;
		r.lo=~lo;
		r.hi=~hi;
		return r;
	}

	constexpr uint128& operator+=(uint128 rhs)
	{
		uint64_t old_lo = lo;
		lo += rhs.lo;
		hi += rhs.hi + (lo < old_lo);
		return *this;
	}

	constexpr uint128 operator+(uint128 rhs) const
	{
		uint128 r=*this;
		r+=rhs;
		return r;
	}

	constexpr uint128& operator*=(uint128 rhs)
	{
		uint128 lhs = *this;
		uint128 result;
		while(rhs.lo || rhs.hi)
		{
			if(rhs.lo & 1)
				result += lhs;
			rhs >>= 1;
			lhs <<= 1;
		}
		*this = result;
		return *this;
	}

	constexpr uint128 operator*(uint128 rhs) const
	{
		uint128 r=*this;
		r*=rhs;
		return r;
	}

	constexpr bool operator==(const uint128 &) const = default;

	constexpr bool operator>(const uint128 &rhs) const
	{
		return hi>rhs.hi || hi==rhs.hi && lo>rhs.lo;
	}

	constexpr explicit operator uint64_t() const
	{
		return lo;
	}
};

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
