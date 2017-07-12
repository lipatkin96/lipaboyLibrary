#ifndef INTERVAL_H
#define INTERVAL_H

#include "iplenty.h"

#include <functional>

namespace IntervalFuncs {

	
	//TODO: Add debug function to output result ( if contains then "c in [a, b]" or "c out [a, b]" )

	template <class T, typename LeftComparison, typename RightComparison>
	class Interval : public IPlenty<T>
	{
	public:
		Interval(const T& left1, const T& right1)
			: leftBorder(left1), rightBorder(right1) {}
		bool in(const T& element) const { return leftComp(leftBorder, element) && rightComp(element, rightBorder); }
		bool out(const T& element) const { return !in(element); }
		bool outLeft(const T& element) const { return !leftComp(leftBorder, element); }
		bool outRight(const T& element) const { return !rightComp(element, rightBorder); }

		virtual bool contains(const T& element) const { return in(element); }

		const T& left() const { return leftBorder; }
		const T& right() const {return rightBorder; }

	protected:
		T leftBorder;
		T rightBorder;
		LeftComparison leftComp;
		RightComparison rightComp;
	};

	template <class T>
	using OpenInterval = Interval<T, std::less<>, std::less<> >;

	template <class T>
	using CloseInterval = Interval<T, std::less_equal<>, std::less_equal<> >;


	//use * to function
	/*std::function<bool(const T& lhs, const T& element)> leftComparison,
	std::function<bool(const T& element, const T& rhs)> rightComparison*/
}

#endif // INTERVAL_H
