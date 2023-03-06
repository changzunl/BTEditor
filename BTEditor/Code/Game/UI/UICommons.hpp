#pragma once

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"

#include "Game/Framework/GameCommon.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <functional>

#define UNUSED(x) (void)(x);

constexpr unsigned char KEYCODE_FOCUS = 0x07; // reserved on windows, use for focus detection

class BitmapFont;
class Shader;

extern VertexList g_vertsBuffer;
extern BitmapFont* g_uiFont;
extern Shader* g_uiFontShader;

// using String = std::string;
// 
// template<class T>
// using List = std::vector<T>;
// 
// extern List<String> g_textConstants;

enum class ELayoutSide
{
	LEFT,
	RIGHT,
	TOP,
	BOTTOM,
};

struct Rect
{
    Vec2                            m_pos;
    Vec2                            m_size;
    Vec2                            m_align = Vec2(0.5f, 0.5f);
};

