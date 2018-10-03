#include "gtest/gtest.h"

#include <iostream>

#include "common_interfaces/algebra.h"
#include "maths/fixed_precision_number.h"

namespace lipaboy_lib_tests {

	using std::cout;
	using std::endl;

	using lipaboy_lib::FixedPrecisionNumber;


	TEST(Algebra, check) {
		FixedPrecisionNumber<double, 1, -5>
			kek1(2),
			kek2(3);
		FixedPrecisionNumber<double, 1, -8>
			kuk(-4);

		ASSERT_EQ(kek1 + kek2, 5);
		ASSERT_EQ(kek1 + 4., 6.);
		ASSERT_EQ((kek1 + kek2) / (kek1 - kek2), -5);
		ASSERT_EQ(kek1 + kuk, -2);

		ASSERT_EQ(kek1 - kek2, -1.);
		ASSERT_EQ(kek1 - 4., -2.);
		ASSERT_EQ(kek1 - kuk, 6.);

		ASSERT_EQ(kek1 * kek2, 6.);
		ASSERT_EQ(kek1 * 4., 8.);
		ASSERT_EQ(kek1 * kuk, -8.);

		ASSERT_EQ(kek1 / kek2, 2. / 3.);
		ASSERT_EQ(kek1 / 4., 1. / 2.);
		ASSERT_EQ(kek1 / kuk, -1. / 2.);
	}

	TEST(EitherComparable, comparison) {
		FixedPrecisionNumber<double, 1, -5>
			kek1(2);
		// not work
		ASSERT_NE(kek1, 2.00002);
		ASSERT_EQ(kek1, 2.00001);
		//ASSERT_FALSE(true);

		ASSERT_TRUE(kek1 < 2.00002);
		ASSERT_TRUE(kek1 <= 2.00002);
		ASSERT_FALSE(kek1 < 2.00001);
		ASSERT_TRUE(kek1 <= 2.00001);
		ASSERT_TRUE(kek1 >= 2.00001);
		ASSERT_FALSE(kek1 > 2.00001);

		ASSERT_TRUE(2.00002 > kek1);
		ASSERT_TRUE(2.00002 >= kek1);
		ASSERT_FALSE(2.00001 > kek1);
		ASSERT_TRUE(2.00001 >= kek1);
		ASSERT_TRUE(2.00001 <= kek1);
		ASSERT_FALSE(2.00001 < kek1);
	}

}