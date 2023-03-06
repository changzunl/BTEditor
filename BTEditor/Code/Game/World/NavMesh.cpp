#include "Game/World/NavMesh.hpp"

#include "Game/World/ChunkProvider.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/Block/BlockDef.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

const IntVec2 DIRECTIONS_EWNS[4] = { IntVec2(1, 0), IntVec2(-1, 0), IntVec2(0, 1), IntVec2(0, -1) };

NavMesh2D::NavMesh2D(World* world, const IntVec3& origin, const IntVec2& dimension)
    : m_world(world)
    , m_origin(origin)
    , m_halfDimension(dimension)
    , m_dimension(dimension * 2 + IntVec2(1, 1))
    , m_size((size_t)m_dimension.x * (size_t)m_dimension.y)
{
    m_navmap.resize(m_size);
    memset(&m_navmap[0], 0xFF, m_size);

    m_debugBuffer = new Vertex_PCU[m_size * 6];
    m_vbo = g_theRenderer->CreateVertexBuffer(m_size * 6 * sizeof(Vertex_PCU), &g_theRenderer->GetDefaultVF_PCU());
}

NavMesh2D::~NavMesh2D()
{
    delete[] m_debugBuffer;
    delete m_vbo;
}

void NavMesh2D::BuildNav()
{
    memset(&m_navmap[0], 0xFF, m_size);

    int _i = 0;
    int z = m_origin.z;
    ChunkCoords coords = Chunk::GetChunkCoords(IntVec3(m_origin.x, m_origin.y, 0));
    Chunk* chunk = m_world->FindChunk(coords);
    for (int j = m_origin.y - m_halfDimension.y; j <= m_origin.y + m_halfDimension.y; j++)
    {
        for (int i = m_origin.x - m_halfDimension.x; i <= m_origin.x + m_halfDimension.x; i++)
        {
            int cnt = _i++;

            ChunkCoords crds = Chunk::GetChunkCoords(IntVec3(i, j, 0));

            if (crds != coords)
            {
                coords = crds;
                chunk = m_world->FindChunk(coords);
            }

            const Block& block  = chunk ? chunk->GetBlock(Chunk::GetLocalCoords(IntVec3(i, j, z    ))) : Block::INVALID; 
            const Block& head   = chunk ? chunk->GetBlock(Chunk::GetLocalCoords(IntVec3(i, j, z + 1))) : Block::INVALID; 
            const Block& ground = chunk ? chunk->GetBlock(Chunk::GetLocalCoords(IntVec3(i, j, z - 1))) : Block::INVALID; 

            char& value = m_navmap[cnt];

            bool valid = block.IsValid();
            bool passable = valid && !block.IsSolid() && (!head.IsValid() || !head.IsSolid());
            bool solid = ground.IsValid() && ground.IsSolid();

            value = valid ? passable ? solid ? 0x00 : 0x02 : 0x01 : 0xFF;

            Rgba8 color = valid ? passable ? solid ? Rgba8::GREEN : Rgba8::YELLOW : Rgba8::RED : Rgba8(255, 0, 255);

            m_debugBuffer[cnt * 6 + 0] = Vertex_PCU{ Vec3(i    , j    , z + 4), color, Vec2() };
            m_debugBuffer[cnt * 6 + 1] = Vertex_PCU{ Vec3(i + 1, j    , z + 4), color, Vec2() };
            m_debugBuffer[cnt * 6 + 2] = Vertex_PCU{ Vec3(i    , j + 1, z + 4), color, Vec2() };
            m_debugBuffer[cnt * 6 + 3] = Vertex_PCU{ Vec3(i + 1, j + 1, z + 4), color, Vec2() };
            m_debugBuffer[cnt * 6 + 4] = Vertex_PCU{ Vec3(i    , j + 1, z + 4), color, Vec2() };
            m_debugBuffer[cnt * 6 + 5] = Vertex_PCU{ Vec3(i + 1, j    , z + 4), color, Vec2() };
        }
    }

    g_theRenderer->CopyCPUToGPU(&m_debugBuffer[0], m_size * 6 * sizeof(Vertex_PCU), m_vbo);
}

void NavMesh2D::Render() const
{
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->BindShader(nullptr);
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetDepthMask(true);
    g_theRenderer->SetDepthTest(DepthTest::LESSEQUAL);
    g_theRenderer->SetCullMode(CullMode::NONE);
    g_theRenderer->SetTintColor(Rgba8(255, 255, 255, 40));
    g_theRenderer->DrawVertexBuffer(m_vbo, m_size * 6);
    g_theRenderer->SetCullMode(CullMode::BACK);
    g_theRenderer->SetTintColor(Rgba8::WHITE);
}

NavMeshInst* NavMesh2D::CreatePathfind(const IntVec2& goal, bool flying)
{
    return new NavMeshInst(this, goal, flying);
}

bool NavMesh2D::QueryAccessible(const IntVec2& goal, bool flying)
{
    if (IsOutOfBounds(goal))
        return false;

    IntVec2 start = IntVec2(m_origin.x, m_origin.y) - m_halfDimension;

    IntVec2 offset = goal - start;

    char value = m_navmap[offset.x + offset.y * m_dimension.x];

    return (value == 0x00 || (flying && value == 0x02));
}

bool NavMesh2D::IsOutOfBounds(const IntVec2& coords) const
{
    return coords.x >= m_origin.x + m_halfDimension.x
        || coords.x <= m_origin.x - m_halfDimension.x
        || coords.y >= m_origin.y + m_halfDimension.y
        || coords.y <= m_origin.y - m_halfDimension.y;
}

NavMeshInst::NavMeshInst(NavMesh2D* mesh, const IntVec2& goal, bool flying)
    : m_navmesh(mesh)
    , m_flowmap(mesh->m_dimension)
{
    IntVec2 origin = IntVec2{ m_navmesh->m_origin.x - m_navmesh->m_halfDimension.x, m_navmesh->m_origin.y - m_navmesh->m_halfDimension.y };

    // double vector
    std::vector<IntVec2> modifiedCoords1;
    std::vector<IntVec2> modifiedCoords2;
    modifiedCoords1.reserve(128);
    modifiedCoords2.reserve(128);

    // Initialize
    m_flowmap.SetAllValues(0xFF);
    m_flowmap.SetValue(goal - origin, 0);
    modifiedCoords1.push_back(goal);

    IntVec2 start = IntVec2(m_navmesh->m_origin.x, m_navmesh->m_origin.y) - m_navmesh->m_halfDimension;

    // iterate step, limit to 127 blocks
    for (unsigned char heat = 1; heat < 127; heat += 1)
    {
        modifiedCoords1.swap(modifiedCoords2);
        for (const IntVec2& coord : modifiedCoords2)
        {
            for (const IntVec2& coordRelative : DIRECTIONS_EWNS)
            {
                IntVec2 coordNeighbor = coord + coordRelative;
                if (m_navmesh->IsOutOfBounds(coordNeighbor)) continue;

                IntVec2 offset = coordNeighbor - start;

                char value = m_navmesh->m_navmap[offset.x + offset.y * m_navmesh->m_dimension.x];

                // position is air && (flying || ground is solid)
                if (value == 0x00 || (flying && value == 0x02)) // reachable
                {
                    if (m_flowmap.GetValue(coordNeighbor - origin) > heat)
                    {
                        m_flowmap.SetValue(coordNeighbor - origin, heat);
                        modifiedCoords1.push_back(coordNeighbor);
                    }
                }
            }
        }
        modifiedCoords2.clear();

        if (modifiedCoords1.size() == 0)
            return;
    }
}

bool NavMeshInst::GetPath(const IntVec2& from, std::vector<IntVec2>& path)
{
    if (m_navmesh->IsOutOfBounds(from))
        return false;

    IntVec2 origin = IntVec2{ m_navmesh->m_origin.x - m_navmesh->m_halfDimension.x, m_navmesh->m_origin.y - m_navmesh->m_halfDimension.y };

    path.push_back(from);
    unsigned char heat = m_flowmap.GetValue(path.back() - origin);
    while (heat != 0)
    {
        IntVec2 step = path.back();
        for (const IntVec2& coordRelative : DIRECTIONS_EWNS)
        {
            IntVec2 coordNeighbor = path.back() + coordRelative;

            if (m_navmesh->IsOutOfBounds(coordNeighbor))
                continue;

            unsigned char newheat = m_flowmap.GetValue(coordNeighbor - origin);

            if (newheat < heat)
            {
                heat = newheat;
                step = coordNeighbor;
            }
        }

        if (step == path.back())
            return false;
        else
            path.push_back(step);
    }
    return true;
}

