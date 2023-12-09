#pragma once
#include <d3dcompiler.h>

class ShaderPreviewManager
{
public:
	class Preview;
	class Shader
	{
	public:
		ID3D11PixelShader* shader = nullptr;
		ID3D11SamplerState* sampler = nullptr;
		std::string id = "";
		std::string source = "";
		std::string name = "";
		std::string target = "ps_4_0";
		fs::path path = "";
		std::vector<std::string> previewIds = {};

		Shader() : id(Random::GetString(10))
		{}

		Shader(std::string source, std::string name, fs::path path) : id(Random::GetString(10)), source(source), name(name), path(path)
		{}

		bool IsSource() const
		{
			return source.size() > 0;
		}

		bool IsComplied() const
		{
			return shader != nullptr;
		}

		bool ComplieShader()
		{
			if (source.size() <= 0)
			{
				FORMAT_LOG(Warning, "[%s](id:%s) Shader source is empty!", name.c_str(), id.c_str());
				return false;
			}
			if (shader != nullptr)
			{
				shader->Release();
				shader = nullptr;
			}
			if (sampler != nullptr)
			{
				sampler->Release();
				sampler = nullptr;
			}
			ID3DBlob* blob = nullptr;
			ID3DBlob* error = nullptr;
			HRESULT hr =
				D3DCompile(source.c_str(), source.size(), nullptr, nullptr, nullptr, "main", target.c_str(), 0, 0, &blob, &error);
			if (FAILED(hr))
			{
				if (error != nullptr)
				{
					FORMAT_LOG(Warning, "[%s](id:%s) Failed to compile shader: %s", name.c_str(), id.c_str(), (char*)error->GetBufferPointer());
					error->Release();
				}
				else
				{
					FORMAT_LOG(Warning, "[%s](id:%s) Failed to compile shader: Unknown error (%x)", name.c_str(), id.c_str(), hr);
				}
				return false;
			}
			hr = SingleInstance<Render>::Get()->device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
			if (FAILED(hr))
			{
				FORMAT_LOG(Warning, "[%s](id:%s) Failed to create shader: %x", name.c_str(), id.c_str(), hr);
				blob->Release();
				return false;
			}
			blob->Release();
			D3D11_SAMPLER_DESC samplerDesc = {};
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			samplerDesc.MinLOD = 0;
			samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
			hr = SingleInstance<Render>::Get()->device->CreateSamplerState(&samplerDesc, &sampler);
			if (FAILED(hr))
			{
				FORMAT_LOG(Warning, "[%s](id:%s) Failed to create sampler: %x", name.c_str(), id.c_str(), hr);
				return false;
			}
			FORMAT_LOG(Info, "[%s](id:%s) Shader compiled successfully!", name.c_str(), id.c_str());
			return true;
		}

		// Process texture
		ID3D11ShaderResourceView* ProcessTexture(ID3D11ShaderResourceView* texture)
		{
			if (shader == nullptr || texture == nullptr)
				return nullptr;

			// Apply shader
			SingleInstance<Render>::Get()->context->PSSetShader(shader, nullptr, 0);

			// Set texture and sampler
			SingleInstance<Render>::Get()->context->PSSetShaderResources(0, 1, &texture);
			SingleInstance<Render>::Get()->context->PSSetSamplers(0, 1, &sampler);

			return texture;
		}

		void Destroy()
		{
			// If the shader is applied to any preview, remove the shader from the preview
			ShaderPreviewManager* manager = SingleInstance<ShaderPreviewManager>::Get();
			for (int i = 0; i < previewIds.size(); i++)
			{
				for (int j = 0; j < manager->previews.size(); j++)
				{
					if (manager->previews[j].id == previewIds[i])
					{
						manager->previews[j].shaderId = "";
						break;
					}
				}
			}
			if (shader != nullptr)
			{
				shader->Release();
				shader = nullptr;
			}
			if (sampler != nullptr)
			{
				sampler->Release();
				sampler = nullptr;
			}
		}

		bool operator==(const Shader& shader) const
		{
			return this->id == shader.id;
		}
	};
	class Preview
	{
	public:
		enum class DisplayMode
		{
			Stretch,
			Fit,
			Tile
		};
		ID3D11ShaderResourceView* texture = nullptr;
		std::string shaderId = "";
		std::string id = "";
		std::string name = "";
		fs::path path = "";
		int width = 0;
		int height = 0;
		bool opened = true;
		DisplayMode displayMode = DisplayMode::Fit;

		Preview() : id(Random::GetString(10))
		{}

		void ApplyShader(std::string shader)
		{
			this->shaderId = shader;
			ShaderPreviewManager* manager = SingleInstance<ShaderPreviewManager>::Get();
			for (int i = 0; i < manager->shaders.size(); i++)
			{
				if (manager->shaders[i].id == shader)
				{
					manager->shaders[i].previewIds.push_back(this->id);
					break;
				}
			}
		}

		bool IsApplyShader() const
		{
			return shaderId.size() > 0;
		}

		bool LoadImage()
		{
			unsigned char* data = stbi_load(path.string().c_str(), &width, &height, NULL, 4);
			if (data == NULL)
			{
				FORMAT_LOG(Warning, "[%s](id:%s) Failed to load image", name.c_str(), id.c_str());
				return false;
			}
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Width = width;
			desc.Height = height;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;

			ID3D11Texture2D* texture2d = NULL;
			D3D11_SUBRESOURCE_DATA subResource;
			subResource.pSysMem = data;
			subResource.SysMemPitch = desc.Width * 4;
			subResource.SysMemSlicePitch = 0;
			HRESULT hr = SingleInstance<Render>::Get()->device->CreateTexture2D(&desc, &subResource, &texture2d);
			if (FAILED(hr))
			{
				FORMAT_LOG(Warning, "[%s](id:%s) Failed to create texture: %x", name.c_str(), id.c_str(), hr);
				stbi_image_free(data);
				return false;
			}

			// Create shader resource view
			D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
			ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
			shaderResourceViewDesc.Format = desc.Format;
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDesc.Texture2D.MipLevels = desc.MipLevels;
			shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
			hr = SingleInstance<Render>::Get()->device->CreateShaderResourceView(texture2d, &shaderResourceViewDesc, &texture);
			if (FAILED(hr))
			{
				FORMAT_LOG(Warning, "[%s](id:%s) Failed to create shader resource view: %x", name.c_str(), id.c_str(), hr);
				texture2d->Release();
				stbi_image_free(data);
				return false;
			}

			// Release texture
			texture2d->Release();

			stbi_image_free(data);
			return texture != nullptr;
		}

		void RealeaseImage()
		{
			if (texture != nullptr)
			{
				texture->Release();
				texture = nullptr;
			}
		}

		bool operator==(const Preview& view) const
		{
			return this->id == view.id;
		}
	};

	std::vector<Preview> previews = {};
	std::vector<Shader> shaders = {};
	Preview background = Preview();

	void AddPreview(Preview view)
	{
		previews.push_back(view);
	}

	Shader GetShader(std::string id)
	{
		for (auto& shader : shaders)
		{
			if (shader.id == id)
				return shader;
		}
		return Shader();
	}

	void SetBackground(Preview view)
	{
		background = view;
	}
};

RegisterWindow(ShaderManager, true)
{
	ShaderPreviewManager* manager = SingleInstance<ShaderPreviewManager>::Get();
	static std::string destoryShaderId = "";
	ImGui::SetNextWindowPos(ImVec2(50.f, 50.f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(350.f, 230.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Shader Manager", opened);
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Count:");
		ImGui::SameLine();
		if (ImGui::ArrowButton("##left", ImGuiDir_Left) && manager->shaders.size() > 0)
		{
			// Delete the last shader in the list
			destoryShaderId = manager->shaders[manager->shaders.size() - 1].id;
		}
		ImGui::SameLine();
		// A text box without input for showing the count of shaders
		int count = manager->shaders.size();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
		ImGui::InputInt("##count", &count, 0, 0, ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine();
		if (ImGui::ArrowButton("##right", ImGuiDir_Right))
		{
			ImGui::OpenPopup("Create Shader");
		}
		if (ImGui::BeginPopupModal("Create Shader", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static ShaderPreviewManager::Shader shader = ShaderPreviewManager::Shader();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Path: %s", shader.path.string().c_str());
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Double click to select file");
				ImGui::Text("Full: %s", shader.path.string().c_str());
				ImGui::EndTooltip();

				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					OPENFILENAMEA ofn;
					CHAR szFile[MAX_PATH] = { 0 };
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = SingleInstance<Application>::Get()->GetMainWindow().handl;
					ofn.lpstrFilter = "Shader Files (*.hlsl)\0*.hlsl\0All Files (*.*)\0*.*\0";
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					ofn.lpstrDefExt = "hlsl";
					if (GetOpenFileNameA(&ofn))
					{
						shader.path = ofn.lpstrFile;
						shader.name = shader.path.filename().string();
						std::ifstream file(shader.path, std::ios::in);
						if (!file.is_open())
						{
							FORMAT_LOG(Warning, "[%s](id:%s) Failed to open shader file", shader.name.c_str(), shader.id.c_str());
							return;
						}
						shader.source = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
					}
				}

			}
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Name:");
			ImGui::SameLine();
			ImGui::InputText("##name", &shader.name);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Target:");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##target", shader.target.c_str()))
			{
				if (ImGui::Selectable("ps_4_0", shader.target == "ps_4_0"))
				{
					shader.target = "ps_4_0";
				}
				if (ImGui::Selectable("ps_5_0", shader.target == "ps_5_0"))
				{
					shader.target = "ps_5_0";
				}
				ImGui::EndCombo();
			}
			if (ImGui::Button("Create"))
			{
				if (!shader.IsSource())
				{
					shader = ShaderPreviewManager::Shader(R"(
					struct PS_INPUT
					{
					    float4 pos : SV_POSITION;
					    float4 col : COLOR0;
					    float2 uv  : TEXCOORD0;
					};
					
					sampler sampler0 : register(s0);
					Texture2D texture0 : register(t0);
					
					float4 main(PS_INPUT input) : SV_Target
					{
					    float4 sampledColor = texture0.Sample(sampler0, input.uv);
					    float4 out_col = input.col * sampledColor;
					    return out_col;
					}
					)", "Default.hlsl", "");
				}
				// 当vector添加新元素时，如果大小不够，会重新分配内存，导致之前的指针失效
				// 需要给原本preview已经绑定了地址的shader重新绑定地址
				manager->shaders.push_back(shader);
				shader = ShaderPreviewManager::Shader();

				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				shader = ShaderPreviewManager::Shader();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::Separator();
		ImGui::BeginChild("Shaders", ImVec2(0, 0), false);
		for (auto& shader : manager->shaders)
		{
			ImGui::PushID(shader.id.c_str());
			ImGui::SeparatorText((shader.name + "(id:" + shader.id + ")").c_str());
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Path: %s", shader.path.string().c_str());
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Target:");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##target", shader.target.c_str()))
			{
				if (ImGui::Selectable("ps_4_0", shader.target == "ps_4_0"))
				{
					shader.target = "ps_4_0";
				}
				if (ImGui::Selectable("ps_5_0", shader.target == "ps_5_0"))
				{
					shader.target = "ps_5_0";
				}
				ImGui::EndCombo();
			}
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Source: %s", shader.IsSource() ? "Has" : "No");
			ImGui::SameLine();
			if (ImGui::Button("Reload"))
			{
				if (shader.path.string().size() > 0)
				{
					std::ifstream file(shader.path, std::ios::in);
					if (!file.is_open())
					{
						FORMAT_LOG(Warning, "[%s](id:%s) Failed to open shader file", shader.name.c_str(), shader.id.c_str());
						ImGui::PopID();
						continue;
					}
					shader.source = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				}
			}
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Compiled: %s", shader.IsComplied() ? "Yes" : "No");
			ImGui::SameLine();
			if (ImGui::Button("Destroy"))
			{
				destoryShaderId = shader.id;
			}
			ImGui::SameLine();
			if (ImGui::Button("(Re-)Complie"))
			{
				shader.ComplieShader();
			}
			if (ImGui::Button("Compile with reloading souce") && shader.source.size() > 0)
			{
				if (shader.path.string().size() > 0)
				{
					std::ifstream file(shader.path, std::ios::in);
					if (!file.is_open())
					{
						FORMAT_LOG(Warning, "[%s](id:%s) Failed to open shader file", shader.name.c_str(), shader.id.c_str());
						ImGui::PopID();
						continue;
					}
					shader.source = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
				}
				shader.ComplieShader();
			}

			ImGui::SameLine();
			if (ImGui::Button("Delete"))
			{
				destoryShaderId = shader.id;
				ImGui::PopID();
				continue;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
		if (destoryShaderId.size() > 0)
		{
			ImGui::OpenPopup("Destory Shader");
		}
		if (ImGui::BeginPopupModal("Destory Shader", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure to destory shader?");
			ImGui::Separator();
			// Show the previews which are using the shader
			ImGui::Text("Here are previews which are using the shader:");
			for (auto& preview : manager->previews)
			{
				if (preview.shaderId == destoryShaderId)
				{
					ImGui::Text("(id: %s) %s", preview.id.c_str(), preview.name.c_str());
				}
			}
			ImGui::Separator();
			if (ImGui::Button("Yes"))
			{
				// Destory the shader
				for (int i = 0; i < manager->shaders.size(); i++)
				{
					if (manager->shaders[i].id == destoryShaderId)
					{
						manager->shaders[i].Destroy();
						manager->shaders.erase(manager->shaders.begin() + i);
						break;
					}
				}
				destoryShaderId = "";
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("No"))
			{
				destoryShaderId = "";
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
	ImGui::End();
}

RegisterWindow(PreviewManager, true)
{
	ShaderPreviewManager* manager = SingleInstance<ShaderPreviewManager>::Get();
	ImGui::SetNextWindowPos(ImVec2(50.f, 320.f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(400.f, 280.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Preview Manager", opened);
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Count:");
		ImGui::SameLine();
		if (ImGui::ArrowButton("##left", ImGuiDir_Left) && manager->previews.size() > 0)
		{
			// Delete the last view in the list
			manager->previews.pop_back();
		}
		ImGui::SameLine();
		// A text box without input for showing the count of views
		int count = manager->previews.size();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
		ImGui::InputInt("##count", &count, 0, 0, ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine();
		if (ImGui::ArrowButton("##right", ImGuiDir_Right))
		{
			ImGui::OpenPopup("Create Preview");
		}
		if (ImGui::BeginPopupModal("Create Preview", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static ShaderPreviewManager::Preview preview = ShaderPreviewManager::Preview();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Path: %s", preview.path.string().c_str());
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Double click to select file");
				ImGui::Text("Full: %s", preview.path.string().c_str());
				ImGui::EndTooltip();

				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					OPENFILENAMEA ofn;
					CHAR szFile[MAX_PATH] = { 0 };
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = SingleInstance<Application>::Get()->GetMainWindow().handl;
					ofn.lpstrFilter = "Image Files (*.png, *.jpg)\0*.png;*.jpg\0All Files (*.*)\0*.*\0";
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					ofn.lpstrDefExt = "png";
					if (GetOpenFileNameA(&ofn))
					{
						preview.path = ofn.lpstrFile;
						preview.name = preview.path.filename().string();
						preview.RealeaseImage();
						preview.LoadImage();
					}
				}
			}
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Name:");
			ImGui::SameLine();
			ImGui::InputText("##name", &preview.name);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Shader:");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##shader", preview.IsApplyShader() ? manager->GetShader(preview.shaderId).name.c_str() : "None"))
			{
				if (ImGui::Selectable("None", preview.shaderId.size() <= 0))
				{
					preview.shaderId = "";
				}
				for (auto& shader : manager->shaders)
				{
					if (!shader.IsSource() || !shader.IsComplied())
						continue;
					if (ImGui::Selectable(shader.name.c_str(), preview.shaderId == shader.id))
					{
						preview.ApplyShader(shader.id);
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::Button("Create"))
			{
				manager->previews.push_back(preview);
				preview = ShaderPreviewManager::Preview();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				preview = ShaderPreviewManager::Preview();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::Separator();
		ImGui::BeginChild("Previews", ImVec2(0, 0), false);
		for (auto& view : manager->previews)
		{
			ImGui::PushID(view.id.c_str());
			ImGui::SeparatorText((view.name + "(id:" + view.id + ")").c_str());
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Path: %s", view.path.string().c_str());
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Double click to select file");
				ImGui::Text("Full: %s", view.path.string().c_str());
				ImGui::EndTooltip();
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					OPENFILENAMEA ofn;
					CHAR szFile[MAX_PATH] = { 0 };
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = SingleInstance<Application>::Get()->GetMainWindow().handl;
					ofn.lpstrFilter = "Image Files (*.png, *.jpg)\0*.png;*.jpg\0All Files (*.*)\0*.*\0";
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					ofn.lpstrDefExt = "png";
					if (GetOpenFileNameA(&ofn))
					{
						view.path = ofn.lpstrFile;
						view.RealeaseImage();
						view.LoadImage();
					}
				}
			}
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Image: %s", view.texture == nullptr ? "No" : "Has");
			ImGui::SameLine();
			if (ImGui::Button("Reload"))
			{
				view.RealeaseImage();
				view.LoadImage();
			}
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Size: %dx%d", view.width, view.height);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Opened: %s", view.opened ? "Yes" : "No");
			ImGui::SameLine();
			ImGui::Checkbox("##opened", &view.opened);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Shader:");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##shader", view.IsApplyShader() ? manager->GetShader(view.shaderId).name.c_str() : "None"))
			{
				if (ImGui::Selectable("None", view.shaderId.size() <= 0))
				{
					view.shaderId = "";
				}
				for (auto& shader : manager->shaders)
				{
					if (!shader.IsSource() || !shader.IsComplied())
						continue;
					if (ImGui::Selectable(shader.name.c_str(), view.shaderId == shader.id))
					{
						view.ApplyShader(shader.id);
					}
				}
				ImGui::EndCombo();
			}
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Display Mode:");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##displayMode", view.displayMode == ShaderPreviewManager::Preview::DisplayMode::Stretch ? "Stretch" :
				view.displayMode == ShaderPreviewManager::Preview::DisplayMode::Fit ? "Fit" :
				view.displayMode == ShaderPreviewManager::Preview::DisplayMode::Tile ? "Tile" : "None"))
			{
				if (ImGui::Selectable("Stretch", view.displayMode == ShaderPreviewManager::Preview::DisplayMode::Stretch))
				{
					view.displayMode = ShaderPreviewManager::Preview::DisplayMode::Stretch;
				}
				if (ImGui::Selectable("Fit", view.displayMode == ShaderPreviewManager::Preview::DisplayMode::Fit))
				{
					view.displayMode = ShaderPreviewManager::Preview::DisplayMode::Fit;
				}
				if (ImGui::Selectable("Tile", view.displayMode == ShaderPreviewManager::Preview::DisplayMode::Tile))
				{
					view.displayMode = ShaderPreviewManager::Preview::DisplayMode::Tile;
				}
				ImGui::EndCombo();
			}
			if (ImGui::Button("Delete"))
			{
				manager->previews.erase(std::find(manager->previews.begin(), manager->previews.end(), view));
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

RegisterWindow(BackgroundManager, true)
{
	ImGui::SetNextWindowPos(ImVec2(50.f, 680.f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(350.f, 220.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Background Manager", opened);
	{
		ShaderPreviewManager* manager = SingleInstance<ShaderPreviewManager>::Get();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Path: %s", manager->background.path.string().c_str());
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("Double click to select file");
			ImGui::Text("Full: %s", manager->background.path.string().c_str());
			ImGui::EndTooltip();
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				OPENFILENAMEA ofn;
				CHAR szFile[MAX_PATH] = { 0 };
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = SingleInstance<Application>::Get()->GetMainWindow().handl;
				ofn.lpstrFilter = "Image Files (*.png, *.jpg)\0*.png;*.jpg\0All Files (*.*)\0*.*\0";
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				ofn.lpstrDefExt = "png";
				if (GetOpenFileNameA(&ofn))
				{
					manager->background.path = ofn.lpstrFile;
					manager->background.RealeaseImage();
					manager->background.LoadImage();
				}
			}
		}
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Image: %s", manager->background.texture == nullptr ? "No" : "Has");
		ImGui::SameLine();
		if (ImGui::Button("Reload"))
		{
			manager->background.RealeaseImage();
			manager->background.LoadImage();
		}
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Size: %dx%d", manager->background.width, manager->background.height);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Shader:");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##shader", manager->background.IsApplyShader() ? manager->GetShader(manager->background.shaderId).name.c_str() : "None"))
		{
			if (ImGui::Selectable("None", manager->background.shaderId.size() <= 0))
			{
				manager->background.shaderId = "";
			}
			for (auto& shader : manager->shaders)
			{
				if (!shader.IsSource() || !shader.IsComplied())
					continue;
				if (ImGui::Selectable(shader.name.c_str(), manager->background.shaderId == shader.id))
				{
					manager->background.ApplyShader(shader.id);
				}
			}
			ImGui::EndCombo();
		}
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Display Mode:");
		ImGui::SameLine();
		if (ImGui::BeginCombo("##displayMode", manager->background.displayMode == ShaderPreviewManager::Preview::DisplayMode::Stretch ? "Stretch" :
			manager->background.displayMode == ShaderPreviewManager::Preview::DisplayMode::Fit ? "Fit" :
			manager->background.displayMode == ShaderPreviewManager::Preview::DisplayMode::Tile ? "Tile" : "None"))
		{
			if (ImGui::Selectable("Stretch", manager->background.displayMode == ShaderPreviewManager::Preview::DisplayMode::Stretch))
			{
				manager->background.displayMode = ShaderPreviewManager::Preview::DisplayMode::Stretch;
			}
			if (ImGui::Selectable("Fit", manager->background.displayMode == ShaderPreviewManager::Preview::DisplayMode::Fit))
			{
				manager->background.displayMode = ShaderPreviewManager::Preview::DisplayMode::Fit;
			}
			if (ImGui::Selectable("Tile", manager->background.displayMode == ShaderPreviewManager::Preview::DisplayMode::Tile))
			{
				manager->background.displayMode = ShaderPreviewManager::Preview::DisplayMode::Tile;
			}
			ImGui::EndCombo();
		}
		if (ImGui::Button("Destory"))
		{
			manager->background.RealeaseImage();
			manager->background = ShaderPreviewManager::Preview();
		}
	}
	ImGui::End();
}

RegisterOverlay(Previews)
{
	ShaderPreviewManager* manager = SingleInstance<ShaderPreviewManager>::Get();
	for (auto& preview : manager->previews)
	{
		if (!preview.opened)
			continue;
		ImGui::Begin(std::string("Preview (" + std::to_string(std::distance(manager->previews.begin(), std::find(manager->previews.begin(), manager->previews.end(), preview)) + 1) + "):" + preview.id).c_str(), &preview.opened, ImGuiWindowFlags_NoSavedSettings);
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			if (preview.texture != nullptr)
			{
				if (preview.IsApplyShader())
				{
					drawList->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
						ShaderPreviewManager* manager = SingleInstance<ShaderPreviewManager>::Get();
						ShaderPreviewManager::Preview* view = (ShaderPreviewManager::Preview*)cmd->UserCallbackData;
						view->texture = manager->GetShader(view->shaderId).ProcessTexture(view->texture);
						}, &preview);
				}
				switch (preview.displayMode)
				{
				case ShaderPreviewManager::Preview::DisplayMode::Stretch:
					drawList->AddImage(preview.texture, ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y));
					break;
				case ShaderPreviewManager::Preview::DisplayMode::Fit:
				{
					float ratio = preview.width / (float)preview.height;
					float width = ImGui::GetWindowSize().x;
					float height = ImGui::GetWindowSize().y;
					if (width / height > ratio)
					{
						width = height * ratio;
					}
					else
					{
						height = width / ratio;
					}
					drawList->AddImage(preview.texture, ImVec2(ImGui::GetWindowPos().x + (ImGui::GetWindowSize().x - width) / 2, ImGui::GetWindowPos().y + (ImGui::GetWindowSize().y - height) / 2), ImVec2(ImGui::GetWindowPos().x + (ImGui::GetWindowSize().x + width) / 2, ImGui::GetWindowPos().y + (ImGui::GetWindowSize().y + height) / 2));
				}
				break;
				case ShaderPreviewManager::Preview::DisplayMode::Tile:
				{
					float ratio = preview.width / (float)preview.height;
					float width = ImGui::GetWindowSize().x;
					float height = ImGui::GetWindowSize().y;
					if (width / height > ratio)
					{
						width = height * ratio;
					}
					else
					{
						height = width / ratio;
					}
					for (int i = 0; i < ImGui::GetWindowSize().x / width + 1; i++)
					{
						for (int j = 0; j < ImGui::GetWindowSize().y / height + 1; j++)
						{
							drawList->AddImage(preview.texture, ImVec2(ImGui::GetWindowPos().x + i * width, ImGui::GetWindowPos().y + j * height), ImVec2(ImGui::GetWindowPos().x + (i + 1) * width, ImGui::GetWindowPos().y + (j + 1) * height));
						}
					}
				}
				break;
				default:
					break;
				}
				if (preview.IsApplyShader())
				{
					drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
				}
			}
		}
		ImGui::End();
	}
}

RegisterOverlay(Background)
{
	ShaderPreviewManager* manager = SingleInstance<ShaderPreviewManager>::Get();
	if (manager->background.texture != nullptr)
	{
		if (manager->background.IsApplyShader())
		{
			ImGui::GetBackgroundDrawList()->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
				ShaderPreviewManager* manager = SingleInstance<ShaderPreviewManager>::Get();
				ShaderPreviewManager::Preview* view = (ShaderPreviewManager::Preview*)cmd->UserCallbackData;
				view->texture = manager->GetShader(view->shaderId).ProcessTexture(view->texture);
				}, &manager->background);
		}
		switch (manager->background.displayMode)
		{
		case ShaderPreviewManager::Preview::DisplayMode::Stretch:
			ImGui::GetBackgroundDrawList()->AddImage(manager->background.texture, ImVec2(0, 0), ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
			break;
		case ShaderPreviewManager::Preview::DisplayMode::Fit:
		{
			float ratio = manager->background.width / (float)manager->background.height;
			float width = ImGui::GetIO().DisplaySize.x;
			float height = ImGui::GetIO().DisplaySize.y;
			if (width / height > ratio)
			{
				width = height * ratio;
			}
			else
			{
				height = width / ratio;
			}
			ImGui::GetBackgroundDrawList()->AddImage(manager->background.texture, ImVec2((ImGui::GetIO().DisplaySize.x - width) / 2, (ImGui::GetIO().DisplaySize.y - height) / 2), ImVec2((ImGui::GetIO().DisplaySize.x + width) / 2, (ImGui::GetIO().DisplaySize.y + height) / 2));
		}
		break;
		case ShaderPreviewManager::Preview::DisplayMode::Tile:
		{
			float ratio = manager->background.width / (float)manager->background.height;
			float width = ImGui::GetIO().DisplaySize.x;
			float height = ImGui::GetIO().DisplaySize.y;
			if (width / height > ratio)
			{
				width = height * ratio;
			}
			else
			{
				height = width / ratio;
			}
			for (int i = 0; i < ImGui::GetIO().DisplaySize.x / width + 1; i++)
			{
				for (int j = 0; j < ImGui::GetIO().DisplaySize.y / height + 1; j++)
				{
					ImGui::GetBackgroundDrawList()->AddImage(manager->background.texture, ImVec2(i * width, j * height), ImVec2((i + 1) * width, (j + 1) * height));
				}
			}
		}
		break;
		default:
			break;
		}
		if (manager->background.IsApplyShader())
		{
			ImGui::GetBackgroundDrawList()->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
		}
	}
}
