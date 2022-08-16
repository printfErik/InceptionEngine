#include "InceptionEngine.h"

int main()
{
	Inception::InceptionEngine* engine = new Inception::InceptionEngine();

	engine->initializeEngine();
	engine->startEngine();

	engine->stopEngine();

	return 0;
}