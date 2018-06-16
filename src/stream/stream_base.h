#pragma once

#include "stream_extended.h"

#include <vector>
#include <functional>
#include <algorithm>
#include <iterator>
#include <type_traits>

#include <typeinfo>

#include <iostream>

namespace stream_space {

using std::vector;
using std::pair;
using std::string;

using namespace std::placeholders;

using std::cout;
using std::endl;


//--------------------------Stream specialization class----------------------//

namespace {
template <class T> using OwnContainerTypeWithoutValueType = vector<T>;
}

template <class StorageInfo, class OutsideIterator>
class Stream<StorageInfo, OutsideIterator> {
public:
    using T = typename std::iterator_traits<OutsideIterator>::value_type;
    using ValueType = T;
    using size_type = size_t;
    using outside_iterator = OutsideIterator;
    class RangeType;
    using OwnContainerType = typename RangeType::OwnContainerType;
    using OwnContainerTypePtr = typename RangeType::OwnContainerTypePtr;   // TODO: make it unique (it is not easy)
    using OwnIterator = typename OwnContainerType::iterator;

    // TODO: make unique_ptr
    using GeneratorTypePtr = std::function<T(void)>;

    template <class Functor>
    struct ExtendedStreamType {
        using type = Stream<Functor, StorageInfo, OutsideIterator>;
    };
    using ResultValueType = ValueType;

    template <typename, typename, typename...> friend class Stream;

public:
    //----------------------Constructors----------------------//

    template <class OuterIterator>
    Stream(OuterIterator begin, OuterIterator end) : range_(begin, end) {}
    Stream(std::initializer_list<T> init) : range_(init) {}
    Stream(const Stream& obj) : range_(obj.range_) {}
    Stream(Stream&& obj) : range_(std::move(obj.range_)) {}
    Stream(GeneratorTypePtr generator) : range_(generator) {}

    //----------------------Methods API-----------------------//

    // TODO: put off this methods into "global" function operators (for move-semantics of *this)

    template <class Predicate>
    auto operator| (filter<Predicate> functor) -> typename ExtendedStreamType<filter<Predicate> >::type
    {
        using ExtendedStream = typename ExtendedStreamType<filter<Predicate> >::type;
        ExtendedStream newStream(functor, *this);
        // you can't constraint the lambda only for this because the object will be changed after moving
        newStream.action_ = [] (ExtendedStream* obj) {
            if (obj->range().isInfinite())
                throw std::logic_error("Infinite stream");
            obj->filter_(obj->getFunctor().functor());
            obj->action_ = [] (ExtendedStream*) {};
        };
        return std::move(newStream);   // copy container (only once)
    }
    template <class Transform>
    auto operator| (map<Transform> functor) -> typename ExtendedStreamType<map<Transform> >::type {
        typename ExtendedStreamType<map<Transform> >::type newStream(functor, *this);
        return std::move(newStream);   // copy container (only once)
    }
    auto operator| (get functor) -> typename ExtendedStreamType<get>::type {
        using ExtendedStream = typename ExtendedStreamType<get>::type;
        ExtendedStream newStream(functor, *this);
        newStream.preAction_ =              // preAction_ -> is important
                                            // (because before generating the elements you must set the size)
            [] (ExtendedStream * obj)
            {
                auto border = obj->getFunctor().border();
                if (obj->range().isInfinite())
                    obj->range().makeFinite(border);
                else if (border <= obj->size())
                    obj->range().setSize(border);
                obj->preAction_ = [] (ExtendedStream*) {};
            };
        return std::move(newStream);   // copy container (only once)
    }
    auto operator| (group functor) -> typename ExtendedStreamType<group>::type {
        typename ExtendedStreamType<group>::type newStream(functor, *this);
        return std::move(newStream);   // copy container (only once)
    }
    auto operator| (skip&& skipObj) -> typename ExtendedStreamType<skip>::type {
        using ExtendedStream = typename ExtendedStreamType<skip>::type;
        ExtendedStream newStream(skipObj, *this);
        newStream.action_ = [] (ExtendedStream* obj) {
            obj->range().template moveBeginIter<ExtendedStream::isOwnContainer()>(obj->functor_.index());
            obj->action_ = [] (ExtendedStream*) {};
        };
        return std::move(newStream);   // copy container (only once)
    }

    //-----------Terminated operations------------//

    std::ostream& operator| (print_to&& printer) {
        doPreliminaryActions();
        for (initSlider(); hasNext(); )
            printer.ostream() << nextElem() << printer.delimiter();
        return printer.ostream();
    }
    template <class Accumulator, class IdenityFn>
    auto operator| (reduce<Accumulator, IdenityFn>&& reduceObj)
        -> typename reduce<Accumulator, IdenityFn>::
            template IdentityRetType<ResultValueType>::type
    {
        using RetType = typename reduce<Accumulator, IdenityFn>::
            template IdentityRetType<ResultValueType>::type;
        doPreliminaryActions();
        initSlider();
        if (hasNext()) {
            auto result = reduceObj.identity(nextElem());
            for ( ; hasNext(); )
                result = reduceObj.accum(result, nextElem());
            return result;
        }
        return RetType();
    }
    ResultValueType operator| (sum&&) {
        doPreliminaryActions();
        auto result = getElem(0);
        for (size_type i = 1; i < size(); i++)
            result += getElem(i);
        return result;
    }
    ResultValueType operator| (nth&& nthObj) {
        doPreliminaryActions();
        return getElem(nthObj.index());
    }
    vector<ValueType> operator| (to_vector&&) {
        doPreliminaryActions();
        if (range().isInfinite())
            throw std::logic_error("Infinite stream");
        return vector<ValueType>(range().template ibegin<isOwnContainer()>(),
                                 range().template iend<isOwnContainer()>());;
    }

    //------------------Additional methods---------------//

    size_type size() const { return range_.size(); }

public:
    RangeType & range() { return range_; }
    const RangeType & range() const { return range_; }
protected:
    static constexpr bool isOwnContainer() {
        return StorageInfo::info == INITIALIZING_LIST
                || StorageInfo::info == GENERATOR;
    }
    static constexpr bool isNoGetTypeBefore() {
        return true;
    }
    static constexpr bool isNoGroupBefore() {
        return true;
    }
    static constexpr bool isGeneratorProducing() {
        return StorageInfo::info == GENERATOR;
    }

protected:
    // Info:
    // illusion of protected (it means that can be replace on private)
    // (because all the variadic templates are friends
    // from current Stream to first specialization) (it is not a real inheritance)

    void doPreliminaryActions() { range().doPreliminaryActions(); }
protected:
    ValueType getElem(size_type index) const { return getElem<isOwnContainer()>(index); }
    template <bool isOwnContainer_>
    ValueType getElem(size_type index) const {
        return this->range().template get<isOwnContainer_>(index);
    }

    //-----------------Slider API--------------//

    void initSlider() { initSlider<isOwnContainer()>(); }
    template <bool isOwnContainer_>
    void initSlider() { range().template initSlider<isOwnContainer_>(); }
    ValueType nextElem() { return nextElem<isOwnContainer()>(); }
    template <bool isOwnContainer_>
    ValueType nextElem() { return range().template nextElem<isOwnContainer_>(); }
    bool hasNext() const { return hasNext<isOwnContainer()>(); }
    template <bool isOwnContainer_>
    bool hasNext() const { return range().template hasNext<isOwnContainer_>(); }

    //-----------------Slider API Ends--------------//

    decltype(auto) bindFunctors() const {
        return std::bind([] (ValueType const & a) -> ValueType const & { return a; }, _1);
    }

private:
    RangeType range_;
};

//-------------------Wrappers-----------------------//

template <class TIterator>
auto createStream(TIterator begin, TIterator end)
    -> Stream<IsOutsideIteratorsRefer, TIterator>
{
    return Stream<IsOutsideIteratorsRefer, TIterator>(begin, end);
}

template <class T>
decltype(auto) createStream(std::initializer_list<T> init)
{
    return Stream<IsInitializingListCreation, typename OwnContainerTypeWithoutValueType<T>::iterator>(init);
}

template <class T, class... Args>
decltype(auto) createStream(T elem, Args... args)
{
    return Stream<IsInitializingListCreation,
            typename OwnContainerTypeWithoutValueType<T>::iterator>({elem, args...});
}

template <class Generator>
decltype(auto) createStream(Generator&& generator)
{
    return Stream<IsGeneratorProducing,
            typename OwnContainerTypeWithoutValueType<
                typename std::result_of<Generator(void)>::type>::iterator>(std::forward<Generator>(generator));
}

}
