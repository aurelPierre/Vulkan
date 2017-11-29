#pragma once

#include <iostream>
#include <fstream>

#include "NonCopyable.h"

namespace util::log
{
	class FileOutput : public NonCopyable
	{
	public:
		FileOutput() : _os(&_fb) {
			_fb.open("logs/log", std::ios::out);
		}

		~FileOutput() {
			_fb.close();
		}

		template<typename T>
		void print(const T& message) {
			_os << message;
		}

	private:
		std::filebuf _fb;
		std::ostream _os;
	};

	template<typename T>
	FileOutput& operator<< (FileOutput& out, const T& message) {
		out.print(message);
		return out;
	}
}