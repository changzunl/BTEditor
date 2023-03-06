#pragma once

#include "Game/World/World.hpp"
#include "Game/World/Chunk.hpp"

#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Core/HeatMaps.hpp"

#include <vector>

struct Vertex_PCU;
class VertexBuffer;

struct NavMeshInst
{
public:
    NavMeshInst(NavMesh2D* mesh, const IntVec2& goal, bool flying);

    bool GetPath(const IntVec2& from, std::vector<IntVec2>& path);

public:
    NavMesh2D * const    m_navmesh;
    CompressedHeatMap    m_flowmap;
    bool                 m_goalReachable = false;
};

class NavMesh2D
{
    friend struct NavMeshInst;

public:
    NavMesh2D(World* world, const IntVec3& origin, const IntVec2& dimension);
    ~NavMesh2D();

    void BuildNav();
    void Render() const;

    NavMeshInst* CreatePathfind(const IntVec2& goal, bool flying);

    bool QueryAccessible(const IntVec2& goal, bool flying);

private:
    bool IsOutOfBounds(const IntVec2& coords) const;

private:
    World * const        m_world;
    Vertex_PCU*          m_debugBuffer;
    VertexBuffer*        m_vbo;
    IntVec3              m_origin;
    IntVec2              m_halfDimension;
    IntVec2              m_dimension;
    size_t               m_size;

    std::vector<char>    m_navmap;
};

