#pragma once

#include <iostream>

namespace util::log
{
	class StdOutput 
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