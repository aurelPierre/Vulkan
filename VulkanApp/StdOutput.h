#pragma once

#include <iostream>

#include "NonCopyable.h"

namespace util::log
{
	class StdOutput : public NonCopyable
	{
	public:
		template<typename T>
		void print(const T& message)
		{
			std::cout << message;
		}
	};

	template<typename T>
	StdOutput& operator<< (StdOutput& out, const T& message)
	{
		out.print(message);
		return out;
	}
}