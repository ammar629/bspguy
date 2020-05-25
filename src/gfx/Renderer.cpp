#include "Renderer.h"
#include "ShaderProgram.h"
#include "primitives.h"
#include "VertexBuffer.h"
#include "shaders.h"

void error_callback(int error, const char* description)
{
	printf("GLFW Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

Renderer::Renderer() {
	if (!glfwInit())
	{
		printf("GLFW initialization failed\n");
		return;
	}

	glfwSetErrorCallback(error_callback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(1920, 1080, "bspguy", NULL, NULL);
	if (!window)
	{
		printf("Window creation failed\n");
		return;
	}

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	glewInit();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 130";
	ImGui_ImplOpenGL3_Init(glsl_version);

	vsync = true;
	smallFont = io.Fonts->AddFontFromFileTTF("../imgui/misc/fonts/Roboto-Medium.ttf", 20.0f);
	largeFont = io.Fonts->AddFontFromFileTTF("../imgui/misc/fonts/Roboto-Medium.ttf", 24.0f);

	io.ConfigWindowsMoveFromTitleBarOnly = true;

	bspShader = new ShaderProgram(g_shader_multitexture_vertex, g_shader_multitexture_fragment);
	bspShader->setMatrixes(&model, &view, &projection, &modelView, &modelViewProjection);
	bspShader->setMatrixNames(NULL, "modelViewProjection");

	colorShader = new ShaderProgram(g_shader_cVert_vertex, g_shader_cVert_fragment);
	colorShader->setMatrixes(&model, &view, &projection, &modelView, &modelViewProjection);
	colorShader->setMatrixNames(NULL, "modelViewProjection");
	colorShader->setVertexAttributeNames("vPosition", "vColor", NULL);

	g_render_flags = RENDER_TEXTURES | RENDER_LIGHTMAPS | RENDER_SPECIAL 
		| RENDER_ENTS | RENDER_SPECIAL_ENTS | RENDER_POINT_ENTS;
	showDebugWidget = true;
	showKeyvalueWidget = true;
	pickInfo.valid = false;

	fgd = new Fgd(g_game_path + "/svencoop/sven-coop.fgd");
	fgd->parse();

	pointEntRenderer = new PointEntRenderer(fgd, colorShader);
}

Renderer::~Renderer() {
	glfwTerminate();
}

void Renderer::renderLoop() {
	glfwSwapInterval(1);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	cameraOrigin.y = -50;

	float lastFrameTime = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		float frameDelta = glfwGetTime() - lastFrameTime;
		frameTimeScale = 0.05f / frameDelta;
		float fps = 1.0f / frameDelta;
		
		frameTimeScale = 144.0f / fps;

		lastFrameTime = glfwGetTime();

		cameraControls();

		float spin = glfwGetTime() * 2;
		model.loadIdentity();
		model.rotateZ(spin);
		model.rotateX(spin);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		setupView();
		bspShader->bind();
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		for (int i = 0; i < mapRenderers.size(); i++) {
			model.loadIdentity();
			bspShader->updateMatrixes();

			int highlightEnt = -1;
			if (pickInfo.valid && pickInfo.mapIdx == i) {
				highlightEnt = pickInfo.entIdx;
			}
			mapRenderers[i]->render(highlightEnt);
		}

		/*
		model.loadIdentity();
		colorShader->bind();

		drawLine(pickEnd + vec3(-32, 0, 0), pickEnd + vec3(32,0,0), { 255, 0, 0 });
		drawLine(pickEnd + vec3(0, -32, 0), pickEnd + vec3(0,32,0), { 255, 255, 0 });
		drawLine(pickEnd + vec3(0, 0, -32), pickEnd + vec3(0,0,32), { 0, 255, 0 });

		vec3 forward, right, up;
		makeVectors(cameraAngles, forward, right, up);
		//printf("DRAW %.1f %.1f %.1f -> %.1f %.1f %.1f\n", pickStart.x, pickStart.y, pickStart.z, pickDir.x, pickDir.y, pickDir.z);
		*/

		drawGui();

		glfwSwapBuffers(window);
	}

	glfwTerminate();
}

void Renderer::drawGui() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	drawMenuBar();

	drawFpsOverlay();

	if (showDebugWidget) {
		drawDebugWidget();
	}

	if (showKeyvalueWidget) {
		drawKeyvalueEditor();
	}

	// Rendering
	ImGui::Render();
	glViewport(0, 0, windowWidth, windowHeight);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::drawMenuBar() {
	ImGui::BeginMainMenuBar();

	if (ImGui::BeginMenu("Widgets"))
	{
		if (ImGui::MenuItem("Debug", NULL, showDebugWidget)) {
			showDebugWidget = !showDebugWidget;
		}
		if (ImGui::MenuItem("Keyvalue Editor", NULL, showKeyvalueWidget)) {
			showKeyvalueWidget = !showKeyvalueWidget;
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Render"))
	{
		if (ImGui::MenuItem("Textures", NULL, g_render_flags & RENDER_TEXTURES)) {
			g_render_flags ^= RENDER_TEXTURES;
		}
		if (ImGui::MenuItem("Lightmaps", NULL, g_render_flags & RENDER_LIGHTMAPS)) {
			g_render_flags ^= RENDER_LIGHTMAPS;
		}
		if (ImGui::MenuItem("Wireframe", NULL, g_render_flags & RENDER_WIREFRAME)) {
			g_render_flags ^= RENDER_WIREFRAME;
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Entities", NULL, g_render_flags & RENDER_ENTS)) {
			g_render_flags ^= RENDER_ENTS;
		}
		if (ImGui::MenuItem("Special", NULL, g_render_flags & RENDER_SPECIAL)) {
			g_render_flags ^= RENDER_SPECIAL;
		}
		if (ImGui::MenuItem("Special Entities", NULL, g_render_flags & RENDER_SPECIAL_ENTS)) {
			g_render_flags ^= RENDER_SPECIAL_ENTS;
		}
		if (ImGui::MenuItem("Point Entities", NULL, g_render_flags & RENDER_POINT_ENTS)) {
			g_render_flags ^= RENDER_POINT_ENTS;
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void Renderer::drawFpsOverlay() {
	ImVec2 window_pos = ImVec2(10.0f, 35.0f);
	ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f);
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
	if (ImGui::Begin("Overlay", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
	{
		ImGui::Text("%.0f FPS", ImGui::GetIO().Framerate);
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::MenuItem("VSync", NULL, vsync)) {
				vsync = !vsync;
				glfwSwapInterval(vsync ? 1 : 0);
			}
			ImGui::EndPopup();
		}
	}
	ImGui::End();
}

void Renderer::drawDebugWidget() {
	ImGui::SetNextWindowBgAlpha(0.75f);
	if (ImGui::Begin("Debug info", &showDebugWidget, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (pickInfo.valid) {
			Bsp* map = mapRenderers[pickInfo.mapIdx]->map;
			Entity* ent = map->ents[pickInfo.entIdx];
			BSPMODEL& model = map->models[pickInfo.modelIdx];
			BSPFACE& face = map->faces[pickInfo.faceIdx];

			if (ImGui::TreeNodeEx("Map", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Name: %s", map->name.c_str());
				ImGui::TreePop();
			}

			ImGui::Separator();

			if (ImGui::TreeNodeEx("Entity", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::TreePop();
			}


			ImGui::Separator();

			ImGui::Text("Face ID: %d", pickInfo.faceIdx);
			ImGui::Text("Plane ID: %d", face.iPlane);
		}
		else {
			ImGui::Text("Click on an object for debug info");
		}

	}
	ImGui::End();
}

void Renderer::drawKeyvalueEditor() {
	ImGui::SetNextWindowBgAlpha(0.75f);
	string title = "Keyvalue Editor";

	title += "###entwindow";

	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 100), ImVec2(FLT_MAX, windowHeight - 40));
	//ImGui::SetNextWindowContentSize(ImVec2(550, 0.0f));
	if (ImGui::Begin(title.c_str(), &showKeyvalueWidget, 0)) {
		if (pickInfo.valid) {
			Bsp* map = mapRenderers[pickInfo.mapIdx]->map;
			Entity* ent = map->ents[pickInfo.entIdx];
			BSPMODEL& model = map->models[pickInfo.modelIdx];
			BSPFACE& face = map->faces[pickInfo.faceIdx];

			if (ImGui::BeginTabBar("##tabs"))
			{
				if (ImGui::BeginTabItem("Smart Edit")) {
					ImGui::Dummy(ImVec2(0, 10));
					drawKeyvalueEditor_SmartEditTab(ent);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Flags")) {
					ImGui::Dummy(ImVec2(0, 10));
					drawKeyvalueEditor_FlagsTab(ent);
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Raw Edit")) {
					ImGui::Dummy(ImVec2(0, 10));
					drawKeyvalueEditor_RawEditTab(ent);
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();

		}
		else {
			ImGui::Text("No entity selected");
		}

	}
	ImGui::End();
}

void Renderer::drawKeyvalueEditor_SmartEditTab(Entity* ent) {
	string cname = ent->keyvalues["classname"];
	FgdClass* fgdClass = fgd->getFgdClass(cname);
	ImGuiStyle& style = ImGui::GetStyle();

	ImGui::PushFont(largeFont);
	ImGui::AlignTextToFramePadding();
	//ImGui::Text("Class:"); ImGui::SameLine();
	if (cname != "worldspawn") {
		if (ImGui::Button((" " + cname + " ").c_str()))
			ImGui::OpenPopup("classname_popup");
	}
	else {
		ImGui::Text(cname.c_str());
	}
	ImGui::PopFont();

	if (fgdClass != NULL) {
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted((fgdClass->description).c_str());
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}
	

	if (ImGui::BeginPopup("classname_popup"))
	{
		ImGui::Text("Change Class");
		ImGui::Separator();

		vector<FgdGroup>* targetGroup = &fgd->pointEntGroups;
		if (ent->getBspModelIdx() != -1) {
			targetGroup = &fgd->solidEntGroups;
		}

		for (int i = 0; i < targetGroup->size(); i++) {
			FgdGroup& group = targetGroup->at(i);

			if (group.classes.size() == 1) {
				if (ImGui::Selectable(group.classes[0]->name.c_str())) {

				}
			}
			else if (ImGui::BeginMenu(group.groupName.c_str())) {
				for (int k = 0; k < group.classes.size(); k++) {
					ImGui::MenuItem(group.classes[k]->name.c_str());
				}

				ImGui::EndMenu();
			}
		}

		ImGui::EndPopup();
	}

	ImGui::Dummy(ImVec2(0, 10));

	ImGui::Text("Attributes:");
	ImGui::Separator();

	ImGui::BeginChild("SmartEditWindow");

	ImGui::Columns(2, "smartcolumns", false); // 4-ways, with border

	static char keyNames[128][64];
	static char keyValues[128][64];

	float paddingx = style.WindowPadding.x + style.FramePadding.x;
	float inputWidth = (ImGui::GetWindowWidth() - (paddingx * 2)) * 0.5f;

	// needed if autoresize is true
	if (ImGui::GetScrollMaxY() > 0)
		inputWidth -= style.ScrollbarSize * 0.5f;

	if (fgdClass != NULL) {
		for (int i = 0; i < fgdClass->keyvalues.size(); i++) {
			KeyvalueDef& keyvalue = fgdClass->keyvalues[i];
			string key = keyvalue.name;
			if (key == "spawnflags") {
				continue;
			}
			string value = ent->keyvalues[key];
			string niceName = keyvalue.description;

			strcpy(keyNames[i], niceName.c_str());
			strcpy(keyValues[i], value.c_str());

			ImGui::SetNextItemWidth(inputWidth);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(keyNames[i]); ImGui::NextColumn();

			ImGui::SetNextItemWidth(inputWidth);

			if (keyvalue.iType == FGD_KEY_CHOICES && keyvalue.choices.size() > 0) {
				string selectedValue = keyvalue.choices[0].name;
				int ikey = atoi(value.c_str());

				for (int k = 0; k < keyvalue.choices.size(); k++) {
					KeyvalueChoice& choice = keyvalue.choices[k];

					if ((choice.isInteger && ikey == choice.ivalue) ||
						(!choice.isInteger && value == choice.svalue)) {
						selectedValue = choice.name;
					}
				}

				if (ImGui::BeginCombo(("##val" + to_string(i)).c_str(), selectedValue.c_str()))
				{
					for (int k = 0; k < keyvalue.choices.size(); k++) {
						KeyvalueChoice& choice = keyvalue.choices[k];

						if (ImGui::Selectable(choice.name.c_str())) {
							// TODO: change value
						}
					}

					ImGui::EndCombo();
				}
			}
			else {
				struct TextFilters {
					static int filterInteger(ImGuiInputTextCallbackData* data) {
						if (data->EventChar < 256 && strchr("-0123456789", (char)data->EventChar))
							return 0;
						return 1;
					}
				};

				if (keyvalue.iType == FGD_KEY_INTEGER) {
					ImGui::InputText(("##val" + to_string(i)).c_str(), keyValues[i], 64, ImGuiInputTextFlags_CallbackCharFilter, TextFilters::filterInteger);
				}
				else {
					ImGui::InputText(("##val" + to_string(i)).c_str(), keyValues[i], 64);
				}


			}

			ImGui::NextColumn();
		}
	}

	ImGui::Columns(1);

	ImGui::EndChild();
}

void Renderer::drawKeyvalueEditor_FlagsTab(Entity* ent) {
	uint spawnflags = strtoul(ent->keyvalues["spawnflags"].c_str(), NULL, 10);
	FgdClass* fgdClass = fgd->getFgdClass(ent->keyvalues["classname"]);

	ImGui::Columns(2, "keyvalcols", true);

	for (int i = 0; i < 32; i++) {
		if (i == 16) {
			ImGui::NextColumn();
		}
		string name;
		if (fgdClass != NULL) {
			name = fgdClass->spawnFlagNames[i];
		}
		//ImGui::Text(to_string(1U << i).c_str()); ImGui::SameLine();
		ImGui::Checkbox((name + "##flag" + to_string(i)).c_str(), &showDebugWidget);
	}

	ImGui::Columns(1);
}

void Renderer::drawKeyvalueEditor_RawEditTab(Entity* ent) {
	ImGui::Columns(2, "keyvalcols", false);
	ImGui::Text("  Key"); ImGui::NextColumn();
	ImGui::Text("Value"); ImGui::NextColumn();

	ImGui::Columns(1);
	ImGui::BeginChild("RawValuesWindow");

	ImGui::Columns(2, "keyvalcols2", false);

	static char keyNames[128][64];
	static char keyValues[128][64];

	ImGuiStyle& style = ImGui::GetStyle();
	float paddingx = style.WindowPadding.x + style.FramePadding.x;
	float inputWidth = (ImGui::GetWindowWidth() - paddingx * 2) * 0.5f;

	struct kvInputState {
		int idx;
		Entity* entRef;
		int entIdx;
		BspRenderer* bspRenderer;
	};

	struct TextChangeCallback {
		static int keyNameChanged(ImGuiInputTextCallbackData* data) {
			kvInputState* keyId = (kvInputState*)data->UserData;
			Entity* ent = keyId->entRef;

			string key = ent->keyOrder[keyId->idx];
			if (key != data->Buf) {
				ent->renameKey(keyId->idx, data->Buf);
				keyId->bspRenderer->refreshEnt(keyId->entIdx);
			}
			
			return 1;
		}

		static int keyValueChanged(ImGuiInputTextCallbackData* data) {
			kvInputState* keyId = (kvInputState*)data->UserData;
			Entity* ent = keyId->entRef;
			string key = ent->keyOrder[keyId->idx];
			if (ent->keyvalues[key] != data->Buf) {
				ent->keyvalues[key] = data->Buf;
				keyId->bspRenderer->refreshEnt(keyId->entIdx);
			}
			
			return 1;
		}
	};

	static kvInputState keyIds[128];
	static kvInputState valueIds[128];
	static int lastPickCount = 0;

	for (int i = 0; i < ent->keyOrder.size() && i < 128; i++) {
		string key = ent->keyOrder[i];
		string value = ent->keyvalues[key];

		bool invalidKey = lastPickCount == pickCount && key != keyNames[i];

		strcpy(keyNames[i], key.c_str());
		strcpy(keyValues[i], value.c_str());

		keyIds[i].idx = i;
		keyIds[i].entIdx = pickInfo.entIdx;
		keyIds[i].entRef = ent;
		keyIds[i].bspRenderer = mapRenderers[pickInfo.mapIdx];
		valueIds[i].idx = i;
		valueIds[i].entIdx = pickInfo.entIdx;
		valueIds[i].entRef = ent;
		valueIds[i].bspRenderer = mapRenderers[pickInfo.mapIdx];

		
		if (invalidKey) {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(0, 0.6f, 0.6f));
		}

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText(("##key" + to_string(i) + "_" + to_string(pickCount)).c_str(), keyNames[i], 64, ImGuiInputTextFlags_CallbackAlways,
			TextChangeCallback::keyNameChanged, &keyIds[i]);
		ImGui::NextColumn();

		if (invalidKey) {
			ImGui::PopStyleColor();
		}

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText(("##val" + to_string(i) + to_string(pickCount)).c_str(), keyValues[i], 64, ImGuiInputTextFlags_CallbackAlways,
			TextChangeCallback::keyValueChanged, &valueIds[i]);
		ImGui::NextColumn();
	}

	lastPickCount = pickCount;

	ImGui::Columns(1);

	ImGui::EndChild();
}

void Renderer::cameraControls() {
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	if (!io.WantCaptureKeyboard)
		cameraOrigin += getMoveDir() * frameTimeScale;

	if (io.WantCaptureMouse)
		return;


	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	vec2 mousePos(xpos, ypos);

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (!cameraIsRotating) {
			lastMousePos = mousePos;
			cameraIsRotating = true;
		}
		else {
			vec2 drag = mousePos - lastMousePos;
			cameraAngles.z += drag.x * 0.5f;
			cameraAngles.x += drag.y * 0.5f;
			lastMousePos = mousePos;
		}

		ImGui::SetWindowFocus(NULL);
		ImGui::ClearActiveID();
	}
	else {
		cameraIsRotating = false;
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && oldLeftMouse != GLFW_PRESS) {
		getPickRay(pickStart, pickDir);

		pickCount++;
		memset(&pickInfo, 0, sizeof(PickInfo));
		pickInfo.bestDist = 9e99;
		for (int i = 0; i < mapRenderers.size(); i++) {
			if (mapRenderers[i]->pickPoly(pickStart, pickDir, pickInfo)) {
				pickInfo.mapIdx = i;
			}
		}

		pickEnd = pickStart + pickDir*pickInfo.bestDist;
	}

	oldLeftMouse = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
}

vec3 Renderer::getMoveDir()
{
	mat4x4 rotMat;
	rotMat.loadIdentity();
	rotMat.rotateX(PI * cameraAngles.x / 180.0f);
	rotMat.rotateZ(PI * cameraAngles.z / 180.0f);

	vec3 forward, right, up;
	makeVectors(cameraAngles, forward, right, up);


	vec3 wishdir(0, 0, 0);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		wishdir -= right;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		wishdir += right;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		wishdir += forward;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		wishdir -= forward;
	}

	wishdir *= moveSpeed;

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		wishdir *= 4.0f;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		wishdir *= 0.25f;
	return wishdir;
}

void Renderer::getPickRay(vec3& start, vec3& pickDir) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// invert ypos
	ypos = windowHeight - ypos;

	// translate mouse coordinates so that the origin lies in the center and is a scaler from +/-1.0
	float mouseX = ((xpos / (double)windowWidth) * 2.0f) - 1.0f;
	float mouseY = ((ypos / (double)windowHeight) * 2.0f) - 1.0f;

	// http://schabby.de/picking-opengl-ray-tracing/
	vec3 forward, right, up;
	makeVectors(cameraAngles, forward, right, up);

	vec3 view = forward.normalize(1.0f);
	vec3 h = crossProduct(view, up).normalize(1.0f); // 3D float vector
	vec3 v = crossProduct(h, view).normalize(1.0f); // 3D float vector

	// convert fovy to radians 
	float rad = fov * PI / 180.0f;
	float vLength = tan(rad / 2.0f) * zNear;
	float hLength = vLength * (windowWidth / (float)windowHeight);

	v *= vLength;
	h *= hLength;

	// linear combination to compute intersection of picking ray with view port plane
	start = cameraOrigin + view * zNear + h * mouseX + v * mouseY;

	// compute direction of picking ray by subtracting intersection point with camera position
	pickDir = (start - cameraOrigin).normalize(1.0f);
}

void Renderer::setupView() {
	fov = 75.0f;
	zNear = 1.0f;
	zFar = 262144.0f;

	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	glViewport(0, 0, windowWidth, windowHeight);

	projection.perspective(fov, (float)windowWidth / (float)windowHeight, zNear, zFar);

	view.loadIdentity();
	view.rotateX(PI * cameraAngles.x / 180.0f);
	view.rotateY(PI * cameraAngles.z / 180.0f);
	view.translate(-cameraOrigin.x, -cameraOrigin.z, cameraOrigin.y);
}

void Renderer::addMap(Bsp* map) {
	BspRenderer* mapRenderer = new BspRenderer(map, bspShader, colorShader, pointEntRenderer);

	mapRenderers.push_back(mapRenderer);
}

void Renderer::drawLine(vec3 start, vec3 end, COLOR3 color) {
	cVert verts[2];

	verts[0].x = start.x;
	verts[0].y = start.z;
	verts[0].z = -start.y;
	verts[0].c = color;

	verts[1].x = end.x;
	verts[1].y = end.z;
	verts[1].z = -end.y;
	verts[1].c = color;

	VertexBuffer buffer(colorShader, COLOR_3B | POS_3F, &verts[0], 2);
	buffer.draw(GL_LINES);
}