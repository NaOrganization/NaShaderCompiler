#pragma once

#include <Windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <filesystem>
#pragma comment(lib, "d3d11.lib")
namespace fs = std::filesystem;

#include "Dependence/ThreadPool.h"
#include "Dependence/CallbackManager.h"
#include "Dependence/SingleInstance.h"
#include "Dependence/Random.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "Dependence/ImGui/imgui.h"
#include "Dependence/ImGui/imgui_internal.h"
#include "Dependence/ImGui/backends/imgui_impl_dx11.h"
#include "Dependence/ImGui/backends/imgui_impl_win32.h"
#include "Dependence/ImGui/misc/cpp/imgui_stdlib.h"
#define STB_IMAGE_IMPLEMENTATION
#include "Dependence/stb_image.h"

#undef LoadImage

#include "Core/Controller/Application.h"
#include "Core/Controller/Content.h"
#include "Core/Controller/Render.h"
#include "Core/Controller/Logger.h"

#include "Core/Monitor/LoggerView.h"
#include "Core/Monitor/Previews.h"