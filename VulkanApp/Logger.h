#pragma once

#include "Singleton.h"
#include "LoggerLevel.h"
#include "NullOutput.h"

namespace util
{
	namespace log
	{
		struct LoggingReturnType;

		template<typename Level = Void, typename R = LoggingReturnType>
		class Logger
		{
		public:
			typedef R return_type;

			static constexpr return_type& logging() {
				return Obj<return_type::output_base_type>::obj();
			}
		private:

			template<typename log_t>
			struct Obj {
				static constexpr return_type& obj() {
					return Singleton<return_type>::instance();
				}
			};

			template<>
			struct Obj<NullOutput> {
				static constexpr return_type& obj() {
					return *reinterpret_cast<return_type*>(nullptr);
				}
			};
		};

		template<typename TT, typename M>
		static inline void output(TT& log_t, const M& message) {
			static_cast<TT::output_base_type&>(log_t) << message;
		}
	}

	template<typename T>
	inline log::LoggingReturnType& operator<< (log::LoggingReturnType& ret, const T& message) {
		log::output(ret, message);
		return ret;
	}

	struct Log
	{
		static const char endl = '\n';
		static const char tab  = '\t';

		template<typename Level>
		static constexpr typename log::Logger<Level>::return_type& emit() {
			return log::Logger<Level>::logging() << Level::severity() << Level::desc();
		}

		static constexpr log::Logger<>::return_type& emit() {
			return log::Logger<>::logging();
		}
	};
}

#define LOGGING_DEFINE_OUTPUT(BASE)				\
namespace util::log {							\
	struct LoggingReturnType : public BASE {	\
		typedef BASE output_base_type;			\
	};											\
}												\

#define LOGGING_DISABLE_LEVEL(LEVEL)									\
namespace util::log {													\
	template<>															\
	class Logger<LEVEL, LoggingReturnType>								\
	{																	\
	public:																\
		typedef NullOutput return_type;									\
		static constexpr return_type& logging() {						\
			return *reinterpret_cast<return_type*>(nullptr);			\
		}																\
	};																	\
}																		\

#define LOG_NEW_LINE util::Log::emit() << util::Log::endl;
#define LOG(LEVEL, TEXT) util::Log::emit<LEVEL>() << TEXT << util::Log::endl;

#define THROW(TEXT) throw std::runtime_error(std::string(TEXT) + util::Log::endl + '(' + __FILE__ + ':' + std::to_string(__LINE__) + ')');