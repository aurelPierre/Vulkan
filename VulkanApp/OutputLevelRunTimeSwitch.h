#pragma once

#include "LoggerLevel.h"

namespace util::log
{
	template<typename Output>
	class OutputLevelRunTimeSwitch
	{
	public:
		Output output;

		bool isAllowed() {
			return _severity & _current;
		}

		void changeCurrentSeverity(const Severity::severities& severity) { 
			_current = severity; 
		}

	private:
		Severity _severity;
		Severity _current;
	};

	template<typename Output, typename M>
	OutputLevelRunTimeSwitch<Output>& operator<< (OutputLevelRunTimeSwitch<Output>& out, const M& message) {
		if(out.isAllowed())
			out.output << message;
		return out;
	}

	template<typename Output>
	OutputLevelRunTimeSwitch<Output>& operator<< (OutputLevelRunTimeSwitch<Output>& out, const Severity::severities& severity) {
		out.changeCurrentSeverity(severity);
		return out;
	}
}