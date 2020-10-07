#include <icppch.h>
#include <Inception.h>

class ExampleLayer : public Inception::Layer
{
public:
	ExampleLayer()
		: Layer("Example!")
	{
	}

	void OnUpdate() override
	{
		ICP_INFO("ExampleLayer::Update");
	}

	void OnEvent(Inception::Event& event) override
	{
		ICP_TRACE("{0}", event);
	}
};


class Prototype : public Inception::Application
{
public:
	Prototype()
	{
		PushLayer(new ExampleLayer());
		PushOverLayer(new Inception::ImGuiLayer());
	}
	~Prototype()
	{

	}
};

Inception::Application* Inception::CreateApplication() {
	return new Prototype();
}