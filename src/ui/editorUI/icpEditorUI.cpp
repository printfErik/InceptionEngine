#include "icpEditorUI.h"
#include "../../core/icpSystemContainer.h"
#include "../../scene/icpSceneSystem.h"
#include "../../resource/icpResourceSystem.h"

INCEPTION_BEGIN_NAMESPACE

icpEditorUI::icpEditorUI()
{
	
}

icpEditorUI::~icpEditorUI()
{
	
}

// todo
bool checkFilePath(const std::string path)
{
	return true;
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

		if (ImGui::BeginMenu("Entity"))
		{
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Component"))
		{
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Debug Tool"))
		{
			if (ImGui::TreeNode("Save files"))
			{
				static char buf1[64] = "";
				ImGui::InputText("Out File Path", buf1, 64);

				if (ImGui::Button("Save default scene to File"))
				{
					if (std::strlen(buf1) == 0 || !checkFilePath(std::string()))
					{
						ImGui::SameLine();
						ImGui::Text("File Path is not Legal!");
					}
					else
					{
						g_system_container.m_sceneSystem->saveScene(buf1);
					}
				}
				ImGui::TreePop();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Resources"))
		{
			static char Path[64] = "";
			ImGui::InputText("Load Model File Path", Path, 64);
			if (ImGui::Button("Load Obj Files"))
			{
				if (std::strlen(Path) == 0 || !checkFilePath(std::string()))
				{
					ImGui::SameLine();
					ImGui::Text("File Path is not Legal!");
				}
				else
				{
					g_system_container.m_resourceSystem->loadObjModelResource(Path);
				}
			}

			static char ImagePath[64] = "";
			ImGui::InputText("Load Image File Path", ImagePath, 64);
			if (ImGui::Button("Load Img Files"))
			{
				if (std::strlen(ImagePath) == 0 || !checkFilePath(std::string()))
				{
					ImGui::SameLine();
					ImGui::Text("File Path is not Legal!");
				}
				else
				{
					g_system_container.m_resourceSystem->loadImageResource(ImagePath);
				}
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}



INCEPTION_END_NAMESPACE