#include "Physics/Physics.h"

#include <iostream>
#include <queue>
#include "Physics.h"
#include <GameEngine.h>

GamePhysics::GamePhysics()
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2(0.0f, -9.81f);
    
    worldId = b2CreateWorld(&worldDef);
}

GamePhysics::~GamePhysics()
{
    b2DestroyWorld(worldId);
}

/// @brief                  Basic flood fill algorithm
/// @param values           2D array. true if part of the fill area, false otherwise
/// @param labels           2D array to store labels for each cell (should be initialized to 0)
/// @param start            starting point for the flood fill
/// @param currentLabel     current label to assign to the filled area
/// @return                 true if the flood fill was successful, false otherwise
void GamePhysics::FloodFillChunk(
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
}

/// @brief                  Modified Marching Squares algorithm to find edges in a 2D grid
/// @param labels           2D array of labels from the flood fill
/// @param currentLabel     the label for which to find edges
/// @return                 vector of b2Vec2 representing the edges found
std::vector<b2Vec2> GamePhysics::MarchingSquaresEdgeTrace(
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

    // Start trace
    Vec2i current = start;

    Vec2i direction(-1, 0); // Start going right

    Vec2i startPos = current;
    Vec2i startDir = direction;

    do {
        polygon.push_back(b2Vec2(current.x, current.y));

        Vec2i leftDir = direction;
        leftDir.RotateLeft();
        Vec2i leftPos = current + leftDir;

        bool leftFilled = 
            (leftPos.x >= 0 && leftPos.x < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL && 
             leftPos.y >= 0 && leftPos.y < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL) 
            && labels[leftPos.y][leftPos.x] == currentLabel;
        
        if(leftFilled){
            direction = leftDir;
            current += direction;
        } else {
            Vec2i forwardPos = current + direction;
            bool forwardFilled = 
                (forwardPos.x >= 0 && forwardPos.x < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL && 
                 forwardPos.y >= 0 && forwardPos.y < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL) 
                && labels[forwardPos.y][forwardPos.x] == currentLabel;
            
            if(forwardFilled) {
                current = forwardPos;
            } else {
                direction.RotateRight();
            }
        }

    } while(!(current == startPos && direction == startDir));

    return polygon;
}

/// @brief                  Calculate the perpendicular distance from a point to a line segment
/// @param point            the point to measure distance from
/// @param lineStart        the start point of the line segment
/// @param lineEnd          the end point of the line segment
/// @return                 the perpendicular distance from the point to the line segment
/// If the line segment is a point, it returns the distance from the point to that point
float GamePhysics::PerpendicularDistance(const b2Vec2 &point, const b2Vec2 &lineStart, const b2Vec2 &lineEnd)
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

std::vector<b2Vec2> GamePhysics::DouglasPeuckerSimplify(const std::vector<b2Vec2> &points, float epsilon)
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

void GamePhysics::Generate2DCollidersForChunk(Volume::Chunk *chunk, glm::mat4 voxelProj)
{

    // STEP 1: flood fill
    int labels      [Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL] = {0};
    int currentLabel = 1;

    bool grid       [Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL] = {false};
    bool paddedGrid [Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL] = {false};

    for(int y = 0; y < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL; ++y) {
        for(int x = 0; x < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL; ++x) {

            if( y < Volume::Chunk::CHUNK_SIZE && x < Volume::Chunk::CHUNK_SIZE &&
                chunk->voxels[x][y] && chunk->voxels[x][y]->GetState() == Volume::State::Solid) {
                grid[y][x] = true;
            } // Pad to the right and bottom due to how the flood fill works
            else if(grid[y][x-1] && !paddedGrid[y][x-1]) {
                grid[y][x] = true;
                paddedGrid[y][x] = true;
            }
            else if(grid[y-1][x] && !paddedGrid[y-1][x]) {
                grid[y][x] = true;
                paddedGrid[y][x] = true;
                
                if(paddedGrid[y-1][x+1]){
                    paddedGrid[y][x+1] = true;
                    grid[y][x+1] = true;
                }
            }
            
        }
    }

    for(int x = 0; x < Volume::Chunk::CHUNK_SIZE; ++x) {
        for(int y = 0; y < Volume::Chunk::CHUNK_SIZE; ++y) {
            if(grid[y][x] && labels[y][x] == 0) {
                FloodFillChunk(grid, labels, Vec2i(x, y), currentLabel);
                currentLabel++;
            }
        }
    }

    // STEP 2: marching squares to get edges
    for(int label = 1; label < currentLabel; ++label) {
        std::vector<b2Vec2> edges = MarchingSquaresEdgeTrace(labels, label);

        if (!edges.empty()) {
            // STEP 3: Douglas-Peucker simplification
            edges = DouglasPeuckerSimplify(edges, 0.8f);

            // Convert b2Vec2 to your renderer's vector type if needed
            std::vector<glm::vec2> renderPoints;
            for (const auto& v : edges) {
                renderPoints.emplace_back(v.x + chunk->GetPos().x*chunk->CHUNK_SIZE, v.y + chunk->GetPos().y*chunk->CHUNK_SIZE);
            }
            
            GameEngine::instance->renderer->DrawClosedShape(
                renderPoints, 
                glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // Color red for edges
                voxelProj, 
                1.0f // Line width
            );
        }
        
        // STEP 3: create polygon shape from edges
        //if(!edges.empty()) {
        //    b2PolygonShape polygonShape;
        //    polygonShape.Set(edges.data(), edges.size());

        //    // Create a body for the polygon
        //    b2BodyDef bodyDef;
        //    bodyDef.type = b2_staticBody;
        //    b2Body* body = b2CreateBody(worldId, &bodyDef);

        //    // Attach the polygon shape to the body
        //    b2FixtureDef fixtureDef;
        //    fixtureDef.shape = &polygonShape;
        //    fixtureDef.density = 1.0f;
        //    fixtureDef.friction = 0.5f;
        //    body->CreateFixture(&fixtureDef);
        //}
    }
}

void GamePhysics::Step(float deltaTime)
{
    b2World_Step(worldId, deltaTime, this->SIMULATION_STEP_COUNT);
}
