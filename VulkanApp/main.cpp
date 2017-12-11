#include "App.h"

#include "Logging.h"

int main()
{
	try {
		core::App::instance().run();
	}
	catch (const std::runtime_error& e) {
		LOG(LogError, e.what())
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}