#pragma once

#include "NonCopyable.h"

namespace util
{
	template<typename T>
	class Singleton : public NonCopyable
	{
	public:
		constexpr static T& instance() { return _obj; }

		virtual ~Singleton() = default;

	private:
		static T _obj;

		Singleton() = default;
	};

	template<typename T>
	T Singleton<T>::_obj;
}