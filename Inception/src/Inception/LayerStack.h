#pragma once
#include "Inception/Core.h"
#include "Inception/Layer.h"

#include <vector>

namespace Inception
{
	class INCEPTION_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverLayer(Layer* overlayer);
		void PopLayer(Layer* layer);
		void PopOverLayer(Layer* overlayer);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }

	private:
		std::vector<Layer*> m_Layers;
		std::vector<Layer*>::iterator m_LayerInsert;
	};
}


