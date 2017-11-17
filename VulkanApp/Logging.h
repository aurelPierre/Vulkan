#pragma once

#include "FileOutput.h"
#include "StdOutput.h"
#include "OutputLevelRunTimeSwitch.h"

#include "Logger.h"

#ifndef _DEBUG
	#define LOGGING_DISABLE
#endif

#ifdef LOGGING_DISABLE

LOGGING_DEFINE_SEVERITIES_MASK("000000")
LOGGING_DEFINE_OUTPUT(util::log::NullOutput)

#undef LOGGING_DEFINE_OUTPUT
#define LOGGING_DEFINE_OUTPUT(NAME)

#else

LOGGING_DEFINE_SEVERITIES_MASK("111111")

LOGGING_DEFINE_OUTPUT(util::log::OutputLevelRunTimeSwitch<util::log::StdOutput>)

#endif

