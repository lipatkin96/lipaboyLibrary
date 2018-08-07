#include "extra_tools_tests.h"

#include "extra_tools/extra_tools.h"

#include <iterator>
#include <string>
#include <iostream>

namespace lipaboy_lib_tests {

using lipaboy_lib::RelativeForward;
using lipaboy_lib::ProducingIterator;
using lipaboy_lib::InitializerListIterator;

using std::string;
using std::cout;
using std::endl;

namespace {

template <class T>
class A {
public:
    using value_type = T;
public:
    A() {}

    template <class A_>
    A(int e, A_&& a)
          : n(RelativeForward<A_&&, T>::forward(a.n))
    {
//        cout << std::is_lvalue_reference<A_&&>::value << std::is_rvalue_reference<A_&&>::value << endl;
//        cout << std::is_lvalue_reference<A_>::value << std::is_rvalue_reference<A_>::value << endl;
//        cout << std::is_lvalue_reference<decltype(a)>::value
//             << std::is_rvalue_reference<decltype(a)>::value
//             << std::is_object<decltype(a)>::value
//             << endl;
//        cout << std::is_lvalue_reference<decltype(a.n)>::value
//             << std::is_rvalue_reference<decltype(a.n)>::value
//             << std::is_object<decltype(a.n)>::value
//             << endl;
    }

    T n;
};

template <class A_>
using Briefly = RelativeForward<A_&&, typename std::remove_reference_t<A_>::value_type>;

template <class A_>
bool isLValueRef(A_&& a) {
    return std::is_lvalue_reference<
            decltype(Briefly<A_&&>::forward(a.n))>::value;
}

template <class A_>
bool isRValueRef(A_&& a) {
    return std::is_rvalue_reference<
            decltype(Briefly<A_&&>::forward(a.n))>::value;
}

}

TEST(RelativeForward, noisy_test) {
//    using type = A<NoisyD>;
//    type a;
//    type a2 = type(5, a);
//    type a3 = type(3, std::move(a));
//    type a4 = type(1, type());
}

TEST(RelativeForward, simple_test) {
    A<int> temp;
    ASSERT_TRUE(isLValueRef(temp));
    ASSERT_FALSE(isLValueRef(std::move(temp)));
    ASSERT_TRUE(isRValueRef(std::move(temp)));
    ASSERT_FALSE(isRValueRef(temp));
}

TEST(ProducingIterator, lambda_relative_on_external_context) {
	int x = 0;
	ProducingIterator<int> iter([&a = x]() { return a++; });

	ASSERT_EQ(*iter, 0);
	iter++;
	ASSERT_EQ(*iter, 1);
	++iter;
	ASSERT_EQ(*(++iter), 3);

	auto iter2 = iter;
	ASSERT_TRUE(iter == iter2);
	iter++;
	// don't work because ProducingIterator hasn't strong condition in comparison
	//ASSERT_TRUE(iter != iter2);

	// ProducingIterator is not a real iterator
	/*auto lol = std::is_same<typename std::iterator_traits<ProducingIterator<int> >::iterator_category,
		std::input_iterator_tag>::value;
	ASSERT_TRUE(lol);*/
}

TEST(InitializingListIterator, simple) {
	InitializerListIterator<string> iter({ "a", "b", "c", "d", "e" });

	ASSERT_EQ(*(iter++), "a");
	ASSERT_EQ(*iter, "b");
	++iter;
	ASSERT_EQ(*(++iter), "d");

	auto iter2 = iter;

	ASSERT_EQ(*(iter2++), "d");
	ASSERT_NE(iter, iter2);
	iter++;
	ASSERT_EQ(iter, iter2);

	// no difference_type
	auto lol = std::is_same<typename std::iterator_traits<InitializerListIterator<string> >::iterator_category,
							std::input_iterator_tag>::value;
	ASSERT_TRUE(lol);
}

}

