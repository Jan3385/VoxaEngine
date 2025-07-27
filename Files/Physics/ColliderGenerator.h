#pragma once

#include <vector>
#include <box2d/box2d.h>
#include "Math/Vector.h"
#include "World/Chunk.h"

struct Triangle;

static constexpr int GRID_PADDING_FILL = 1;

void FloodFillChunk(
    const bool values[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL], 
    int labels[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL],
    Vec2i start, unsigned short int currentLabel
);

void FloodFillObject(
    const std::vector<std::vector<bool>> &values,
    std::vector<std::vector<int>> &labels,
    const Vec2i &start, unsigned short int currentLabel
);

std::vector<b2Vec2> MarchingSquaresEdgeTrace(
    const int labels[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL],
    const int currentLabel
);

std::vector<b2Vec2> MarchingSquaresEdgeTrace(
    std::vector<std::vector<int>> &labels, 
    const int currentLabel
);

float PerpendicularDistance(
    const b2Vec2& point, 
    const b2Vec2& lineStart, 
    const b2Vec2& lineEnd
);

std::vector<b2Vec2> DouglasPeuckerSimplify(
    const std::vector<b2Vec2>& points, 
    float epsilon
);

std::vector<b2Vec2> RemoveCollinearPoints(
    const std::vector<b2Vec2>& points, 
    float epsilon
);

std::vector<b2Vec2> RemoveDuplicatePoints(
    const std::vector<b2Vec2>& points,
    float epsilon
);

// Helper function for triangulation of a polygon
float PolygonArea(const std::vector<b2Vec2>& polygon);
bool  IsConvex(const b2Vec2& prev, const b2Vec2& curr, const b2Vec2& next, bool ccw);
bool  IsPointInTriangle(const b2Vec2& point, const Triangle& triangle);

// unused as tradeoff for already made solution which is better and faster
std::vector<Triangle> TriangulatePolygon(
    const std::vector<b2Vec2>& polygon
);