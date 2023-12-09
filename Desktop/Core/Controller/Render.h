#pragma once
#include <d3d11.h>

class Render
{
public:
	enum class CallbackPeriod
	{
		Create,
		UpdateBeforeSetRenderTargets,
		UpdateAfterSetRenderTargets,
		Destroy,
		None
	};
private:
	CallbackManager<CallbackPeriod> callbackManager = CallbackManager<CallbackPeriod>();
public:
	ID3D11Device* device = NULL;
	ID3D11DeviceContext* context = NULL;
	IDXGISwapChain* swapChain = NULL;
	ID3D11RenderTargetView* renderTargetView = NULL;

	void CreateRenderTargetView()
	{
		ID3D11Texture2D* backBuffer;
		swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
		device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
		backBuffer->Release();
	}

	void AddCallback(CallbackPeriod period, std::function<void()> callback)
	{
		callbackManager.AddCallback(period, callback);
	}

	void Create(HWND handl, int width, int height)
	{
		// Create device and context
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1, D3D11_SDK_VERSION, &device, NULL, &context);

		// Create swap chain
		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Width = width;
		swapChainDesc.BufferDesc.Height = height;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = handl;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Windowed = TRUE;
		D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, NULL, &context);

		// Create render target view
		CreateRenderTargetView();

		callbackManager.InvokeCallbacks(CallbackPeriod::Create);
	}


	void Update()
	{
		callbackManager.InvokeCallbacks(CallbackPeriod::UpdateBeforeSetRenderTargets);

		// Clear the back buffer
		float clearColor[] = { 255.0f, 255.0f, 255.0f, 255.0f };
		context->OMSetRenderTargets(1, &renderTargetView, nullptr);
		context->ClearRenderTargetView(renderTargetView, clearColor);
		callbackManager.InvokeCallbacks(CallbackPeriod::UpdateAfterSetRenderTargets);

		// Present the information rendered to the back buffer to the front buffer (the screen)
		swapChain->Present(0, 0);
	}

	void Destroy()
	{
		callbackManager.InvokeCallbacks(CallbackPeriod::Destroy);

		// Release the COM Objects we created
		renderTargetView->Release();
		swapChain->Release();
		context->Release();
		device->Release();
	}
};