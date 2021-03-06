#pragma once

#include "tools.h"

#include <memory>

namespace lipaboy_lib::stream_space {

	namespace operators {

		using std::shared_ptr;

		// Contract rules : 
		//	1) filter_impl needs to store current element because
		//		when we call hasNext() it has pass the pack of elements
		//		to make sure the stream has next element indeed.

		// INFO: you can remove intermediate type (filter) because you can deduce type of elems from Predicate's
		//		 argument. BUT: you cannot pass lambda with auto parameter deducing.

		//-------------------------------------------------------------------------------------//
		//--------------------------------Unterminated operation------------------------------//
		//-------------------------------------------------------------------------------------//

		template <class Predicate>
		struct filter : FunctorHolder<Predicate>, TReturnSameType
		{
		public:
			filter(Predicate functor) : FunctorHolder<Predicate>(functor) {}
		};

		template <class Predicate, class T>
		struct filter_impl : FunctorHolder<Predicate>, TReturnSameType
		{
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
					*pCurrentElem_ = stream.nextElem();
					hasNext(stream);
				}
				else
					pCurrentElem_ = nullptr;
				return temp;
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
					if (FunctorHolder<Predicate>::functor()(*pCurrentElem_)) {
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
			bool curr_ = false;
			bool isSavesActual_ = false;
		};
			
	}

	using operators::filter;
	using operators::filter_impl;

	template <class TStream, class Predicate>
	struct shortening::StreamTypeExtender<TStream, filter<Predicate> > {
		template <class T>
		using remref = std::remove_reference_t<T>;

		using type = typename remref<TStream>::template ExtendedStreamType<
			remref<filter_impl<Predicate, typename TStream::ResultValueType> > >;
	};

}

