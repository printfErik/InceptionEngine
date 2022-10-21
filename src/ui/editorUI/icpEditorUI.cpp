#include "icpEditorUI.h"
#include "../../core/icpSystemContainer.h"
#include "../../scene/icpSceneSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpEditorUI::icpEditorUI()
{
	
}

icpEditorUI::~icpEditorUI()
{
	
}

void icpEditorUI::showEditorUI()
{
	IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!");

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			//ShowExampleMenuFile();
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Debug Tool"))
		{
			if (ImGui::TreeNode("Save files"))
			{
				static char buf1[64] = "";
				ImGui::InputText("Out File Path", buf1, 64);

				if (ImGui::Button("Save default scene to File") && std::strlen(buf1) != 0)
				{
					g_system_container.m_sceneSystem->saveScene(buf1);
				}

				ImGui::TreePop();
			}

			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}



INCEPTION_END_NAMESPACE