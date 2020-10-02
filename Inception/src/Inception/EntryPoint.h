#pragma once

#ifdef INCEPTION_PLATFORM_WINDOWS

extern Inception::Application* Inception::CreateApplication();

	int main(int argc, char** argv) {

		Inception::Log::Init();
		ICP_CORE_WARN("Init Core Logger");
		ICP_INFO("Init Client Logger");

		auto app = Inception::CreateApplication();
		app->Run();
		delete app;
	}
#endif // INCEPTION_PLATFORM_WINDOWS
