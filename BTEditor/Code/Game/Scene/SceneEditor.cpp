#include "Game/Scene/SceneEditor.hpp"

#include "Game/Framework/Game.hpp"
#include "Game/Scene/SceneAttract.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

#include "Game/Editor/BTNode.hpp"
#include "Game/Editor/UIGraph.hpp"

#include "Game/UI/UICanvas.hpp"
#include "Game/UI/UIComponents.hpp"

#include "Game/Framework/App.hpp"


extern RandomNumberGenerator rng;

SceneEditor::SceneEditor(Game* game)
	: Scene(game)
{
}

SceneEditor::~SceneEditor()
{
}

void SceneEditor::Initialize()
{
    g_theApp->StartEditorWindow();
    g_editorWindow->SetMouseMode(false, false, false);
	g_editorWindow->SetFocus();

	DebugRenderClear();

	m_editorUI = new UIEditor();
	m_editorUI->SetViewport(g_theRenderer->GetViewport());
	m_editorUI->Active();

	m_editorUI->ForEachChild_BFS([](UIWidget* widget)
		{
			DebuggerPrintf("Widget: %s\n", widget->GetName().c_str());
		}
	);


	g_theGame->StopBGM();
}

void SceneEditor::Shutdown()
{
	delete m_editorUI;
	m_editorUI = nullptr;

// 	for (auto node : m_nodes)
// 		delete node;
// 	m_rootNode = nullptr;

	for (auto element : m_uiElements)
		delete element;
	m_uiElements.clear();

	g_theApp->ShutdownEditorWindow();
}

void SceneEditor::Update()
{
/*	m_rootNode->Execute();*/
	m_editorUI->OnUpdate();

	HandleInput();

	DebugAddInfo();
}

void SceneEditor::UpdateCamera()
{
	m_worldCamera[0].SetPerspectiveView(float(g_editorWindow->GetClientDimensions().x) / float(g_editorWindow->GetClientDimensions().y), 60.0f, 0.1f, 100.0f);
	m_worldCamera[0].SetRenderTransform(Vec3(0, -1, 0), Vec3(0, 0, 1), Vec3(1, 0, 0));
	m_worldCamera[0].SetViewTransform(Vec3::ZERO, EulerAngles(0, 0, GetLifeTime() * 50));

	m_screenCamera[0].SetOrthoView(g_theRenderer->GetViewport().m_mins, g_theRenderer->GetViewport().m_maxs);

	Scene::UpdateCamera();
}

void SceneEditor::RenderWorld() const
{

}

void SceneEditor::RenderUI() const
{
	Texture* rt = g_theRenderer->GetScreenTexture(g_editorWindow);
	g_theRenderer->SetRenderTargets(1, &rt);

	AABB2 viewport  = g_theRenderer->GetViewport();

    m_editorUI->Render();

	static VertexList verts;
	verts.clear();

	Vec2 position = g_theInput->GetMousePositionInWindow(viewport);

	for (auto element : m_uiElements)
	{
		Rgba8 color = element->m_color;

		if (element->m_selectable && element->IsInside(position))
		{
			color.r = 255 - color.r;
			color.g = 255 - color.g;
			color.b = 255 - color.b;
		}

		AddVertsForAABB2D(verts, element->m_box, color);
	}

// 	m_rootNode->ForAllChildNode([this](BTNode* node) 
// 	{
// 		Vec2 position = node->GetPosition();
// 		AABB2 box = AABB2(position, position);
// 		box.SetDimensions(Vec2(0.08f, 0.08f));
// 		box = m_graph->m_viewBox.GetSubBox(box);
// 
// 		Rgba8 color = Rgba8(40, 40, 40);
// 
// 		if (node->IsExecuting())
// 		{
// 			color.r = 255;
// 			color.g = 255;
// 		}
// 		else if (!node->IsSuccess())
// 		{
// 			color.r = 255;
// 		}
// 		else if (node->IsAborted())
// 		{
// 			color.r = 127;
// 			color.g = 127;
// 			color.b = 127;
// 		}
// 
// 		AddVertsForAABB2D(verts, box, color);
// 	});

	g_theRenderer->DrawVertexArray(verts);
	verts.clear();

// 	int i = 0;
// 	m_rootNode->ForAllChildNode([this, &i](BTNode* node)
// 		{
// 			Vec2 position = node->GetPosition();
// 			AABB2 box = AABB2(position, position);
// 			box.SetDimensions(Vec2(0.075f, 0.08f));
// 			box = m_graph->m_viewBox.GetSubBox(box);
// 
// 			std::string name = std::string(typeid(*node).name()).substr(12);
// 			Rgba8 color(160, 160, 160);
// 
// 			std::string index = Stringf("%d", ++i);
// 			g_testFont->AddVertsForTextInBox2D(verts, box, 20.0f, name, color, 0.666f, Vec2(0.5f, 0.5f), TextDrawMode::SHRINK_TO_FIT);
// 			g_testFont->AddVertsForTextInBox2D(verts, box, 15.0f, index, color, 0.666f, Vec2(0.01f, 0.9f), TextDrawMode::SHRINK_TO_FIT);
// 		}
// 	);

	g_theRenderer->BindTexture(&g_testFont->GetTexture());
	g_theRenderer->DrawVertexArray(verts);
	g_theRenderer->BindTexture(nullptr);

	// DebugRenderWorld(GetScreenCamera());
	// DebugRenderScreen(GetScreenCamera());

    g_theRenderer->SetRenderTargets(0, nullptr);
}

void SceneEditor::HandleInput()
{
    if (m_editorUI->IsInputEnabled() && g_editorWindow->PullQuitRequested())
    {
        g_theGame->LoadScene(nullptr);
        g_theGame->m_clickSound->PlaySound();
    }
}

void SceneEditor::DebugAddInfo() const
{
    const char* msgRaw = "Windows: Game active=%s, Editor active=%s";
    std::string msg = Stringf(msgRaw, g_theWindow->IsWindowActive() ? "true" : "false", g_editorWindow->IsWindowActive() ? "true" : "false");
    DebugAddMessage(msg, .0f, Rgba8::WHITE, Rgba8::WHITE);
}

EditorUIElement::EditorUIElement(const char* name, const AABB2& box, Rgba8 color, bool selectable /*= false*/)
	: m_name(name)
	, m_box(box)
	, m_color(color)
	, m_selectable(selectable)
{

}

bool EditorUIElement::IsInside(const Vec2& pos)
{
	return m_box.IsPointInside(pos);
}

