﻿#include "UserInterface.hpp"
#include "Logger.hpp"
#include <Game.hpp>

UserInterface* UserInterface::ui_Instance = nullptr;

UserInterface::UserInterface()
{
	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void UserInterface::EngineEditor(FrameBuffer* fbo)
{
	cameraRef = Game::getInstance()->activeCamera;

	Start();
	Style();
	DockSpace();	

	if (showPostProccesed)
		GameViewport(fbo->postprocessedTexture, cameraRef, Game::getInstance()->activeScene);
	else
		GameViewport(fbo->framebufferTexture, cameraRef, Game::getInstance()->activeScene);
	Logger();
	Hierarchy(Game::getInstance()->activeScene);
	PropertiesPanel(Game::getInstance()->activeScene);
	Shutdown(Game::getInstance()->activeScene);
}

void UserInterface::Start()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void UserInterface::Init(SDL_Window* windowArg, void* glContextArg)
{
	IMGUI_CHECKVERSION();
	const char* glsl_version = "#version 100";
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO();
	int w, h;
	SDL_GetWindowSize(windowArg, &w, &h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	//io.DisplaySize = ImVec2(SCREEN_WIDTH, SCREEN_HEIGHT);
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;;
	io.FontGlobalScale = 0.95f;
	io.Fonts->AddFontFromFileTTF("Assets/fonts/MonaspaceNeonFrozen-Regular.ttf", 16.0f);
	io.Fonts->Build();
	ImGui_ImplSDL2_InitForOpenGL(windowArg, glContextArg);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void UserInterface::Resize(SDL_Window* windowArg)
{
	ImGuiIO& io = ImGui::GetIO();
	int w, h;
	SDL_GetWindowSize(windowArg, &w, &h);
	io.DisplaySize = ImVec2((float)w, (float)h);
}

void UserInterface::Shutdown(Scene* sceneRef)
{
	for (auto entity : entitiesToDestroy)
	{
		sceneRef->DestroyEntity(entity);
	}
	entitiesToDestroy.clear();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UserInterface::DockSpace()
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("DockSpace_Window", nullptr, window_flags);
	ImGui::PopStyleVar(2);

	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	HeaderBar();

	ImGui::End();
}

void UserInterface::GameViewport(GLuint fboID, Camera* maincam, Scene* sceneRef)
{
	ImGui::Begin("Viewport", nullptr, windowFlags);

	// Because I have docking this is required to know to draw imguizmo rect on top
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 cursorPos = ImGui::GetCursorPos();
	ImVec2 imagePos = ImVec2(windowPos.x + cursorPos.x, windowPos.y + cursorPos.y);
	ImVec2 imageSize = ImVec2(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	float buttonWidth = 40.0f;
	float buttonHeight = 25.0f;
	float spacing = ImGui::GetStyle().ItemSpacing.x;

	float totalWidth =   buttonWidth + spacing;
	float panelWidth = ImGui::GetContentRegionAvail().x;
	float startX = (panelWidth - totalWidth) * 0.5f;
	if (gameState == GameState::STOPPED)
	{
		ImGui::Checkbox("Play Maximized", &triggerMaximized); ImGui::SameLine();
		ImGui::Checkbox("Post Processed", &showPostProccesed); ImGui::SameLine();

		ImGui::SetCursorPosX(startX);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 255, 0, 255));
		if (ImGui::ImageButton("##Start", (ImTextureID)(uintptr_t)ResourceManager::playButton->ID, ImVec2(18, 18)))
		{
			gameState = GameState::PLAYING;
			sceneRef->Start();
			if (triggerMaximized)
				isViewportMaximized = true;
		}
		ImGui::PopStyleColor();
	}
	else
	{
		ImGui::SetCursorPosX(startX);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 0, 0, 255));
		if (ImGui::ImageButton("##Shutdown", (ImTextureID)(uintptr_t)ResourceManager::stopButton->ID, ImVec2(18, 18)))
		{
			gameState = GameState::STOPPED;
			sceneRef->Shutdown();
			isViewportMaximized = false;
		}
		ImGui::PopStyleColor();
	}

	// Render the framebuffer in imgui image
	ImGui::Image((ImTextureID)(uintptr_t)fboID, imageSize, ImVec2(0, 1), ImVec2(1, 0));

	// Set imguzimor rect
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(imagePos.x, imagePos.y + buttonHeight, imageSize.x, imageSize.y);

	// TODO: view and projection do not seem to work from main camera
	glm::mat4 viewMatrix = glm::lookAt(glm::vec3(0, 0, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 projMatrix = maincam->GetProjectionMatrix();//glm::perspective(glm::radians(45.0f), imageSize.x / imageSize.y, 0.1f, 100.0f);

	if (selectedHierarchyItem != entt::null && sceneRef->registry.any_of<TransformComponent>(selectedHierarchyItem))
	{
		auto& transform = sceneRef->registry.get<TransformComponent>(selectedHierarchyItem);
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), transform.Translation);

		// From glm mat4 to C array for ImGui
		float view[16], proj[16], model[16];
		memcpy(view, glm::value_ptr(viewMatrix), sizeof(view));
		memcpy(proj, glm::value_ptr(projMatrix), sizeof(proj));
		memcpy(model, glm::value_ptr(modelMatrix), sizeof(model));

		// Actual Gizmo
		ImGuizmo::Manipulate(view, proj, ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y, ImGuizmo::LOCAL, model); // ROTATE - SCALE
		if (ImGuizmo::IsUsing())
		{
			glm::mat4 updatedModel = glm::make_mat4(model);

			// Decompose the matrix to update Translation / Rotation / Scale
			glm::vec3 translation, rotation, scale;
			ImGuizmo::DecomposeMatrixToComponents(model, glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));

			auto& transform = sceneRef->registry.get<TransformComponent>(selectedHierarchyItem);
			transform.Translation = translation;
		}
	}

	ImGui::End();
}

void UserInterface::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
{
	ImGui::PushID(label.c_str());
	ImGui::BeginTable("Vec3Control", 4, ImGuiTableFlags_SizingFixedFit);
	ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
	ImGui::TableSetupColumn("X");
	ImGui::TableSetupColumn("Y");
	ImGui::TableSetupColumn("Z");

	ImGui::TableNextRow();

	// Label
	ImGui::TableSetColumnIndex(0);
	if (ImGui::Button(label.c_str())) values = glm::vec3(resetValue);

	// Common settings
	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::TableSetColumnIndex(1);
	ImGui::SetNextItemWidth(70); // Fill remaining column space
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 0.8f, 0.1f, 0.15f, 0.3f });
	ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopStyleColor();

	ImGui::TableSetColumnIndex(2);
	ImGui::SetNextItemWidth(70);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 0.2f, 0.7f, 0.2f, 0.3f });
	ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopStyleColor();

	ImGui::TableSetColumnIndex(3);
	ImGui::SetNextItemWidth(70);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 0.1f, 0.25f, 0.8f, 0.3f });
	ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::EndTable();
	ImGui::PopID();
}

void UserInterface::Style()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// Darker, more contrasted tones
	ImVec4 backgroundGray = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // almost black
	ImVec4 windowBgGray = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);    // darker than before
	ImVec4 baseGray = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);         // darker
	ImVec4 hoverGray = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);        // same hover
	ImVec4 activeGray = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);       // brighter when active

	// Text
	colors[ImGuiCol_Text] = ImVec4(1.f, 1.f, 1.f, 1.0f); // brighter text for readability

	// Backgrounds
	colors[ImGuiCol_WindowBg] = windowBgGray;
	colors[ImGuiCol_ChildBg] = windowBgGray;
	colors[ImGuiCol_PopupBg] = windowBgGray;

	// Borders
	colors[ImGuiCol_Border] = baseGray;

	// Headers
	colors[ImGuiCol_Header] = baseGray;
	colors[ImGuiCol_HeaderHovered] = hoverGray;
	colors[ImGuiCol_HeaderActive] = activeGray;

	// Buttons
	colors[ImGuiCol_Button] = baseGray;
	colors[ImGuiCol_ButtonHovered] = hoverGray;
	colors[ImGuiCol_ButtonActive] = activeGray;

	// Frames
	colors[ImGuiCol_FrameBg] = baseGray;
	colors[ImGuiCol_FrameBgHovered] = hoverGray;
	colors[ImGuiCol_FrameBgActive] = activeGray;

	// Scrollbars
	colors[ImGuiCol_ScrollbarBg] = backgroundGray;
	colors[ImGuiCol_ScrollbarGrab] = baseGray;
	colors[ImGuiCol_ScrollbarGrabHovered] = hoverGray;
	colors[ImGuiCol_ScrollbarGrabActive] = activeGray;

	// Title bars
	colors[ImGuiCol_TitleBg] = windowBgGray;
	colors[ImGuiCol_TitleBgActive] = baseGray;
	colors[ImGuiCol_TitleBgCollapsed] = windowBgGray;

	// Resize grips
	colors[ImGuiCol_ResizeGrip] = baseGray;
	colors[ImGuiCol_ResizeGripHovered] = hoverGray;
	colors[ImGuiCol_ResizeGripActive] = activeGray;

	// Tabs
	colors[ImGuiCol_Tab] = baseGray;
	colors[ImGuiCol_TabHovered] = hoverGray;
	colors[ImGuiCol_TabActive] = activeGray;
	colors[ImGuiCol_TabUnfocused] = baseGray;
	colors[ImGuiCol_TabUnfocusedActive] = baseGray;

	// Menus
	colors[ImGuiCol_MenuBarBg] = windowBgGray;

	// Checkmarks
	colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); // visible light gray

	// Sliders
	colors[ImGuiCol_SliderGrab] = hoverGray;
	colors[ImGuiCol_SliderGrabActive] = activeGray;

	// Roundness
	style.WindowRounding = 1.0f;
	style.FrameRounding = 1.0f;
	style.ScrollbarRounding = 1.0f;
	style.GrabRounding = 1.0f;
	style.TabRounding = 1.0f;
}

void UserInterface::CreatureMenu(Creature* creatureArg)
{
		ImGui::Text("Leg Options:");
		ImGui::Checkbox("Gait", &creatureArg->gait);
		ImGui::SliderFloat("Leg Width", &creatureArg->legWidth, 1.f, 5.f);
		ImGui::SliderFloat("Snap Distance", &creatureArg->distanceToSnap, -150.f, 200.f);
		ImGui::SliderFloat("Forward Step", &creatureArg->forwardOffset, -150.f, 200.f);
		ImGui::SliderFloat("Otward Step", &creatureArg->outwardBias, 0.f, 1.f);

		ImGui::Text("Leg Count ", &creatureArg->legCounter);
		ImGui::SameLine(); if (ImGui::Button("+")) creatureArg->AddLeg();  ImGui::SameLine(); if (ImGui::Button("-")) creatureArg->RemoveLeg();

		for (size_t i = 0; i < creatureArg->legs.size(); ++i) {
			ImGui::PushID(static_cast<int>(i)); // Avoid ImGui ID collisions
			std::string legLabel = "Leg " + std::to_string(i);
			if (ImGui::TreeNode(legLabel.c_str())) {
				for (size_t j = 0; j < creatureArg->legs[i]->segments.size(); j++) 
				{
					std::string segmentLabel = "Segment " + std::to_string(j);
					if (ImGui::TreeNode(segmentLabel.c_str())) 
					{
						ImGui::SliderFloat("Leg Length", &creatureArg->legs[i]->segments[j]->lenght, 1.0f, 30.0f);
						ImGui::TreePop();
					}
				}

				ImGui::Text("Chain Segments Count ", &creatureArg->legs[i]->chainLenght);
				ImGui::SameLine(); if (ImGui::Button("+")) creatureArg->legs[i]->AddSegment();  ImGui::SameLine(); if (ImGui::Button("-")) creatureArg->legs[i]->RemoveSegment();

				ImGui::TreePop();
			}
			ImGui::PopID();
		}


		ImGui::Spacing();

		ImGui::Text("Body Options:");
		ImGui::SliderInt("Creature Lenght", &creatureArg->desiredLenght, 2, 30);
		ImGui::SliderInt("Segment Spacing", &creatureArg->desiredSegmentSpacing, 5, 50);

		while (creatureArg->segmentSpacing < creatureArg->desiredSegmentSpacing)
		{
			creatureArg->AddSpacing();
		}

		while (creatureArg->segmentSpacing > creatureArg->desiredSegmentSpacing)
		{
			creatureArg->RemoveSpacing();
		}

		for (size_t i = 0; i < creatureArg->segments.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i)); // Avoid ImGui ID collisions

			std::string label = "Segment " + std::to_string(i);
			if (ImGui::TreeNode(label.c_str()))
			{
				ImGui::SliderInt("Thickness", &creatureArg->segments[i]->segmentRadius, 0, 20);
				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		// Required Logic for Sliders to work
		while (creatureArg->creatureLenght < creatureArg->desiredLenght)
			creatureArg->IncreaseBodyLenght();

		while (creatureArg->creatureLenght > creatureArg->desiredLenght)
			creatureArg->DecreaseBodyLenght();

		ImGui::Spacing();
		ImGui::SliderFloat("Rotation Speed", &creatureArg->rotationSpeed, 0.f, 5.f);
		ImGui::SliderFloat("Dynamic Speed", &creatureArg->dynamicSpeed, 0.f, 200.f);
		ImGui::SliderFloat("Texture Scale", &creatureArg->zoomFactor, 0.f, 50.f);

		ImGui::SliderFloat2("Offset", &creatureArg->offset.x, 0.f, 50.f);

		ImGui::SliderFloat3("Secondary Color", &creatureArg->secondaryColor.x, 0.f, 1.f);
		ImGui::SliderFloat3("Primary Color", &creatureArg->primaryColor.x, 0.f, 1.f);

		ImGui::SliderFloat("Spot Size", &creatureArg->spotSize, -1.f, 1.f);
}

void UserInterface::CameraMenu(Creature* creatureArg, FrameBuffer* fbo, Camera* maincam)
{
	
	ImGui::Begin("Camera");

	if (ImGui::SliderFloat("Zoom", &maincam->zoom, 0.f, 5.f, "%.2f"))
	{
		glm::vec2 focus = glm::vec2(creatureArg->segments[0]->transform.position.x, creatureArg->segments[0]->transform.position.y);
		maincam->updateProjection();
	}
	ImGui::SliderFloat("Camera Offset", &maincam->distanceFromPlayer, 0.f, 5.f);
	ImGui::SliderFloat("Follow Speed", &maincam->followSpeed, 0.f, 5.f);

	ImGui::Checkbox("Post Processing:", &showPostProccesed);

	if (showPostProccesed)
	{
		ImGui::Checkbox("Show Scanline", &fbo->scanline);
		ImGui::SliderFloat("Chromatic Abberation", &fbo->abberationIntensity, 0.f, 5.f);
		ImGui::SliderFloat("Curve Left", &fbo->curveLeft, 0.f, 5.f);
		ImGui::SliderFloat("Curve Right", &fbo->curveRight, 0.f, 5.f);
		ImGui::SliderFloat("Vignette", &fbo->vignetteIntensity, 0.f, 5.f);
	}

	ImGui::End();
}

void UserInterface::Logger()
{
	ImGui::Begin("Logs");

	const auto& logs = Logger::GetMessages();

	for (const auto& entry : logs)
	{
		ImVec4 color;
		switch (entry.type)
		{
		case LogType::LOG_INFO:
			color = ImVec4(0.f, 1.0f, 0.f, 1.0f); // Green
			break;
		case LogType::LOG_ERROR:
			color = ImVec4(1.f, 0.f, 0.0f, 1.0f); // Red
			break;
		case LogType::LOG_WARNING:
			color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
			break;
		default:
			color = ImVec4(1, 1, 1, 1); // White
			break;
		}
		
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(entry.message.c_str());
		ImGui::PopStyleColor();
	}

	ImGui::End();
}

void UserInterface::Hierarchy(Scene* sceneRef)
{
	ImGui::Begin("Hierarchy");

	for (auto entity : sceneRef->registry.view<entt::entity>())
	{
		// Out of the entt id make one for IMGUI
		int id = static_cast<int>(entity);
		ImGui::PushID(id);

		// Get name of the Entity
		std::string label = "Entity" + std::to_string(id);
		if (sceneRef->registry.any_of<NameComponent>(entity)) 
		{
			auto& name = sceneRef->registry.get<NameComponent>(entity);
			label = name.name;
		}

		// Draw the selectable
		float availableWidth = ImGui::GetContentRegionAvail().x;
		float buttonWidth = 20.0f;
		float spacing = 5.0f; // Spacing between selectable and button

		// TODO: wrap this inside a Tree Node for scene graph functionality
		bool clicked = ImGui::Selectable(label.c_str(), selectedHierarchyItem == entity, 0, ImVec2(availableWidth - buttonWidth - spacing, 0));
		if (clicked) selectedHierarchyItem = entity;

		ImGui::PopID();

		// Delete BUTTON
		if (selectedHierarchyItem == entity)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			ImGui::SameLine();
			// align the button at the right
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spacing);
			if (ImGui::SmallButton("X"))
			{
				entitiesToDestroy.push_back(entity);
				if (selectedHierarchyItem == entity) selectedHierarchyItem = entt::null;
			}
			ImGui::PopStyleColor();
		}
	}

	ImGui::Spacing();
	ImGui::Spacing();
	float panelWidth = ImGui::GetContentRegionAvail().x;
	float buttonWidth = 100.0f, buttonHeight = 25.f;
	ImGui::SetCursorPosX((panelWidth - buttonWidth) * 0.5f);

	if (ImGui::Button("New Entity", ImVec2(buttonWidth, buttonHeight)))
		sceneRef->AddEntity("Test", "Test");

	ImGui::End();
}

void UserInterface::DrawEntity()
{

}

void UserInterface::PropertiesPanel(Scene* sceneRef)
{
	ImGui::Begin("Properties");

	if (selectedHierarchyItem != entt::null && !sceneRef->registry.valid(selectedHierarchyItem)) {
		selectedHierarchyItem = entt::null;
		ImGui::Text("No entity selected.");
		ImGui::End();
		return;
	}

	float buttonWidth = 100.0f, buttonHeight = 22.f;
	if (selectedHierarchyItem != entt::null && sceneRef->registry.valid(selectedHierarchyItem))
	{
		if (ImGui::CollapsingHeader("Name")) 
		{
			char buffer[64];
			auto& name = sceneRef->registry.get<NameComponent>(selectedHierarchyItem);
			strncpy_s(buffer, name.name.c_str(), sizeof(buffer));
			buffer[sizeof(buffer) - 1] = '\0';
			if (ImGui::InputText("Name ##", buffer, sizeof(buffer))) {
				name.name = buffer;
			}
		
			char buffer2[64];
			strncpy_s(buffer2, name.tag.c_str(), sizeof(buffer2));
			buffer2[sizeof(buffer2) - 1] = '\0';
			if (ImGui::InputText("Tag", buffer2, sizeof(buffer2))) {
				name.tag = buffer2;
			}
		}

		if (sceneRef->registry.any_of<TransformComponent>(selectedHierarchyItem)) 
		{
			auto& transform = sceneRef->registry.get<TransformComponent>(selectedHierarchyItem);

			if (ImGui::CollapsingHeader("Transform"))
			{
				ImGui::PushID(1);
					DrawVec3Control("Position", transform.Translation);
				ImGui::PopID();
				ImGui::PushID(2);
					DrawVec3Control("Rotation", transform.Rotation);
				ImGui::PopID();
				ImGui::PushID(3);
					DrawVec3Control(" Scale  ", transform.Scale);
				ImGui::PopID();

				ImGui::Spacing();
				float panelWidth = ImGui::GetContentRegionAvail().x;

				ImGui::PushID(4);
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGui::SetCursorPosX((panelWidth - buttonWidth) * 0.5f);
				if (ImGui::Button("Delete", ImVec2(buttonWidth, buttonHeight)))
				{
					sceneRef->registry.remove<TransformComponent>(selectedHierarchyItem);
				}
				ImGui::PopStyleColor();
				ImGui::Spacing();
				ImGui::PopID();
			}
		}

		if (sceneRef->registry.any_of<SpriteComponent>(selectedHierarchyItem))
		{
			auto& sprite = sceneRef->registry.get<SpriteComponent>(selectedHierarchyItem);

			if (ImGui::CollapsingHeader("Sprite"))
			{
				
				if (ImGui::Button("Browse"))
				{
					std::string path = OpenFileDialog(FILE_TYPE::IMAGE);
					if (!path.empty())
					{
						fileName = std::filesystem::path(path).filename().string();
						sprite.texturePath = path;
						sprite.texture = Texture2D();
						int width, height, nrChannels;
						unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
						sprite.texture.Generate(width, height, data);
						stbi_image_free(data);
					}
				}
				ImGui::SameLine(); ImGui::Text(fileName.c_str());

				ImGui::Spacing();
				float panelWidth = ImGui::GetContentRegionAvail().x;

				ImGui::PushID(5);
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGui::SetCursorPosX((panelWidth - buttonWidth) * 0.5f);
				if (ImGui::Button("Delete", ImVec2(buttonWidth, buttonHeight)))
				{
					sceneRef->registry.remove<SpriteComponent>(selectedHierarchyItem);
				}
				ImGui::PopStyleColor();
				ImGui::Spacing();
				ImGui::PopID();
			}
		}

		if (sceneRef->registry.any_of<ScriptComponent>(selectedHierarchyItem))
		{
			if (ImGui::CollapsingHeader("Script"))
			{
				if (ImGui::Button("Browse"))
				{
					std::string path = OpenFileDialog(FILE_TYPE::SCRIPT);
					if (!path.empty())
					{
						fileName = std::filesystem::path(path).filename().string();
					}
				}
				ImGui::SameLine(); ImGui::Text(fileName.c_str());

				ImGui::Spacing();
				float panelWidth = ImGui::GetContentRegionAvail().x;

				ImGui::PushID(5);
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGui::SetCursorPosX((panelWidth - buttonWidth) * 0.5f);
				if (ImGui::Button("Delete", ImVec2(buttonWidth, buttonHeight)))
				{
					sceneRef->registry.remove<ScriptComponent>(selectedHierarchyItem);
				}
				ImGui::PopStyleColor();
				ImGui::Spacing();
				ImGui::PopID();
			}
		}

		if (sceneRef->registry.any_of<Creature>(selectedHierarchyItem))
		{
			auto& creature = sceneRef->registry.get<Creature>(selectedHierarchyItem);

			if (ImGui::CollapsingHeader("Creature"))
			{
				CreatureMenu(&creature);

				ImGui::Spacing();
				float panelWidth = ImGui::GetContentRegionAvail().x;

				ImGui::PushID(4);
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
				ImGui::SetCursorPosX((panelWidth - buttonWidth) * 0.5f);
				if (ImGui::Button("Delete", ImVec2(buttonWidth, buttonHeight)))
				{
					sceneRef->registry.remove<Creature>(selectedHierarchyItem);
				}
				ImGui::PopStyleColor();
				ImGui::Spacing();
				ImGui::PopID();
			}
		}

		// ADD COMPONENT
		ImGui::Spacing();
		ImGui::Spacing();
		float popUpWidth = 200, popUpHeight = 100;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		buttonWidth = 140.0f, buttonHeight = 25.f;
		ImGui::SetCursorPosX((panelWidth - buttonWidth) * 0.5f);
		if(ImGui::Button("New Component", ImVec2(buttonWidth, buttonHeight)))
		{
			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGui::SetNextWindowPos(ImVec2(pos.x + (panelWidth - popUpWidth) * 0.5f, pos.y));
			ImGui::OpenPopup("AddComponentPopup");
		}

		ImGui::SetNextWindowSize(ImVec2(popUpWidth, popUpHeight));
		ImGui::Spacing();
		if (ImGui::BeginPopup("AddComponentPopup")) 
		{
			Entity entityWrapper(selectedHierarchyItem, sceneRef->registry);

			if (!sceneRef->registry.any_of<TransformComponent>(selectedHierarchyItem))
			{
				if (ImGui::MenuItem("Transform Component"))
				{
					entityWrapper.AddComponent<TransformComponent>(glm::vec3{ 0.f }, glm::vec3{ 0.f }, glm::vec3{ 0.f });
				}
			}
			if (!sceneRef->registry.any_of<SpriteComponent>(selectedHierarchyItem))
			{
				if (ImGui::MenuItem("Sprite Component"))
				{
					entityWrapper.AddComponent<SpriteComponent>();
				}
			}
			if (!sceneRef->registry.any_of<ScriptComponent>(selectedHierarchyItem))
			{
				if (ImGui::MenuItem("Script Component"))
				{
					entityWrapper.AddComponent<ScriptComponent>();
				}
			}
			if (!sceneRef->registry.any_of<Creature>(selectedHierarchyItem))
			{
				if (ImGui::MenuItem("Creature Component"))
				{
					auto& creature = entityWrapper.AddComponent<Creature>(15, cameraRef);
				}
			}
			ImGui::EndPopup();
		}
	}
	else 
		ImGui::Text("No entity selected.");

	ImGui::End();
}

void UserInterface::HeaderBar()
{
	if (ImGui::BeginMenuBar())
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

		if (ImGui::Button("Engine")) ImGui::OpenPopup("EnginePopup");
		ImGui::SetNextWindowSize(ImVec2(125, 60));
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::SetNextWindowPos(ImVec2(pos.x - 65, pos.y + 25));
		if (ImGui::BeginPopup("EnginePopup"))
		{
			if (ImGui::MenuItem("Build Game")) { }
			if (ImGui::MenuItem("Reload")) { }
			ImGui::EndPopup();
		}

		if (ImGui::Button("Scene")) ImGui::OpenPopup("ScenePopup");
		ImGui::SetNextWindowSize(ImVec2(85, 60));
		pos = ImGui::GetCursorScreenPos();
		ImGui::SetNextWindowPos(ImVec2(pos.x - 65, pos.y + 25));
		if (ImGui::BeginPopup("ScenePopup"))
		{
			if (ImGui::MenuItem("Save")) { Game::getInstance()->activeScene->Serialize(); }
			if (ImGui::MenuItem("Load")) { Game::getInstance()->activeScene->Deserialize(); }
			ImGui::EndPopup();
		}

		if (ImGui::Button("Create")) ImGui::OpenPopup("CreatePopup");
		ImGui::SetNextWindowSize(ImVec2(125, 60));
		pos = ImGui::GetCursorScreenPos();
		ImGui::SetNextWindowPos(ImVec2(pos.x - 65, pos.y + 25));
		if (ImGui::BeginPopup("CreatePopup"))
		{
			if (ImGui::MenuItem("New Script")) { }
			if (ImGui::MenuItem("New Scene ")) { }
			ImGui::EndPopup();
		}

		ImGui::PopStyleColor();
		ImGui::EndMenuBar();
	}
}

std::string UserInterface::OpenFileDialog(const FILE_TYPE typeArg)
{
	char filename[MAX_PATH] = "";

	OPENFILENAMEA ofn = { 0 };
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	if (typeArg == FILE_TYPE::SCRIPT)
		ofn.lpstrFilter = "Script Files\0*.lua;\0All Files\0*.*\0";
	else
		ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn))
		return std::string(filename);
	return ""; // Cancelled or failed
}