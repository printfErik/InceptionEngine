#include <Inception.h>

class Prototype : public Inception::Application
{
public:
	Prototype()
	{

	}
	~Prototype()
	{

	}
};

Inception::Application* Inception::CreateApplication() {
	return new Prototype();
}