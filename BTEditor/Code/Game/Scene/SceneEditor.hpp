#include "Game/Scene/Scene.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Game/Editor/UIGraph.hpp"

#include <vector>
#include <string>

class BTNode;
class BTNodeRoot;

class EditorUIElement
{
public:
	EditorUIElement(const char* name, const AABB2& box, Rgba8 color, bool selectable = false);

	bool IsInside(const Vec2& pos);

public:
	std::string m_name;
	AABB2 m_box;
	Rgba8 m_color;
	bool m_selectable;
};

class ScenePlaying;

class SceneEditor : public Scene
{
	friend class ScenePlaying;

public:
	SceneEditor(Game* game);
	virtual ~SceneEditor();
	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void Update() override;
	virtual void UpdateCamera() override;
	virtual void RenderWorld() const override;
	virtual void RenderUI() const override;

private:
	void HandleInput();
	void DebugAddInfo() const;

private:
	std::vector<EditorUIElement*> m_uiElements;
	bool m_debugElement = false;

	UIEditor*   m_editorUI = nullptr;
};

