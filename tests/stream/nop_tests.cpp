#include <iostream>
#include <vector>
#include <string>

#include <functional>

#include <gtest/gtest.h>

#include "stream/stream.h"

namespace stream_tests {

	using std::cout;
	using std::endl;
	using std::vector;
	using std::string;

	using namespace lipaboy_lib;

	using namespace lipaboy_lib::stream_space;
	using namespace lipaboy_lib::stream_space::operators;

	//---------------------------------Tests-------------------------------//

	TEST(Stream_nop, simple) {
		int zero = Stream(1, 2, 3, 4, 5)
			| nop()
			| return_zero();
		ASSERT_EQ(zero, 0);
	}

}