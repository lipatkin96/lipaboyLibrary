#pragma once

#include "tools.h"

#include <exception>
#include <optional>

namespace lipaboy_lib {

	namespace stream {

		namespace operators {

			//---------------------------------------------------------------------------------------------------//
			//-----------------------------------Terminated operation-------------------------------------------//
			//---------------------------------------------------------------------------------------------------//
			

			struct nth : TReturnSameType {
				using size_type = size_t;
				static constexpr OperatorMetaTypeEnum metaInfo = NTH;
				static constexpr bool isTerminated = true;
				
				template <class T>
				using RetType = std::optional<T>;

				nth(size_type count) : count_(count) {}

				template <class Stream_>
				auto apply(Stream_ & obj) -> RetType<typename Stream_::ResultValueType>
				{
					for (size_t i = 0; i < count() && obj.hasNext(); i++)
						obj.incrementSlider();
					if (!obj.hasNext())
						return std::nullopt;
					return std::move(obj.nextElem());
				}

				size_type count() const { return count_; }
			private:
				size_type count_;
			};

			//-------------------------------------------------------------//
			//--------------------------Apply API--------------------------//
			//-------------------------------------------------------------//

		}

	}

}

