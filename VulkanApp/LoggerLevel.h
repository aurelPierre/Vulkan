#pragma once

#include <bitset>

#define LOGGING_CREATE_LEVEL(LEVELNAME, SEVERITY, DESC)	\
namespace util::log {									\
	struct LEVELNAME {									\
		static Severity::severities severity() {		\
			return SEVERITY;							\
		}												\
														\
		static const char* desc() {						\
			return DESC;								\
		}												\
	};													\
}														\
typedef util::log::LEVELNAME Log ## LEVELNAME;			\

namespace util::log
{
	struct SeveritiesMask;

	struct Severity
	{
		enum severities
		{
			error = 1,
			warning,
			normal,
			info,
			debug,
			user
		};

		template<typename M = SeveritiesMask>
		struct Mask {
			static M _mask;

			static constexpr std::string mask() {
				return _mask._mask;
			}
		};

		severities _severity = user;
		std::bitset<user> _enabledSeverities = std::bitset<user>(Mask<>::mask());

		bool operator& (Severity severity) {
			return _enabledSeverities.test(severity._severity - 1);
		}

		Severity& operator= (severities severity) {
			_severity = severity;
			_enabledSeverities = std::bitset<user>(Mask<>::mask(), static_cast<size_t>(0), static_cast<size_t>(severity));
			return *this;
		}
	};

	template<typename M>
	M Severity::Mask<M>::_mask;
}

#define LOGGING_DEFINE_SEVERITIES_MASK(MASK)		\
namespace util::log {								\
	struct SeveritiesMask {							\
		const std::string _mask = MASK;				\
	};												\
}													\

LOGGING_CREATE_LEVEL(Void, util::log::Severity::normal, "")
LOGGING_CREATE_LEVEL(Info, util::log::Severity::info, "[ INFO ] ")
LOGGING_CREATE_LEVEL(Debug, util::log::Severity::debug, "[ DEBUG ] ")
LOGGING_CREATE_LEVEL(Warning, util::log::Severity::warning, "[ WARNING ] ")
LOGGING_CREATE_LEVEL(Error, util::log::Severity::error, "[ ERROR ] ")