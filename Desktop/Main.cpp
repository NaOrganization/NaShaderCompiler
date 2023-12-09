#include "Main.h"

#define DEFAULT_WINDOW_WIDTH 1920
#define DEFAULT_WINDOW_HEIGHT 1080
#define VERSION "v0.1"
#define APPLICATION_NAME "NaShaderCompiler Preview" " " VERSION
#define THREAD_COUNT 4

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Windows Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	FORMAT_LOG(Info, "Already start" APPLICATION_NAME);

	SingleInstance<ThreadPool>::Get(THREAD_COUNT)->Start();

	static bool isResetWindowSize = false;

	// Set Window's WndPoc
	SingleInstance<Application>::Get()->GetMainWindow().SetWindowsProcess([](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)->LRESULT {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;
		switch (msg)
		{
		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED)
				return 0;
			isResetWindowSize = true;
			SingleInstance<Application>::Get()->GetMainWindow().width = (UINT)LOWORD(lParam); // Queue resize
			SingleInstance<Application>::Get()->GetMainWindow().height = (UINT)HIWORD(lParam);
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		}
		return ::DefWindowProcW(hWnd, msg, wParam, lParam);
		});
	FORMAT_LOG(Info, "Already set application's WndProc");

	// Add DirectX11 and ImGui Create
	SingleInstance<Application>::Get()->GetMainWindow().AddCallback(Application::Window::CallbackPeriod::Create, [](Application::Window* window) {
		SingleInstance<Render>::Get()->Create(window->handl, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		//ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\CascadiaCode.ttf", 16.f);
		ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.f);
		ImGui_ImplWin32_Init(SingleInstance<Application>::Get()->GetMainWindow().handl);
		ImGui_ImplDX11_Init(SingleInstance<Render>::Get()->device, SingleInstance<Render>::Get()->context);
		});
	FORMAT_LOG(Info, "Already add callback for DirectX11 and ImGui Create");

	// Create Main Window
	SingleInstance<Application>::Get()->GetMainWindow().Create(TEXT(APPLICATION_NAME), DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
	FORMAT_LOG(Info, "Already create main window");

	// Add ImGui Render
	SingleInstance<Render>::Get()->AddCallback(Render::CallbackPeriod::UpdateBeforeSetRenderTargets, []() {
		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		SingleInstance<Content>::Get()->Render();
		ImGui::Render();
		});
	FORMAT_LOG(Info, "Already add callback for ImGui Render and Content Render");

	// Add DirectX11 Render
	SingleInstance<Render>::Get()->AddCallback(Render::CallbackPeriod::UpdateAfterSetRenderTargets, []() {
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		});
	FORMAT_LOG(Info, "Already add callback for ImGui ImplDX11 Render");

	// Add Update
	SingleInstance<Application>::Get()->GetMainWindow().AddCallback(Application::Window::CallbackPeriod::Update, [](Application::Window*) {
		if (isResetWindowSize)
		{
			SingleInstance<Render>::Get()->renderTargetView->Release();
			SingleInstance<Render>::Get()->swapChain->ResizeBuffers(0,
				SingleInstance<Application>::Get()->GetMainWindow().width,
				SingleInstance<Application>::Get()->GetMainWindow().height,
				DXGI_FORMAT_UNKNOWN, 0);
			SingleInstance<Render>::Get()->CreateRenderTargetView();
		}
		SingleInstance<Render>::Get()->Update();
		});
	FORMAT_LOG(Info, "Already add callback for Render Update");

	// Join Message Loop
	FORMAT_LOG(Info, "Join Message Loop");
	SingleInstance<Application>::Get()->GetMainWindow().JoinMessageLoop();


	return 0;
}
