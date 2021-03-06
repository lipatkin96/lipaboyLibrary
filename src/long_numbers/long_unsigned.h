#pragma once

#include <algorithm>    // std::fill, std::next
#include <numeric>
#include <array>
#include <cstdint>
#include <string>
#include <cmath>
#include <tuple>        // std::pair
#include <optional>

#include <exception>
#include <ostream>
#include <string_view>
#include <charconv>

#include "extra_tools/extra_tools.h"
#include "intervals/cutoffborders.h"
#include "extra_tools/maths_tools.h"
#include "common_interfaces/comparable.h"

namespace lipaboy_lib::long_numbers_space {

    // Motivation: I wanna check the theorem in Theory of Numbers
    //			   (you know it or you can find it in pale red pocket book "Theory of Numbers in Cryptography")

    // PLAN
    // ----
    // See LongIntegerDecimal
    // 1) Write new Algebra with another realization: instead of use method 'getNumber()'
    //      for wrapper class you need to cast the primary type to wrapper class.
    //      Like: (*this) + LongUnsigned(value)

    using std::array;
    using std::string;
    using std::pair;

    using lipaboy_lib::cutOffLeftBorder;
    using lipaboy_lib::enable_if_else_t;
    using lipaboy_lib::powDozen;
    using lipaboy_lib::ComparatorExtender;

    // Concept: it is simple long number, without any trivial optimizations like
    //			checking if number is increasing or not (in order to making less computations)
    //			and without move-semantics

    ////////////////////////////////////////////////////////////////////////////////////////

    namespace extra {

        template <class TWord>
        inline constexpr size_t bitsCount() { return sizeof(TWord) * 8; }

        //////////////////////////////////////////////////////////////////////////////////
        template <size_t val1, size_t val2>
        struct Max {
            static constexpr size_t value = (val1 < val2) ? val2 : val1;
        };

        using LengthType = size_t;
    }


    using extra::LengthType;

    // Requirements:
    // 1) TIntegral and TResult must be unsigned.

    template <LengthType lengthOfIntegrals>     // count of integral type variables
    class LongUnsigned
    {
    public:
        using TIntegral = std::uint32_t;
        using TIntegralResult = std::uint64_t;
        using TSigned = std::int32_t;
        using TSignedResult = std::int64_t;
        using LengthType = extra::LengthType;
        using size_type = size_t;

        using IntegralType =
            std::remove_reference_t<
                enable_if_else_t<2 * sizeof(TIntegral) == sizeof(TIntegralResult), TIntegral, void> >;
        using ResultIntegralType = std::remove_reference_t<TIntegralResult>;
        using ContainerType = array<IntegralType, lengthOfIntegrals>;
        using iterator = typename ContainerType::iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_iterator = typename ContainerType::const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        using reference = LongUnsigned&;
        using const_reference = const LongUnsigned&;
        using reference_integral = IntegralType&;
        using const_reference_integral = const IntegralType&;

    public:
        template <LengthType otherLengthOfIntegrals> 
        friend class LongUnsigned;

    protected:
        LongUnsigned(ContainerType const& number)
            : number_(number)
        {}

    public:
        // Note: Non-initialized constructor: without filling array by zeroIntegral value.
        explicit
            LongUnsigned() { checkTemplateParameters(); }
        LongUnsigned(IntegralType small) {
            checkTemplateParameters();
            number_[0] = small;
            std::fill(std::next(begin()), end(), zeroIntegral());
        }
        explicit
            LongUnsigned(string const& numberStr, unsigned int base = 10);
        template <LengthType otherLen>
            explicit
            LongUnsigned(LongUnsigned<otherLen> const & other) {
                checkTemplateParameters();
                auto minLen = (other.length() > length()) ? length() : other.length();
                std::copy_n(other.cbegin(), minLen, std::begin(number_));
                std::fill(std::next(begin(), minLen), end(), zeroIntegral());
            }

        // TODO: calculate how much copy-constructor was called
        template <LengthType otherLen>
        auto operator+(LongUnsigned<otherLen> const& other) const
            -> LongUnsigned< extra::Max<lengthOfIntegrals, otherLen>::value >
        {
            using ResultType = LongUnsigned< extra::Max<lengthOfIntegrals, otherLen>::value >;
            return (ResultType(*this) += other);
        }

        template <LengthType otherLen>
        auto operator+=(LongUnsigned<otherLen> const & other)
            -> const_reference
        {
            constexpr auto MIN_LENGTH = std::min(lengthOfIntegrals, otherLen);
            IntegralType remainder = zeroIntegral();
            size_t i = 0;
            for (; i < MIN_LENGTH; i++) {
                const TIntegralResult dualTemp =
                    TIntegralResult((*this)[i])
                    + TIntegralResult(other[i])
                    + TIntegralResult(remainder);

                (*this)[i] = IntegralType(dualTemp & integralModulus());
                remainder = IntegralType(dualTemp >> integralModulusDegree());
            }
            if constexpr (length() > MIN_LENGTH) {
                for (; i < length(); i++) {
                    const TIntegralResult dualTemp =
                        TIntegralResult((*this)[i])
                        + TIntegralResult(remainder);

                    (*this)[i] = IntegralType(dualTemp & integralModulus());
                    remainder = IntegralType(dualTemp >> integralModulusDegree());
                }
            }

            return *this;
        }

        // TODO: you can optimize it. When inverse operator is called then useless copy will be created.
        template <LengthType otherLen>
        LongUnsigned operator-(LongUnsigned<otherLen> const& other) const {
            return (LongUnsigned(*this) -= other);
        }

        template <LengthType otherLen>
        auto operator-=(LongUnsigned<otherLen> const& other)
            -> const_reference
        {
            constexpr auto MIN_LENGTH = std::min(lengthOfIntegrals, otherLen);
            IntegralType remainder = zeroIntegral();
            size_t i = 0;
            for (; i < MIN_LENGTH; i++) {
                const TIntegralResult dualTemp =
                    TIntegralResult((*this)[i])
                    - TIntegralResult(other[i])
                    - TIntegralResult(remainder);

                (*this)[i] = IntegralType(dualTemp & integralModulus());
                remainder = IntegralType(dualTemp >> (2 * integralModulusDegree() - 1));
            }
            if constexpr (length() > MIN_LENGTH) {
                for (; i < length(); i++) {
                    const TIntegralResult dualTemp =
                        TIntegralResult((*this)[i])
                        - TIntegralResult(remainder);

                    (*this)[i] = IntegralType(dualTemp & integralModulus());
                    remainder = IntegralType(dualTemp >> (2 * integralModulusDegree() - 1));
                }
            }

            return *this;
        }

        template <LengthType otherLen>
        auto operator*(LongUnsigned<otherLen> const& other) const
            -> LongUnsigned< extra::Max<lengthOfIntegrals, otherLen>::value >
        {
            using ResultType = LongUnsigned< extra::Max<lengthOfIntegrals, otherLen>::value >;
            ResultType res(0);
            //		// This chapter has two parts
            //		// First part. Bisecting the result by two portions: main and overflow ones
            //		// Second part. Assigning the main portion into destination
            //		//				and saving the overflow one in order to take account at next multiplication.
            //		// Detail. There are two overflow portions that appear by two requirements:
            //		//		   1) Type can't storage more than it's capable.
            //		//		   2) Decimal type of this class.

            for (size_t i = 0; i < length(); i++)
            {
                IntegralType remainder = zeroIntegral();
                size_t j = 0;
                for (; i + j < res.length(); j++)
                {
                    if (j >= other.length()) {
                        res[i + j] += remainder;
                        break;
                    }

                    const TIntegralResult dualTemp = TIntegralResult((*this)[i]) * other[j] + remainder;
                    // Detail #2
                    res[i + j] += IntegralType(dualTemp & integralModulus());
                    remainder = IntegralType(dualTemp >> integralModulusDegree());
                }
            }

            return res;
        }

        template <LengthType otherLen>
        const_reference operator*=(LongUnsigned<otherLen> const& other) {
            (*this) = (*this) * other;
            return *this;
        }

        LongUnsigned operator/(const_reference other) const {
            return this->divide(other).first;
        }

        const_reference operator/=(const_reference other) {
            (*this) = (*this) / other;
            return *this;
        }

        LongUnsigned operator%(const_reference other) const {
            return this->divide(other).second;
        }

        const_reference operator%=(const_reference other) {
            (*this) = (*this) % other;
            return *this;
        }

        template <LengthType otherLen>
        auto divide(LongUnsigned<otherLen> const & other) const->pair<LongUnsigned, LongUnsigned>
        {
            // TODO: replace to OneDigitNumber
            const LongUnsigned<1> DEC(10);
            const LongUnsigned<1> ONE(1);
            const LongUnsigned<1> ZERO(0);

            #if (defined(WIN32) && defined(DEBUG_)) || (defined(__linux__) && !defined(NDEBUG))
                if (other == ZERO) {
                    throw std::runtime_error("Runtime Error (LongUnsigned): division by zero");
                }
            #endif

            LongUnsigned quotient(0);
            LongUnsigned dividend(*this);
            LongUnsigned divider(other);
            LongUnsigned modulus(1);

            int dividendMajorBit = int(dividend.majorBitPosition().value_or(0));
            int dividerMajorBit = int(divider.majorBitPosition().value_or(0));

            int diff = dividendMajorBit - dividerMajorBit;
            if (diff > 0) {
                divider.shiftLeft(diff);
                modulus.shiftLeft(diff);
                if (divider > dividend) {
                    divider.shiftRight(1);
                    modulus.shiftRight(1);
                }
            }

            while (dividend >= divider || modulus != ONE) {
                while (dividend >= divider) {
                    dividend -= divider;
                    quotient += modulus;
                }

                while (modulus != ONE) {
                    divider.shiftRight(1);
                    modulus.shiftRight(1);
                    if (divider <= dividend)
                        break;
                }
            }
            // dividend - it is equal to remainder of division

            return std::make_pair(quotient, dividend);
        }
        const_reference shiftLeft(unsigned int count);
        const_reference shiftRight(unsigned int count);

        //-------------Converter---------------//

        string to_string(unsigned int base = 10) const;

        //------------Setters, Getters----------//

        static constexpr LengthType length() { return lengthOfIntegrals; }

        bool isZero() const {
            for (auto iter = cbegin(); iter != cend(); iter++)
                if (*iter != zeroIntegral())
                    return false;
            return true;
        }

        // Question: is it normal? Two methods have the same signature and live together??
        //			 Maybe operator[] is exception of rules?
        const_reference_integral operator[] (size_t index) const { return number_[index]; }

        reference_integral operator[] (size_t index) { return number_[index]; }

        const_reference operator= (string const& numberStr) {
            this->assignStr(numberStr);
            return *this;
        }

        const_reference operator= (IntegralType small) {
            number_[0] = small;
            std::fill(std::next(number_.begin()), number_.end(), zeroIntegral());
            return *this;
        }

    public:
        void assignStr(string const& numberStr, unsigned int base = 10);

        //-------------------------Comparison---------------------------//

    private:
        template <LengthType lengthFirst, LengthType lengthSecond>
        bool isLess(LongUnsigned<lengthFirst> const& first,
                    LongUnsigned<lengthSecond> const& second) const
        {
            using FirstTypeIter = typename LongUnsigned<lengthFirst>::iterator;
            using SecondTypeIter = typename LongUnsigned<lengthSecond>::iterator;

            bool isLessVar = true;
            bool isEqual = true;
            bool isResultDefined = false;
            auto iterF = first.crbegin();
            auto iterS = second.crbegin();
            auto checkHigherPartToZero =
                [&isLessVar, &isEqual, &isResultDefined](auto& iter, const int lenHigh, const int lenLow)
                -> bool
            {
                bool partIsZero = true;
                // must cast all the vars to int because (I don't know)
                for (int i = 0; i < int(lenHigh) - int(lenLow); i++) {
                    if (*iter != zeroIntegral()) {
                        partIsZero = false;
                        isEqual = false;
                        isResultDefined = true;
                        break;
                    }
                    iter++;
                }
                return isResultDefined ? partIsZero : isLessVar;
            };

            isLessVar = checkHigherPartToZero(iterF, lengthFirst, lengthSecond);
            if (!isResultDefined)
                isLessVar = !checkHigherPartToZero(iterS, lengthSecond, lengthFirst);

            if (!isResultDefined) {
                for (; iterF != first.crend() && iterS != second.crend(); iterF++, iterS++) {
                    if (*iterF > * iterS) {
                        isLessVar = false;
                        isEqual = false;
                        break;
                    }
                    else if (*iterF < *iterS) {
                        isLessVar = true;
                        isEqual = false;
                        break;
                    }
                }
            }
            return (!isEqual) && isLessVar;
        }

    public:
        template <LengthType otherLen>
        bool operator!= (LongUnsigned<otherLen> const& other) const {
            bool isEqual = true;
            auto iter = cbegin();
            auto iterO = other.cbegin();
            for (; iter != cend() && iterO != other.cend(); iter++, iterO++) {
                if (*iter != *iterO) {
                    isEqual = false;
                    break;
                }
            }
            if (isEqual) { 
                for (; iter != cend(); iter++) {
                    if (*iter != zeroIntegral()) {
                        isEqual = false;
                        break;
                    }
                }
                for (; iterO != other.cend(); iterO++) {
                    if (*iterO != zeroIntegral()) {
                        isEqual = false;
                        break;
                    }
                }
            }
            return !isEqual;
        }
        template <LengthType otherLen>
        bool operator== (LongUnsigned<otherLen> const& other) const { return !(*this != other); }
        template <LengthType otherLen>
        bool operator< (LongUnsigned<otherLen> const& other) const { return this->isLess(*this, other); }

        template <LengthType otherLen>
        bool operator>= (LongUnsigned<otherLen> const& other) const { return !(*this < other); }
        template <LengthType otherLen>
        bool operator> (LongUnsigned<otherLen> const& other) const { return this->isLess(other, *this); }
        template <LengthType otherLen>
        bool operator<= (LongUnsigned<otherLen> const& other) const { return !(*this > other); }

    public:
        auto majorBitPosition() const
            -> std::optional<size_type>
        {
            for (int i = int(length()) - 1; i >= 0; i--) {
                auto curr = (*this)[i];
                if (curr > 0) {
                    size_type bitpos = 0;
                    while (curr > 0) {
                        curr >>= 1;
                        bitpos++;
                    }
                    return (bitpos - 1) + size_type(i) * 8 * sizeof(IntegralType);
                }
            }
            return std::nullopt;
        }

    public:
        // maximum count decimal digits that can be placed into IntegralType
        static constexpr IntegralType integralModulusDegree() { return IntegralType(extra::bitsCount<IntegralType>()); }
        static constexpr ResultIntegralType integralModulus() { return std::numeric_limits<IntegralType>::max(); }
        static LongUnsigned max() {
            LongUnsigned max(1);
            max.shiftLeft(unsigned int(integralModulusDegree() * length() - 1));
            return max;
        }

    private:
        static constexpr IntegralType zeroIntegral() { return IntegralType(0); }

    protected:
        iterator begin() { return number_.begin(); }
        iterator end() { return number_.end(); }
        reverse_iterator rbegin() { return number_.begin(); }
        reverse_iterator rend() { return number_.end(); }
        const_iterator cbegin() const { return number_.cbegin(); }
        const_iterator cend() const { return number_.cend(); }
        const_reverse_iterator crbegin() const { return number_.crbegin(); }
        const_reverse_iterator crend() const { return number_.crend(); }

    private:
        void checkTemplateParameters() {
            static_assert(lengthOfIntegrals > 0, "Wrong length of LongInteger");
        }

    private:
        // if index is increased then rank is increased
        array<IntegralType, lengthOfIntegrals> number_;

    };

    //------------------------------------------------------------------------------------------//
    //-------------------------------      Methods     -----------------------------------------//
    //------------------------------------------------------------------------------------------//

    template <LengthType length>
    LongUnsigned<length>::LongUnsigned(string const& numberStr, unsigned int base)
    {
        checkTemplateParameters();
        if (numberStr.length() <= 0 || base < 2)
            LongUnsigned();
        else
            assignStr(numberStr, base);
    }

    template <LengthType length>
    void LongUnsigned<length>::assignStr(string const& numberStr, unsigned int base) {
        // TODO: add exception for zero length
        if (numberStr.length() > 0) {
            const int integralModulusDegreeOfBase =
                int(std::log(2) / std::log(base) * integralModulusDegree());

            std::string_view numStrView = numberStr;
            numStrView.remove_prefix(
                cutOffLeftBorder<int>(0, int(numStrView.find_first_not_of(" ")))
            );
            // round the number by integral modulus
            numStrView.remove_prefix(
                cutOffLeftBorder<int>(0, int(numStrView.length()) - int(integralModulusDegreeOfBase * length()))
            );

            int blockLen = (base == 2) ? integralModulusDegreeOfBase - 1 : integralModulusDegreeOfBase;
            int last = int(numStrView.length());        // last variable is not included into segment [0, len - 1]
            int first = cutOffLeftBorder<int>(last - blockLen, 0);
            LongUnsigned<length()> iBase = 1;
            int subInt;

            std::fill(begin(), end(), zeroIntegral());
            while (last - first > 0) {
                auto sub = numStrView.substr(size_t(first), size_t(last) - size_t(first));
                std::from_chars(sub.data(), sub.data() + sub.size(), subInt, base);

                subInt = std::abs(subInt);
                auto jBase = iBase;
                while (subInt > 0) {
                    (*this) += LongUnsigned<1>(subInt % base) * jBase;
                    jBase *= LongUnsigned<1>(base);
                    subInt /= base;
                }

                iBase *= special::pow< LongUnsigned<1>, int, LongUnsigned<length()> >
                    (LongUnsigned<1>(base), last - first);
                last -= blockLen;
                first = cutOffLeftBorder<int>(first - blockLen, 0);
            }
        }
    }

    //------------Arithmetic Operations-------------//

    template <LengthType length>
    auto LongUnsigned<length>::shiftLeft(unsigned int count)
        -> const_reference
    {
        // 1    0    - indices
        // 1234 5678 - number
        if (count >= length() * integralModulusDegree()) {
            *this = 0;
        }
        else {
            auto& current = *this;
            int blocksShift = count / integralModulusDegree();
            int bitsShift = count % integralModulusDegree();    // 0 to 31
            for (int i = int(length()) - 1; i >= 0; i--) {
                auto high = (i - blocksShift < 0) ? 0 : (current[i - blocksShift] << bitsShift);
                auto less = (i - blocksShift - 1 < 0) ? 0 :
                    ((current[i - blocksShift - 1] >> (integralModulusDegree() - bitsShift - 1))
                        >> 1);      // INFO: this crutch must be because you cannot shift uint32_t >> 32 bits
                                    //       only 0 to 31.
                auto res = high | less;
                current[i] = res;
            }
        }
        return *this;
    }

    template <LengthType length>
    auto LongUnsigned<length>::shiftRight(unsigned int count)
        -> const_reference
    {
        if (count >= length() * integralModulusDegree()) {
            *this = 0;
        }
        else {
            auto& current = *this;
            int blocksShift = count / integralModulusDegree();
            int bitsShift = count % integralModulusDegree();    // 0 to 31
            for (size_type i = 0; i < length(); i++) {
                auto less = (i + blocksShift >= length())
                    ? 0 : (current[i + blocksShift] >> bitsShift);
                auto high = (i + blocksShift + 1 >= length())
                    ? 0 : ((current[i + blocksShift + 1] << (integralModulusDegree() - bitsShift - 1))
                        << 1);      // INFO: this crutch must be because you cannot shift uint32_t << 32 bits
                                    //       only 0 to 31.
                auto res = high | less;
                current[i] = res;
            }
        }
        return *this;
    }

    //----------------------------------------------------------------------------

    template <size_t length>
    string LongUnsigned<length>::to_string(unsigned int base) const {
        string res = "";
        LongUnsigned temp = *this;
        int i = 0;
        //constexpr size_t digitsCount = extra::getIntegralModulusDegree<rank>();
        do {
            auto pair = temp.divide(LongUnsigned(base));
            temp = pair.first;
            res += std::to_string((pair.second)[0]);
            i++;
        } while (temp > LongUnsigned<1>(0));
        return std::string(res.rbegin(), res.rend());
    }


    template <size_t length>
    std::ostream& operator<<(std::ostream & out, LongUnsigned<length> number) {
        return (out << number.to_string());
    }


}
