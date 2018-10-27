#pragma once

#include "stream_extended.h"

#include <vector>
#include <functional>
#include <algorithm>
#include <iterator>
#include <type_traits>
#include <optional>
#include <typeinfo>

#include <iostream>

namespace lipaboy_lib {

	namespace stream {

		using std::vector;
		using std::pair;
		using std::string;
		using std::optional;

		using std::cout;


		//--------------------------Stream Base (specialization class)----------------------//

		template <class TIterator>
		class Stream<TIterator> {
		public:
			using T = typename std::iterator_traits<TIterator>::value_type;
			using ValueType = T;
			using size_type = size_t;
			using outside_iterator = TIterator;
			using GeneratorTypePtr = std::function<ValueType(void)>;

		public:
			template <class Functor>
			using ExtendedStreamType = Stream<Functor, TIterator>;

			using ResultValueType = ValueType;

		public:
			// INFO: this friendship means that all the Streams, which is extended from current,
			//		 can have access to current class method API.
			template <typename, typename...> friend class Stream;

		public:
			//----------------------Constructors----------------------//

			template <class OuterIterator>
			explicit
				Stream(OuterIterator begin, OuterIterator end) 
				: begin_(begin),
				end_(end)
			{}
			explicit
				Stream(std::initializer_list<T> init) 
				: begin_(init)
			{
				if constexpr (Stream::isInitializingListCreation())
						end_ = begin_.endIter();
			}
			explicit
				Stream(typename GeneratorTypePtr generator) 
				: begin_(generator), 
				end_() 
			{}

			//----------------------Methods API-----------------------//

		protected:
			static constexpr bool isNoGetTypeBefore() { return true; }
			static constexpr bool isGeneratorProducing() {
				return std::is_same_v<TIterator, ProducingIterator<ValueType> >;
			}
			static constexpr bool isInitializingListCreation() {
				return std::is_same_v<TIterator, InitializerListIterator<ValueType>
					//typename std::initializer_list<ValueType>::iterator
				>;
			}
			static constexpr bool isOutsideIteratorsRefer() {
				return !isGeneratorProducing() && !isInitializingListCreation();
			}

		public:
			inline static constexpr bool isInfinite() {
				return isGeneratorProducing() && isNoGetTypeBefore();
			}
			template <class TStream_>
			inline static constexpr void assertOnInfinite() {
				static_assert(!TStream_::isInfinite(),
					"Stream error: attempt to work with infinite stream");
			}
		protected:
			// Info:
			// illusion of protected (it means that can be replace on private)
			// (because all the variadic templates are friends
			// from current Stream to first specialization) (it is not a real inheritance)

			//-----------------Slider API--------------//
		public:
			ResultValueType nextElem() { 
				auto elem = *begin_;
				begin_++;
				return elem; 
			}
			bool hasNext() { return begin_ != end_; }
			void incrementSlider() { begin_++; }

			//-----------------Slider API Ends--------------//

		public:
			bool operator==(Stream const & other) const { return equals(other); }
			bool operator!=(Stream const & other) const { return !((*this) == other); }
		private:
			bool equals(Stream const & other) const { 
				return begin_ == other.begin_ 
					&& end_ == other.end_; 
			}

		private:
			TIterator begin_;
			TIterator end_;
		};

	}

	namespace short_stream {

		using std::vector;
		using std::pair;
		using std::string;

		using std::cout;


		//--------------------------Stream Base (specialization class)----------------------//

		template <class TIterator>
		class ShortStream<TIterator> {
		public:
			using T = typename std::iterator_traits<TIterator>::value_type;
			using ValueType = T;
			using size_type = size_t;
			using outside_iterator = TIterator;
			using GeneratorTypePtr = std::function<ValueType(void)>;

		public:
			template <class Functor>
			using ExtendedStreamType = ShortStream<Functor, TIterator>;

			using ResultValueType = std::optional<ValueType>;

		public:
			// INFO: this friendship means that all the Streams, which is extended from current,
			//		 can have access to current class method API.
			template <typename, typename...> friend class ShortStream;

		public:
			//----------------------Constructors----------------------//

			template <class OuterIterator>
			explicit
				ShortStream(OuterIterator begin, OuterIterator end)
				: begin_(begin),
				end_(end)
			{}
			explicit
				ShortStream(std::initializer_list<T> init)
				: begin_(init)
			{
				if constexpr (ShortStream::isInitializingListCreation())
					end_ = begin_.endIter();
			}
			explicit
				ShortStream(typename GeneratorTypePtr generator)
				: begin_(generator),
				end_()
			{}

			//----------------------Methods API-----------------------//

		protected:
			static constexpr bool isNoGetTypeBefore() { return true; }
			static constexpr bool isGeneratorProducing() {
				return std::is_same_v<TIterator, ProducingIterator<ValueType> >;
			}
			static constexpr bool isInitializingListCreation() {
				return std::is_same_v<TIterator, InitializerListIterator<ValueType>
					//typename std::initializer_list<ValueType>::iterator
				>;
			}
			static constexpr bool isOutsideIteratorsRefer() {
				return !isGeneratorProducing() && !isInitializingListCreation();
			}

		public:
			inline static constexpr bool isInfinite() {
				return isGeneratorProducing() && isNoGetTypeBefore();
			}
			template <class TStream_>
			inline static constexpr void assertOnInfinite() {
				static_assert(!TStream_::isInfinite(),
					"Stream error: attempt to work with infinite stream");
			}
		protected:
			// Info:
			// illusion of protected (it means that can be replace on private)
			// (because all the variadic templates are friends
			// from current Stream to first specialization) (it is not a real inheritance)

			//-----------------Slider API--------------//
		public:
			ResultValueType next() {
				if (begin_ != end_) {
					auto elem = *begin_;
					begin_++;
					return elem;
				}
				return std::nullopt;
			}
			void incrementSlider() { begin_++; }

			//-----------------Slider API Ends--------------//

		public:
			bool operator==(ShortStream const & other) const { return equals(other); }
			bool operator!=(ShortStream const & other) const { return !((*this) == other); }
		private:
			bool equals(ShortStream const & other) const {
				return begin_ == other.begin_
					&& end_ == other.end_;
			}

		private:
			TIterator begin_;
			TIterator end_;
		};

	}

	//------------------------------------------------------------------------------------------------------------//
	//------------------------------------------------------------------------------------------------------------//
	//------------------------------------------------------------------------------------------------------------//
	//------------------------------------------------------------------------------------------------------------//
	//------------------------------------------------------------------------------------------------------------//
	//------------------------------------------------------------------------------------------------------------//

	namespace fast_stream {

		using std::vector;
		using std::pair;
		using std::string;

		using std::cout;


		//--------------------------Stream Base (specialization class)----------------------//

		template <class TIterator>
		class Stream<TIterator> {
		public:
			using T = typename std::iterator_traits<TIterator>::value_type;
			using ValueType = T;
			using size_type = size_t;
			using outside_iterator = TIterator;
			using GeneratorTypePtr = std::function<ValueType(void)>;

		public:
			template <class Functor>
			using ExtendedStreamType = Stream<Functor, TIterator>;

			using ResultValueType = ValueType;

		public:
			// INFO: this friendship means that all the Streams, which is extended from current,
			//		 can have access to current class method API.
			template <typename, typename...> friend class Stream;

		public:
			//----------------------Constructors----------------------//

			template <class OuterIterator>
			explicit
				Stream(OuterIterator begin, OuterIterator end)
				: begin_(begin),
				end_(end)
			{}
			explicit
				Stream(std::initializer_list<T> init)
				: begin_(init)
			{
				if constexpr (Stream::isInitializingListCreation())
					end_ = begin_.endIter();
			}
			explicit
				Stream(typename GeneratorTypePtr generator)
				: begin_(generator),
				end_()
			{}

			//----------------------Methods API-----------------------//

		protected:
			static constexpr bool isNoGetTypeBefore() { return true; }
			static constexpr bool isGeneratorProducing() {
				return std::is_same_v<TIterator, ProducingIterator<ValueType> >;
			}
			static constexpr bool isInitializingListCreation() {
				return std::is_same_v<TIterator, InitializerListIterator<ValueType>
					//typename std::initializer_list<ValueType>::iterator
				>;
			}
			static constexpr bool isOutsideIteratorsRefer() {
				return !isGeneratorProducing() && !isInitializingListCreation();
			}

		public:
			inline static constexpr bool isInfinite() {
				return isGeneratorProducing() && isNoGetTypeBefore();
			}
			template <class TStream_>
			inline static constexpr void assertOnInfinite() {
				static_assert(!TStream_::isInfinite(),
					"Stream error: attempt to work with infinite stream");
			}
		protected:
			// Info:
			// illusion of protected (it means that can be replace on private)
			// (because all the variadic templates are friends
			// from current Stream to first specialization) (it is not a real inheritance)

			//-----------------Slider API--------------//
		public:
			ResultValueType nextElem() {
				auto elem = *begin_;
				begin_++;
				return elem;
			}
			bool hasNext() { return begin_ != end_; }
			void incrementSlider() { begin_++; }
			void initialize() {}

			//-----------------Slider API Ends--------------//

		public:
			bool operator==(Stream const & other) const { return equals(other); }
			bool operator!=(Stream const & other) const { return !((*this) == other); }
		private:
			bool equals(Stream const & other) const {
				return begin_ == other.begin_
					&& end_ == other.end_;
			}

		private:
			TIterator begin_;
			TIterator end_;
		};

	}

	

}
