#pragma once

#include "tools.h"

#include <memory>

namespace lipaboy_lib {

	namespace stream_space {

		namespace operators_space {

			using std::shared_ptr;

			// INFO: you can remove intermediate type (filter) because you can deduce type of elems from Predicate's
			//		 argument.

			//-------------------------------------------------------------------------------------//
			//--------------------------------Unterminated operation------------------------------//
			//-------------------------------------------------------------------------------------//

			template <class Predicate>
			struct filter : public FunctorHolder<Predicate>, 
				public TReturnSameType
			{
				static constexpr OperatorMetaTypeEnum metaInfo = FILTER;
				static constexpr bool isTerminated = false;
			public:
				filter(Predicate functor) : FunctorHolder<Predicate>(functor) {}
			};

			template <class Predicate, class T>
			struct filter_impl : public FunctorHolder<Predicate>, 
				public TReturnSameType
			{
				static constexpr OperatorMetaTypeEnum metaInfo = FILTER;
				static constexpr bool isTerminated = false;
			public:
				filter_impl(filter<Predicate> obj) 
					: FunctorHolder<Predicate>(obj.functor()) 
				{}

				// Opinion: difficult construction but without extra executions and computions

				template <class TSubStream>
				auto nextElem(TSubStream& stream) -> typename TSubStream::ResultValueType {
					// Info: I think you mustn't think about corrupting of branch predicator
					//		 because the gist of filtering is checking the conditions

					// ! calling hasNext() of current StreamType ! in order to skip unfilter elems
					hasNext(stream);
					resetSaves();
					auto temp = std::move(*pCurrentElem_);
					if (stream.hasNext()) {
						*pCurrentElem_ = std::move(stream.nextElem());
						hasNext(stream);
					}
					else
						pCurrentElem_ = nullptr;
					return std::move(temp);
				}

				template <class TSubStream>
				void incrementSlider(TSubStream& stream) { 
					hasNext(stream);
					if (stream.hasNext()) {
						*pCurrentElem_ = std::move(stream.nextElem());
						resetSaves();
						hasNext(stream);
					}
				}

				template <class TSubStream>
				bool hasNext(TSubStream& stream) {
					if (isSavesActual_)
						return curr_;

					if (pCurrentElem_ == nullptr) {
						if (!stream.hasNext()) {
							saveResult(false);
							return false;
						}
						pCurrentElem_ = std::make_shared<T>(std::move(stream.nextElem()));
						resetSaves();
					}

					bool isHasNext = false;
					do {
						// Info: We don't have the right to std::move the content of pCurrentElem_
						if (true == FunctorHolder<Predicate>::functor()(*pCurrentElem_)) {
							saveResult(true);
							return true;
						}
						if (isHasNext = stream.hasNext()) {
							*pCurrentElem_ = std::move(stream.nextElem());
							resetSaves();
						}
					} while (isHasNext);

					saveResult(false);
					return false;
				}

			private:
				void saveResult(bool result) {
					isSavesActual_ = true;
					curr_ = result;
				}
				void resetSaves() {
					isSavesActual_ = false;
				}

			private:
				shared_ptr<T> pCurrentElem_ = nullptr;
				bool curr_;
				bool isSavesActual_ = false;
			};

		}

		using operators_space::filter;
		using operators_space::filter_impl;

		template <class TStream, class Predicate>
		struct shortening::StreamTypeExtender<TStream, filter<Predicate> > {
			template <class T>
			using remref = std::remove_reference_t<T>;

			using type = typename remref<TStream>::template ExtendedStreamType<
				remref<filter_impl<Predicate, typename TStream::ResultValueType> > >;
		};

	}

}