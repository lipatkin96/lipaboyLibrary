#pragma once

#include "functors.h"

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

using namespace functors_space;

using std::cout;
using std::endl;

template <bool isOwnContainer, class TRange>
struct GetRangeIter {};

//-----------------Stream class----------------------//

template <class TFunctor, class StorageInfo, class... Rest>
class Stream : public Stream<StorageInfo, Rest...> {
public:
    using size_type = size_t;
    using FunctorType = TFunctor;
    using SuperType = Stream<StorageInfo, Rest...>;
    using iterator = typename SuperType::outside_iterator;
    using SuperTypePtr = SuperType *;
    using ConstSuperTypePtr = const SuperType *;
    using ValueType = typename SuperType::ValueType;
    using RangeType = typename SuperType::RangeType;
    using OwnContainerType = typename SuperType::OwnContainerType;
    using OwnContainerTypePtr = typename SuperType::OwnContainerTypePtr;
    using ActionSignature = void(Stream*);
    using ActionType = std::function<ActionSignature>;

    template <class Functor>
    struct ExtendedStreamType {
        using type = Stream<Functor, FunctorType, StorageInfo, Rest...>;
    };
    using ResultValueType = typename TFunctor::template RetType<typename SuperType::ResultValueType>::type;

    template <typename, typename, typename...> friend class Stream;

    //----------------------Constructors----------------------//
protected:
    Stream(TFunctor const & functor, SuperType const & obj)
        : SuperType(obj), functor_(functor) {}
public:
    Stream(Stream const & obj)
        : SuperType(static_cast<SuperType>(obj)), functor_(obj.functor_), action_(obj.action_),
          preAction_(obj.preAction_) {}
    Stream(Stream&& obj)
        : SuperType(static_cast<SuperType>(obj)),
          functor_(std::move(obj.functor_)),
          action_(std::move(obj.action_)),
          preAction_(std::move(obj.preAction_)) {}

    //----------------------Methods API-----------------------//

    template <class Functor>
    auto operator| (filter<Functor> functor) -> typename ExtendedStreamType<filter<Functor> >::type {
        using ExtendedStream = typename ExtendedStreamType<filter<Functor> >::type;
        ExtendedStream obj(functor, *this);
        obj.action_ = [] (ExtendedStream* obj) {
            if (obj->range().isInfinite())
                throw std::logic_error("Infinite stream");
            obj->filter_(obj->getFunctor().functor());
            obj->action_ = [] (ExtendedStream*) {};
        };
        return std::move(obj);   // copy container (only once)
    }
    template <class Functor>
    auto operator| (map<Functor> functor)
        -> typename ExtendedStreamType<map<Functor> >::type
    {
        typename ExtendedStreamType<map<Functor> >::type obj(functor, *this);
        return std::move(obj);   // copy container (only once)
    }
    auto operator| (get functor) -> typename ExtendedStreamType<get>::type {
        using ExtendedStream = typename ExtendedStreamType<get>::type;
        ExtendedStream newStream(functor, *this);
        typename ExtendedStream::ActionType ExtendedStream::*pAction;

        if constexpr (isNoGetTypeBefore() && isGeneratorProducing())
            pAction = &ExtendedStream::preAction_;
        else
            pAction = &ExtendedStream::action_;

        newStream.*pAction = [] (ExtendedStream * obj) {
            auto border = obj->getFunctor().border();
            if (obj->range().isInfinite())
                obj->range().makeFinite(border);
            else if (border <= obj->size())
                obj->range().setSize(border);
            obj->preAction_ = [] (ExtendedStream*) {};
        };;

        return std::move(newStream);   // copy container (only once)
    }
    auto operator| (group functor) -> typename ExtendedStreamType<group>::type {
        typename ExtendedStreamType<group>::type obj(functor, *this);
        return std::move(obj);   // copy container (only once)
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

    //-------------------Terminated operations-----------------//

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
        initSlider();
        if (hasNext()) {
            auto result = nextElem();
            for (; hasNext();)
                result += nextElem();
            return result;
        }
        return ResultValueType();
    }
    ResultValueType operator| (nth&& nthObj) {
        doPreliminaryActions();
        initSlider();
        for (size_type i = 0; i < nthObj.index() && hasNext(); i++)
            nextElem();
        return nextElem();
    }
    vector<ResultValueType> operator| (to_vector&&)
    {
        using ToVectorType = vector<ResultValueType>;
        doPreliminaryActions();
        if (range().isInfinite())
            throw std::logic_error("Infinite stream");
        ToVectorType toVector;
        for (size_type i = 0; i < size(); i++)
            toVector.push_back(getElem(i));
        return std::move(toVector);
    }

            //-----------------Tools-------------------//
protected:
    static constexpr bool isOwnContainer() {
        return (TFunctor::metaInfo == FILTER || SuperType::isOwnContainer());
    }
    static constexpr bool isNoGetTypeBefore() {
        return (TFunctor::metaInfo != GET && SuperType::isNoGetTypeBefore());
    }
    static constexpr bool isNoGroupBefore() {
        return (TFunctor::metaInfo != GROUP && SuperType::isNoGroupBefore());
    }
    static constexpr bool isGeneratorProducing() {
        return SuperType::isGeneratorProducing();
    }
protected:
    ResultValueType getElem(size_type index) const { return getElem<isOwnContainer()>(index); }
    template <bool isOwnContainer_>
    ResultValueType getElem(size_type index) const {
        if constexpr (TFunctor::metaInfo == GROUP) {
            if constexpr (static_cast<ConstSuperTypePtr>(this)->isNoGroupBefore()) {
                auto partSize = functor_.partSize();
                // must be so (isOwnContainer_ instead of isOwnContainer()) because client who call that
                // method and derived from that class may have another value of isOwnContainer!
                auto iter = range().template ibegin<isOwnContainer_>();
                std::advance(iter, partSize * index);
                ResultValueType part;
                auto bindedFunctors = bindFunctors();
                auto endIter = range().template iend<isOwnContainer_>();
                for (size_type i = 0; i < partSize && iter != endIter; i++)
                    part.push_back(bindedFunctors(*(iter++)));
                return std::move(part);
            }
            else {
                auto partSize = functor_.partSize();
                ResultValueType part;
                for (size_type i = partSize * index; i < partSize * (index + 1) && i < size(); i++) {
                    part.push_back(static_cast<ConstSuperTypePtr>(this)->template getElem<isOwnContainer_>(i));
                }
                return std::move(part);
            }
        }
        else if constexpr (TFunctor::metaInfo == MAP) {
            return std::move(functor_(static_cast<ConstSuperTypePtr>(this)->template getElem<isOwnContainer_>(index)));
        }
        else {   // magic recursive expanding list of template methods
            return std::move(static_cast<ConstSuperTypePtr>(this)->template getElem<isOwnContainer_>(index));
        }
    }

public:
    //-----------------Slider API--------------//

    void initSlider() { initSlider<isOwnContainer()>(); }
    template <bool isOwnContainer_>
    void initSlider() { static_cast<SuperTypePtr>(this)->template initSlider<isOwnContainer_>(); }
    ResultValueType nextElem() { return nextElem<isOwnContainer()>(); }
    template <bool isOwnContainer_>
    ResultValueType nextElem() {
        // Template Parameter Explaination:
        // must be so (isOwnContainer_ instead of isOwnContainer())
        // because client who call this
        // method and derived from that class may have the another value of isOwnContainer()
        if constexpr (TFunctor::metaInfo == MAP)
            return functor_(static_cast<SuperTypePtr>(this)->template nextElem<isOwnContainer_>());
        else if constexpr (TFunctor::metaInfo != GROUP)
            return static_cast<SuperTypePtr>(this)->template nextElem<isOwnContainer_>();
        else {  // Hint: if (is Group)
            auto partSize = functor_.partSize();
            ResultValueType part;
            size_type i = 0;
            for (; i < partSize && hasNext(); i++)
                part.push_back(static_cast<SuperTypePtr>(this)->template nextElem<isOwnContainer_>());

            return std::move(part);
        }
    }
    bool hasNext() const { return hasNext<isOwnContainer()>(); }
    template <bool isOwnContainer_>
    bool hasNext() const { return static_cast<ConstSuperTypePtr>(this)->template hasNext<isOwnContainer_>(); }

    //-----------------Slider API Ends--------------//

private:

    // used only for one optimization
    decltype(auto) bindFunctors() const {
        if constexpr (TFunctor::metaInfo == MAP)
            return std::bind(this->functor_, static_cast<ConstSuperTypePtr>(this)->bindFunctors());
        else
            return static_cast<ConstSuperTypePtr>(this)->bindFunctors();
    }

protected:
    // For calling all the actions into stream (must be called into terminated operations)
    void doPreliminaryActions() {
        preAction_(this);
        static_cast<SuperTypePtr>(this)->doPreliminaryActions();
        action_(this);
    }
    ValueType getContainerElement(size_type index) {
        return applyFunctors(range().template get<isOwnContainer()>(index));
    }
    RangeType & range() { return static_cast<SuperTypePtr>(this)->range(); }
    const RangeType & range() const { return static_cast<ConstSuperTypePtr>(this)->range(); }

public:
    size_type size() const {
        if constexpr (TFunctor::metaInfo == GROUP)
            return (static_cast<ConstSuperTypePtr>(this)->size() + functor_.partSize() - 1) / functor_.partSize();
        else
            return static_cast<ConstSuperTypePtr>(this)->size();
    }
    TFunctor const & getFunctor() const { return functor_; }

protected:
    TFunctor functor_;
    // TODO: add getter/setter
    ActionType action_ = [] (Stream*) {};
    ActionType preAction_ = [] (Stream*) {};

protected:
    template <class TFilterFunctor>
    void filter_(TFilterFunctor functor) {
        if constexpr (TFunctor::metaInfo == FILTER) {
            // what is need (preliminary actions) have already executed because filter_ is preliminary action
            // doPreliminaryActions(); // infinitive recursive
            OwnContainerTypePtr pNewContainer = range().makeContainer();
            for (size_type i = 0; i < size(); i++)
            {
                auto elem = this->template getElem<SuperType::isOwnContainer()>(i);
                // TODO: move the element (only if copy from your own container)
                if (functor(std::move(elem)) == true) {
                    pNewContainer->push_back(range().template get<SuperType::isOwnContainer()>(i));
                }
            }
            range().setContainer(std::move(pNewContainer));
        }
    }

};

}
