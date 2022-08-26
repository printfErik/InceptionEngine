#include "InceptionEngine.h"
#include <filesystem>

int main(int argc, char** argv)
{
	std::filesystem::path executablePath(argv[0]);
	std::filesystem::path configFilePath = executablePath.parent_path().parent_path().parent_path().parent_path().parent_path() / "configs\\Inception.ini";
	std::filesystem::path shaderDirPath = executablePath.parent_path().parent_path().parent_path().parent_path().parent_path() / "src\\shaders";

	Inception::InceptionEngine* engine = new Inception::InceptionEngine();

	engine->initializeEngine(shaderDirPath);
	engine->startEngine();

	engine->stopEngine();

	return 0;
}