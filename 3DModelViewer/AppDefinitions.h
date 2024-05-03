#pragma once

#include <string>

#define APP_VERSION VK_MAKE_API_VERSION(0, 1, 0, 0)

#define APPDATA_DIRECTORY getenv("APPDATA")

#define APP_CONTENT_DIRECTORY std::string(APPDATA_DIRECTORY) + "/3DModelViewer/"
