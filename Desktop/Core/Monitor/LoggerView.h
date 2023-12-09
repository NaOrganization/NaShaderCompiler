#pragma once

RegisterWindow(LoggerView, true)
{
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->WorkSize.x - 780.f, ImGui::GetMainViewport()->WorkSize.y - 280.f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(730.f, 230.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Logger", opened);
	{
		static ImGuiTextFilter filter;
		ImGui::SetNextItemWidth(100.f);
		filter.Draw("Filter (inc, -exc)");
		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		{
			for (const auto& log : SingleInstance<Logger>::Get()->GetLogs())
			{
				if (!filter.PassFilter(log.ToString().c_str()))
				{
					continue;
				}
				ImGui::BeginGroup();
				ImGui::Text("[%s]", log.timestamp.c_str());
				ImGui::SameLine();
				ImVec4 levelColor = ImVec4();
				switch (log.level)
				{
				case Logger::Log::Level::Error:
					levelColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
					break;
				case Logger::Log::Level::Warning:
					levelColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
					break;
				case Logger::Log::Level::Info:
					levelColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
					break;
				case Logger::Log::Level::Succes:
					levelColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
					break;
				case Logger::Log::Level::Debug:
					levelColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
					break;
				default:
					break;
				}
				ImGui::TextColored(levelColor, "[%s]", log.LevelToString().c_str());
				ImGui::SameLine();
				ImGui::Text("%s", log.message.c_str());
				ImGui::EndGroup();
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
#ifdef _DEBUG
					ImGui::Text("File: %s", log.file.c_str());
					ImGui::Text("Line: %d", log.line);
#endif
					ImGui::Text("Timestamp: %s", log.timestamp.c_str());
					ImGui::Text("Level: %s", log.LevelToString().c_str());
					ImGui::Text("Message: %s", log.message.c_str());
					ImGui::EndTooltip();
			}
		}
			// Scroll to bottom
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();
	}
	ImGui::End();
}