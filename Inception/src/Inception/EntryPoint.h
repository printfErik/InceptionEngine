#pragma once

extern Inception::Application* Inception::CreateApplication();

#ifdef INCEPTION_PLATFORM_WINDOWS

	int main(int argc, char** argv) {

		printf("Welcome!\n");
		auto app = Inception::CreateApplication();
		app->Run();
		delete app;
	}
#endif // INCEPTION_PLATFORM_WINDOWS
