#include "App.h"
#include "Singleton.h"

int main()
{
	try {
		util::Singleton<core::App>::instance().run();
	}
	catch (const std::runtime_error& e) {
		LOG(LogError, e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}