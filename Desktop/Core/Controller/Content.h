#pragma once

class Content
{
public:
	class Window
	{
	public:
		const std::string path = "";
		std::function<void(bool*)> render = [](bool*) {};
		bool open = false;

		Window() {}
		Window(std::string path, std::function<void(bool*)> render, bool open = false) : path(path), open(open)
		{
			this->render = render;
			SingleInstance<Content>::Get()->AddWindow(this);
		}

		void Render()
		{
			render(&open);
		}
	};
	class Overlay
	{
	public:
		const std::string path = "";
		std::function<void()> render = []() {};

		Overlay() {}
		Overlay(std::string path, std::function<void()> render) : path(path)
		{
			this->render = render;
			SingleInstance<Content>::Get()->AddOverlay(this);
		}

		void Render()
		{
			render();
		}
	};
public:
	std::vector<Window*> windows = {};
	std::vector<Overlay*> overlays = {};
public:
	void AddWindow(Window* window)
	{
		windows.push_back(window);
	}
	void AddOverlay(Overlay* overlay)
	{
		overlays.push_back(overlay);
	}
	Overlay* GetOverlay(std::string path)
	{
		for (auto& overlay : overlays)
		{
			if (overlay->path == path)
			{
				return overlay;
			}
		}
		return nullptr;
	}

	void SetWindowState(std::string path, int open = -1)
	{
		for (auto& window : windows)
		{
			if (window->path == path)
			{
				if (open == -1)
				{
					open = !window->open;
				}
				window->open = open;
			}
		}
	}

	void Render()
	{
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->WorkSize.x - 300.f, 50.f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(250.f, 150.f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Windows Controller");
		for (auto& window : windows)
		{
			ImGui::Checkbox(window->path.c_str(), &window->open);
		}
		ImGui::End();
		for (auto& window : windows)
		{
			if (window->open)
			{
				window->Render();
			}
		}
		for (auto& overlay : overlays)
		{
			overlay->Render();
		}
	}
};

#define RegisterWindow(path, state) void path(bool* opened); Content::Window path##Window(#path, path, state); void path(bool* opened)
#define RegisterOverlay(path) void path(); Content::Overlay path##Overlay(#path, path); void path()

