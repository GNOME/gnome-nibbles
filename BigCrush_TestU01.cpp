#include <iostream>
#include <cstdint>
#include <bit>
#include <mutex>
#include <format>
#include <string>
/*
	install TestU01 BigCrush test suite
	apt-get install libtestu01-0-dev

	compile with:
	g++ -O3 -std=gnu++20 BigCrush_TestU01.cpp -o /tmp/test -ltestu01

	run with:
	/tmp/test > BigCrush_TestU01_results.txt
*/

// Bring in the TestU01 library headers (written in C)
extern "C"
{
	#include <testu01/bbattery.h>
}

using uint128 = __uint128_t;

#if INTPTR_MAX == INT64_MAX
	typedef uint64_t uintsys;
	typedef int64_t  intsys;
#else /* 32-bit compiler */
	typedef uint32_t uintsys;
	typedef int32_t  intsys;
#endif

#include "src/pseudo_random.h"

// --- TestU01 State Adapter Glue ---
// Instantiate the generator globally so the C-style callback can access it safely
LXM_GENERATION_ALGORITHM gen(1, 1, 0, 0);

// Cache system to extract and serve 32 bits at a time from our 128-bit function
uint32_t get_32_bits()
{
	static uint128 current_128 = 0;
	static int available_bits = 0;

	if (available_bits == 0)
	{
		current_128 = gen.next128();
		available_bits = 4; // 4 chunks of 32-bits inside 128-bits
	}

	// Grab the lowest 32 bits
	uint32_t chunk = static_cast<uint32_t>(current_128 & 0xFFFFFFFF);
	
	// Shift down for next request
	current_128 >>= 32;
	available_bits--;

	return chunk;
}

int main()
{
	std::cout << "Starting TestU01 BigCrush suite on LXM Generator..." << std::endl;
	std::cout << "This process will take several hours to complete." << std::endl;

	// Create the interface structure that TestU01 expects
	unif01_Gen* testu01_wrapper = unif01_CreateExternGenBits("LXM_128_Test", get_32_bits);

	// Execute BigCrush (runs 106 intensive statistical tests)
	bbattery_BigCrush(testu01_wrapper);

	// Clean up memory allocated by the wrapper interface
	unif01_DeleteExternGenBits(testu01_wrapper);

	return 0;
}
