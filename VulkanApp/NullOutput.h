#pragma once

namespace util::log
{
	class NullOutput : public NonCopyable {};

	template<typename T>
	inline log::NullOutput& operator<< (log::NullOutput& no, const T& message) {
		return no;
	}
}