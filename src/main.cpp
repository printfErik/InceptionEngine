#include "InceptionEngine.h"
#include <filesystem>

int main(int argc, char** argv)
{
	std::filesystem::path executablePath(argv[0]);
	std::filesystem::path configFilePath = executablePath.parent_path().parent_path().parent_path().parent_path() / "configs\\Inception.ini";

	Inception::InceptionEngine* engine = new Inception::InceptionEngine();

	engine->initializeEngine(configFilePath);
	engine->startEngine();

	engine->stopEngine();

	return 0;
}