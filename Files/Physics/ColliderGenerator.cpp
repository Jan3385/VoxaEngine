#include "ColliderGenerator.h"

#include <queue>

#include "Physics/Physics.h"

/// @brief                  Basic flood fill algorithm
/// @param values           2D array. true if part of the fill area, false otherwise
/// @param labels           2D array to store labels for each cell (should be initialized to 0)
/// @param start            starting point for the flood fill
/// @param currentLabel     current label to assign to the filled area
/// @return                 true if the flood fill was successful, false otherwise
void FloodFillChunk(
    const bool values[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL], 
    int labels[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL], 
    Vec2i start, unsigned short int currentLabel)
{
    std::queue<Vec2i> queue;
    queue.push(start);
    labels[start.y][start.x] = currentLabel;

    while(!queue.empty()){
        Vec2i current = queue.front();
        queue.pop();

        for(const Vec2i& offset : vector::AROUND4){
            Vec2i neighbor = current + offset;

            // Check bounds
            if(neighbor.x < 0 || neighbor.x > Volume::Chunk::CHUNK_SIZE ||
               neighbor.y < 0 || neighbor.y > Volume::Chunk::CHUNK_SIZE) {
                continue;
            }

            // If the neighbor is part of the fill area and not labeled yet
            if(values[neighbor.y][neighbor.x] && labels[neighbor.y][neighbor.x] == 0) {
                labels[neighbor.y][neighbor.x] = currentLabel;
                queue.push(neighbor);
            }
        }
    }

    // fill in holes inside labels
    for(int y = 0; y < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL; ++y) {
        for(int x = 0; x < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL; ++x) {
            uint8_t surroundedSides = 0;
            uint8_t verticalSides = 0;
            uint8_t horizontalSides = 0;
            for(const Vec2i& offset : vector::AROUND4) {
                Vec2i neighbor = Vec2i(x, y) + offset;

                // Check bounds
                if(neighbor.x < 0 || neighbor.x > Volume::Chunk::CHUNK_SIZE ||
                   neighbor.y < 0 || neighbor.y > Volume::Chunk::CHUNK_SIZE) {
                    continue;
                }

                if(labels[neighbor.y][neighbor.x] == currentLabel) {
                    ++surroundedSides;

                    if(offset.x == 0) ++verticalSides;
                    else ++horizontalSides;
                }
            }
            if(labels[y][x] == 0 && (surroundedSides >= 3 || verticalSides == 2 || horizontalSides == 2)) {
                labels[y][x] = currentLabel;
            }
        }
    }
}

void FloodFillObject(const std::vector<std::vector<bool>> &values, std::vector<std::vector<int>> &labels, 
    const Vec2i &start, unsigned short int currentLabel)
{
    std::queue<Vec2i> queue;
    queue.push(start);
    labels[start.y][start.x] = currentLabel;

    while(!queue.empty()){
        Vec2i current = queue.front();
        queue.pop();

        for(const Vec2i& offset : vector::AROUND4){
            Vec2i neighbor = current + offset;

            // Check bounds
            if(neighbor.x < 0 || neighbor.x >= static_cast<int>(values[0].size()) ||
               neighbor.y < 0 || neighbor.y >= static_cast<int>(values.size())) {
                continue;
            }

            // If the neighbor is part of the fill area and not labeled yet
            if(values[neighbor.y][neighbor.x] && labels[neighbor.y][neighbor.x] == 0) {
                labels[neighbor.y][neighbor.x] = currentLabel;
                queue.push(neighbor);
            }
        }
    }
}

int m_DirectionToOffsetIndex(const Vec2i &dir)
{
    if(dir == vector::RIGHT)return 2;
    if(dir == vector::UP)   return 1;
    if(dir == vector::LEFT) return 0;
    if(dir == vector::DOWN) return 3;

    throw std::invalid_argument("Invalid direction for vector offset index: " + std::to_string(dir.x) + ", " + std::to_string(dir.y));
}

/// @brief                  Modified Marching Squares algorithm to find edges in a 2D grid
/// @param labels           2D array of labels from the flood fill
/// @param currentLabel     the label for which to find edges
/// @return                 vector of b2Vec2 representing the edges found
std::vector<b2Vec2> MarchingSquaresEdgeTrace(
    const int labels[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL],
    const int currentLabel)
{
    std::vector<b2Vec2> polygon;

    Vec2i start(-1, -1);
    for(int y = Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL-1; y >= 0 && start.x == -1; --y) {
        for(int x = 0; x < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL; ++x) {
            if(labels[y][x] == currentLabel) {
                start = Vec2i(x, y);
                break;
            }
        }
    }

    if(start.x == -1) return polygon;

    //  0,0 ---- 1,0
    //   |        |
    //   |        |
    //   |        |
    //  0,1 ---- 1,1
    
    Vec2i offsets[4] = {
        Vec2i(0, 0),
        Vec2i(1, 0),
        Vec2i(1, 1),
        Vec2i(0, 1),
    };

    Vec2i current = start;
    Vec2i dir = vector::UP;

    // initial polygon start
    int startOffsetIndex = m_DirectionToOffsetIndex(vector::DOWN);
    Vec2i startEdge = current + offsets[startOffsetIndex];
    polygon.push_back(b2Vec2(startEdge.x, startEdge.y));

    int pass = 0;
    do
    {
        Vec2i checkDir = dir;

        for(int i = 0; i < 4; ++i) {
            i == 0 ? checkDir.RotateLeft() : checkDir.RotateRight();

            Vec2i edgePos = current + checkDir;

            if(edgePos == startEdge)
                return polygon; // We have completed the polygon

            
            bool labelAtCheck;
            if(edgePos.x < 0 || edgePos.x >= Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL ||
               edgePos.y < 0 || edgePos.y >= Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL) {
                labelAtCheck = false; // Out of bounds, no label
            } else {
                labelAtCheck = (labels[edgePos.y][edgePos.x] == currentLabel);
            }

            if(!labelAtCheck) {
                // If we havent found a label in the check direction, we can place the polygon point
                Vec2i currentOffset = offsets[m_DirectionToOffsetIndex(checkDir)];
                polygon.push_back(b2Vec2(current.x + currentOffset.x, current.y + currentOffset.y));
                continue;
            }

            // If we found a label in the check direction, we can change the direction in that direction
            dir = checkDir;
            break;
        }

        current += checkDir;
        pass++;
    } while (pass < 5000);

    //TODO: just a hack to not crash, fix this properly someday
    if(pass >= 5000)
        return {};

    return polygon;
}

std::vector<b2Vec2> MarchingSquaresEdgeTrace(std::vector<std::vector<int>> &labels, const int currentLabel)
{
    std::vector<b2Vec2> polygon;

    Vec2i start(-1, -1);
    for(int y = labels.size() - 1; y >= 0 && start.x == -1; --y) {
        for(int x = 0; x < static_cast<int>(labels[0].size()); ++x) {
            if(labels[y][x] == currentLabel) {
                start = Vec2i(x, y);
                break;
            }
        }
    }

    if(start.x == -1) return polygon;

    //  0,0 ---- 1,0
    //   |        |
    //   |        |
    //   |        |
    //  0,1 ---- 1,1
    
    Vec2i offsets[4] = {
        Vec2i(0, 0),
        Vec2i(1, 0),
        Vec2i(1, 1),
        Vec2i(0, 1),
    };

    Vec2i current = start;
    Vec2i dir = vector::UP;

    // initial polygon start
    int startOffsetIndex = m_DirectionToOffsetIndex(vector::DOWN);
    Vec2i startEdge = current + offsets[startOffsetIndex];
    polygon.push_back(b2Vec2(startEdge.x, startEdge.y));

    int pass = 0;
    do
    {
        Vec2i checkDir = dir;

        for(int i = 0; i < 4; ++i) {
            i == 0 ? checkDir.RotateLeft() : checkDir.RotateRight();

            Vec2i edgePos = current + checkDir;

            if(edgePos == startEdge)
                return polygon; // We have completed the polygon

            
            bool labelAtCheck;
            if(edgePos.x < 0 || edgePos.x >= static_cast<int>(labels[0].size()) ||
               edgePos.y < 0 || edgePos.y >= static_cast<int>(labels.size())) {
                labelAtCheck = false; // Out of bounds, no label
            } else {
                labelAtCheck = (labels[edgePos.y][edgePos.x] == currentLabel);
            }

            if(!labelAtCheck) {
                // If we havent found a label in the check direction, we can place the polygon point
                Vec2i currentOffset = offsets[m_DirectionToOffsetIndex(checkDir)];
                polygon.push_back(b2Vec2(current.x + currentOffset.x, current.y + currentOffset.y));
                continue;
            }

            // If we found a label in the check direction, we can change the direction in that direction
            dir = checkDir;
            break;
        }

        current += checkDir;
        pass++;
    } while (pass < 5000);

    //TODO: again a hack to not crash
    if(pass >= 5000)
        return {};
    
    return polygon;
}

/// @brief                  Calculate the perpendicular distance from a point to a line segment
/// @param point            the point to measure distance from
/// @param lineStart        the start point of the line segment
/// @param lineEnd          the end point of the line segment
/// @return                 the perpendicular distance from the point to the line segment
/// If the line segment is a point, it returns the distance from the point to that point
float PerpendicularDistance(const b2Vec2 &point, const b2Vec2 &lineStart, const b2Vec2 &lineEnd)
{
    b2Vec2 dVec = lineEnd - lineStart;

    if(dVec.x == 0.0f && dVec.y == 0.0f) {
        dVec = point - lineStart;
        return std::sqrt(dVec.x * dVec.x + dVec.y * dVec.y);
    }

    float t = ((point.x - lineStart.x) * dVec.x + (point.y - lineStart.y) * dVec.y) / (dVec.x * dVec.x + dVec.y * dVec.y);
    if(t < 0.0f) {
        dVec = point - lineStart;
    } else if(t > 1.0f) {
        dVec = point - lineEnd;
    } else {
        b2Vec2 xVec = lineStart + t * dVec;
        dVec = point - xVec;
    }

    return std::sqrt(dVec.x * dVec.x + dVec.y * dVec.y);
}

std::vector<b2Vec2> DouglasPeuckerSimplify(const std::vector<b2Vec2> &points, float epsilon)
{
    if(points.size() < 3) {
        return points;      // no need to simplify
    }

    float maxDistance = 0.0f;
    size_t index = 0;

    for (size_t i = 0; i < points.size(); i++)
    {
        float distance = PerpendicularDistance(points[i], points.front(), points.back());
        if (distance > maxDistance) {
            maxDistance = distance;
            index = i;
        }
    }

    if(maxDistance > epsilon) {
        std::vector<b2Vec2> leftPoints(points.begin(), points.begin() + index + 1);
        std::vector<b2Vec2> rightPoints(points.begin() + index, points.end());

        std::vector<b2Vec2> leftSimplified = DouglasPeuckerSimplify(leftPoints, epsilon);
        std::vector<b2Vec2> rightSimplified = DouglasPeuckerSimplify(rightPoints, epsilon);

        leftSimplified.pop_back(); // Remove the last point to avoid duplication
        leftSimplified.insert(leftSimplified.end(), rightSimplified.begin(), rightSimplified.end());
        return leftSimplified;
    } else {
        return {points.front(), points.back()};
    }
    
}

std::vector<b2Vec2> RemoveCollinearPoints(const std::vector<b2Vec2> &points, float epsilon)
{
    if(points.size() < 3) {
        return points;
    }

    std::vector<b2Vec2> result;
    result.reserve(points.size());

    result.push_back(points.front());

    for(size_t i = 1; i < points.size() - 1; ++i) {
        const b2Vec2 &prev = points[i - 1];
        const b2Vec2 &curr = points[i];
        const b2Vec2 &next = points[i + 1];

        float area = (curr.x - prev.x) * (next.y - prev.y) - (curr.y - prev.y) * (next.x - prev.x);
        if(std::abs(area) > epsilon) {
            result.push_back(curr);
        }
    }

    result.push_back(points.back());
    
    return result;
}

std::vector<b2Vec2> RemoveDuplicatePoints(const std::vector<b2Vec2> &points, float epsilon)
{
    if(points.size() < 2) {
        return points;
    }

    std::vector<b2Vec2> result;
    result.reserve(points.size());
    result.push_back(points[0]);

    for (size_t i = 1; i < points.size(); ++i) {
        const b2Vec2& prev = result.back();
        const b2Vec2& curr = points[i];
        if (std::abs(prev.x - curr.x) > epsilon || std::abs(prev.y - curr.y) > epsilon) {
            result.push_back(curr);
        }
    }

    return result;
}

// Polygon triangulation helper functions -------------------

/// @brief Calculates the area of a polygon defined by a vector of b2Vec2 points
float PolygonArea(const std::vector<b2Vec2> &polygon)
{
    float area = 0.0f;
    size_t n = polygon.size();
    
    for (size_t i = 0; i < n; ++i) {
        const b2Vec2 &p1 = polygon[i];
        const b2Vec2 &p2 = polygon[(i + 1) % n];
        area += p1.x * p2.y - p2.x * p1.y;
    }
    
    return area * 0.5f;
}

/// @brief Checks if three points form a convex corner (using cross product)
/// @param prev  previous point
/// @param curr  current point
/// @param next  next point
/// @param ccw   true if checking for counter-clockwise convexity, false for clockwise
/// @return      true if the points form a convex corner, false otherwise
bool IsConvex(const b2Vec2 &prev, const b2Vec2 &curr, const b2Vec2 &next, bool ccw)
{
    b2Vec2 v1 = curr - prev;
    b2Vec2 v2 = next - curr;

    float crossProduct = v1.x * v2.y - v1.y * v2.x;

    constexpr float EPSILON = 1e-8f;
    return ccw ? (crossProduct > EPSILON) : (crossProduct < -EPSILON);
}

/// @brief Checks if a point is inside a triangle defined by three b2Vec2 points
bool IsPointInTriangle(const b2Vec2 &point, const Triangle &triangle)
{
    float area = std::abs((triangle.b.x - triangle.a.x) * (triangle.c.y - triangle.a.y) - 
                          (triangle.c.x - triangle.a.x) * (triangle.b.y - triangle.a.y));
    float area1 = std::abs((triangle.a.x - point.x) * (triangle.b.y - point.y) - 
                           (triangle.b.x - point.x) * (triangle.a.y - point.y));
    float area2 = std::abs((triangle.b.x - point.x) * (triangle.c.y - point.y) -
                           (triangle.c.x - point.x) * (triangle.b.y - point.y));
    float area3 = std::abs((triangle.c.x - point.x) * (triangle.a.y - point.y) -
                           (triangle.a.x - point.x) * (triangle.c.y - point.y));

    return std::abs(area - (area1 + area2 + area3)) < 1e-5f;
}

// ----------------------------------------------------------

/// @brief Triangulates a polygon defined by a vector of b2Vec2 points (unused)
/// @param polygon Vector of b2Vec2 points representing the polygon vertices
/// @return Vector of Triangle objects representing the triangulated polygons
std::vector<Triangle> TriangulatePolygon(const std::vector<b2Vec2> &polygon)
{
    std::vector<Triangle> triangles;
    if(polygon.size() < 3) return triangles;

    bool ccw = PolygonArea(polygon) > 0;
    size_t n = polygon.size();

    std::vector<int> indices(n);
    for(size_t i = 0; i < n; ++i) indices[i] = i;

    while(indices.size() > 3){
        bool earFound = false;
        for(size_t i = 0; i < indices.size(); ++i) {
            int prevIdx = indices[(i+indices.size() - 1) % indices.size()];
            int currIdx = indices[i];
            int nextIdx = indices[(i+1) % indices.size()];

            const b2Vec2 &prev = polygon[prevIdx];
            const b2Vec2 &curr = polygon[currIdx];
            const b2Vec2 &next = polygon[nextIdx];

            if(!IsConvex(prev, curr, next, ccw))
                continue; // Not a convex corner
            
            bool hasPointInside = false;
            Triangle triangle(prev, curr, next);
            for(size_t j = 0; j < indices.size(); ++j) {
                int testIdx = indices[j];
                if(testIdx == prevIdx || testIdx == currIdx || testIdx == nextIdx)
                    continue; // Skip the triangle vertices

                if(IsPointInTriangle(polygon[indices[j]], triangle)) {
                    hasPointInside = true;
                    break; // Found a point inside the triangle
                }
            }

            if(hasPointInside) continue;

            // Found an ear
            triangles.emplace_back(triangle);
            indices.erase(indices.begin() + i);
            earFound = true;
            break;
        }

        if(!earFound) {
            break;
        } 
    }

    // Add the last triangle
    if(indices.size() == 3) {
        int idx1 = indices[0];
        int idx2 = indices[1];
        int idx3 = indices[2];
        triangles.emplace_back(Triangle(polygon[idx1], polygon[idx2], polygon[idx3]));
    }

    return triangles;
}