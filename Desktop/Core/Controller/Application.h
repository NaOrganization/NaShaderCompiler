#pragma once

// Application for a window process
class Application
{
public:
	class Window
	{
	public:
		enum class CallbackPeriod
		{
			Create,
			Update,
			Destroy,
			None
		};
	private:
		CallbackManager<CallbackPeriod, Window*> callbackManager = CallbackManager<CallbackPeriod, Window*>();
	public:
		HWND handl = NULL;
		WNDPROC wndProc = DefWindowProcA;
		int width = 0;
		int height = 0;
		std::string title = "";
		bool close = false;

		void AddCallback(CallbackPeriod period, std::function<void(Window*)> callback)
		{
			callbackManager.AddCallback(period, callback);
		}

		void SetWindowsProcess(WNDPROC process)
		{
			wndProc = process;
		}

		int SetWidth(int newWidth)
		{
			width = newWidth;
			if (handl != NULL)
			{
				RECT rect;
				GetWindowRect(handl, &rect);
				MoveWindow(handl, rect.left, rect.top, width, height, true);
			}
			return width;
		}

		int SetHeight(int newHeight)
		{
			height = newHeight;
			if (handl != NULL)
			{
				RECT rect;
				GetWindowRect(handl, &rect);
				MoveWindow(handl, rect.left, rect.top, width, height, true);
			}
			return height;
		}

		void Close()
		{
			close = true;
			callbackManager.InvokeCallbacks(CallbackPeriod::Destroy, this);
		}

		void Minimize()
		{
			ShowWindow(handl, SW_MINIMIZE);
		}

		void Move(int offsetX, int offsetY)
		{
			if (handl != NULL)
			{
				RECT rect;
				GetWindowRect(handl, &rect);
				MoveWindow(handl, rect.left + offsetX, rect.top + offsetY, rect.right - rect.left, rect.bottom - rect.top, true);
			}
		}

		float* GetPosition()
		{
			float position[2] = { 0, 0 };
			if (handl != NULL)
			{
				RECT rect;
				GetWindowRect(handl, &rect);
				position[0] = (float)rect.left;
				position[1] = (float)rect.top;
			}
			return position;
		}

		std::string SetTitle(std::string newTitle)
		{
			title = newTitle;
			if (handl != NULL)
			{
				SetWindowTextA(handl, title.c_str());
			}
			return title;
		}

		Window& Create(const std::wstring& title, const int& width, const int& height)
		{
			// Set window size
			this->width = width;
			this->height = height;
			// Set window title
			this->title = std::string(title.begin(), title.end());
			// Register window class
			WNDCLASSEX wc = { 0 };
			wc.cbSize = sizeof(WNDCLASSEXA);
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = wndProc;
			wc.hInstance = GetModuleHandle(NULL);
			wc.hCursor = LoadCursor(NULL, IDC_ARROW);
			wc.lpszClassName = L"NaShaderComplier";
			if (!RegisterClassEx(&wc))
			{
				MessageBoxA(NULL, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
				return *this;
			}
			// Create window
			handl = CreateWindow(L"NaShaderComplier", title.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);

			if (handl == NULL)
			{
				MessageBoxA(NULL, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
				return *this;
			}

			// Show window
			ShowWindow(handl, SW_SHOWDEFAULT);
			UpdateWindow(handl);

			// Invoke callbacks
			callbackManager.InvokeCallbacks(CallbackPeriod::Create, this);
			return *this;
		}

		void JoinMessageLoop()
		{
			while (!close)
			{
				MSG msg;
				while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
				{
					::TranslateMessage(&msg);
					::DispatchMessage(&msg);
					if (msg.message == WM_QUIT)
						Close();
				}
				if (close)
					break;

				callbackManager.InvokeCallbacks(CallbackPeriod::Update, this);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	};
private:
	Window mainWindow = Window();

public:
	// Constructor
	Application() {}

	// Destructor
	~Application() {}

	// Getters
	Window& GetMainWindow() { return mainWindow; }
};