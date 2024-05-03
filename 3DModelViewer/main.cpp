#include <iostream>
#include <filesystem>

#include <XGREngine/Application.h>
#include <XGREngine/ThreadPool.h>

#include "AppStates/ModelViewState.h"

#include <XGR/helper/exception.h>

#include "AppDefinitions.h"

#include <imgui.h>


using namespace XGREngine;

int main(int argc, char** argv)
{
	try {

		// Create folder in appdata
		if (!std::filesystem::exists(APP_CONTENT_DIRECTORY))
			std::filesystem::create_directory(APP_CONTENT_DIRECTORY);

		Application::Get().OnInit("3DModelViewer", APP_VERSION, 2, VK_PRESENT_MODE_MAILBOX_KHR);

		std::string iniFile = APP_CONTENT_DIRECTORY + "ImGuiConfig.ini";
		auto io = &ImGui::GetIO();
		io->IniFilename = iniFile.c_str();

		Application::Get().SetState(std::unique_ptr<AppState>(new states::ModelViewState()));

		while (Application::Get().IsRunning()) {
			Application::Get().OnUpdateDraw();
		}

		Application::Get().OnDestroy();

	}
	catch (const std::error_code& ex) {
		std::string errorMsg =	"The application returned an error code!\n"
								"message: " + ex.message() + "\n"
								"code: " + std::to_string(ex.value()) + "\n"
								"\n"
								"info:\n"
								"APP_VERSION: " + std::to_string(APP_VERSION) + "\n"
								"ENGINE_VERSION: " + std::to_string(Application::ENGINE_VERSION);

		xgr::debug::ShowMSG("Framework", errorMsg.c_str(), xgr::debug::ICON_ERROR);
		return -1;
	}

	return 0;
}
