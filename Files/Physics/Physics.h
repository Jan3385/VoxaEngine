#pragma once

#include <box2d/box2d.h>

#include <Math/Vector.h>
#include <World/Chunk.h>

struct Triangle{
    b2Vec2 a;
    b2Vec2 b;
    b2Vec2 c;

    Triangle(const b2Vec2& a, const b2Vec2& b, const b2Vec2& c)
        : a(a), b(b), c(c) {};
};

class GamePhysics{
private:
    static constexpr int GRID_PADDING_FILL = 1;
    static constexpr int SIMULATION_STEP_COUNT = 4;
    b2WorldId worldId;

    static void FloodFillChunk(
        const bool values[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL], 
        int labels[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL],
        Vec2i start, unsigned short int currentLabel
    );

    static std::vector<b2Vec2> MarchingSquaresEdgeTrace(
        const int labels[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL],
        const int currentLabel
    );

    static float PerpendicularDistance(
        const b2Vec2& point, 
        const b2Vec2& lineStart, 
        const b2Vec2& lineEnd
    );

    static std::vector<b2Vec2> DouglasPeuckerSimplify(
        const std::vector<b2Vec2>& points, 
        float epsilon
    );

    static std::vector<b2Vec2> RemoveCollinearPoints(
        const std::vector<b2Vec2>& points, 
        float epsilon
    );

    static std::vector<b2Vec2> RemoveDuplicatePoints(
        const std::vector<b2Vec2>& points,
        float epsilon
    );

    // Helper function for triangulation of a polygon
    static float PolygonArea(const std::vector<b2Vec2>& polygon);
    static bool  IsConvex(const b2Vec2& prev, const b2Vec2& curr, const b2Vec2& next, bool ccw);
    static bool  IsPointInTriangle(const b2Vec2& point, const Triangle& triangle);

    // unused as tradeoff for already made solution which is better and faster
    static std::vector<Triangle> TriangulatePolygon(
        const std::vector<b2Vec2>& polygon
    );

public:
    GamePhysics();
    ~GamePhysics();
    void Step(float deltaTime);

    void Generate2DCollidersForChunk(
        Volume::Chunk* chunk, glm::mat4 voxelProj
    );
};