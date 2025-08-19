#include "Physics/Physics.h"

#include <iostream>
#include <algorithm>
#include <queue>
#include <poly2tri/poly2tri.h>

#include "Physics.h"
#include "GameEngine.h"
#include "Physics/ColliderGenerator.h"

GamePhysics::GamePhysics()
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2(0.0f, PHYS_OBJECT_GRAVITY);
    
    worldId = b2CreateWorld(&worldDef);
}

GamePhysics::~GamePhysics()
{
    b2DestroyWorld(worldId);
}

/// @brief Generates 2D colliders for a chunk using flood fill and marching squares
/// @param chunk Chunk to generate colliders for
void GamePhysics::Generate2DCollidersForChunk(Volume::Chunk *chunk)
{
    // STEP 1: flood fill
    int labels      [Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL] = {0};

    bool grid       [Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL] = {false};

    for(int y = 0; y < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL; ++y) {
        for(int x = 0; x < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL; ++x) {

            if( y < Volume::Chunk::CHUNK_SIZE && x < Volume::Chunk::CHUNK_SIZE &&
                    chunk->voxels[y][x] && chunk->voxels[y][x]->IsSolidCollider()) {
                grid[y][x] = true;
            }
            
        }
    }

    int currentLabel = 1;
    for(int x = 0; x < Volume::Chunk::CHUNK_SIZE; ++x) {
        for(int y = 0; y < Volume::Chunk::CHUNK_SIZE; ++y) {
            if(grid[y][x] && labels[y][x] == 0) {
                FloodFillChunk(grid, labels, Vec2i(x, y), currentLabel);
                currentLabel++;
            }
        }
    }

    std::vector<Triangle> allTriangleColliders;
    std::vector<b2Vec2> allEdges;
    for(int label = 1; label < currentLabel; ++label) {
        // STEP 2: marching squares to get edges
        std::vector<b2Vec2> edges = MarchingSquaresEdgeTrace(labels, label);

        if (edges.size() >= 3) {
            // STEP 3: Remove duplicate points
            edges = RemoveDuplicatePoints(edges, 1e-3f);

            // STEP 3.2: Remove collinear points
            edges = RemoveCollinearPoints(edges, 1e-3f);

            // STEP 3.1: Douglas-Peucker simplification
            edges = DouglasPeuckerSimplify(edges, 0.5f);

            if(edges.size() < 3) {
                continue; // Not enough points to form a polygon
            }

            // Ensure the polygon is oriented counter-clockwise
            if (PolygonArea(edges) < 0) {
                std::reverse(edges.begin(), edges.end());
            }

            // STEP 5: Triangulation using poly2tri
            /*
            std::vector<p2t::Point*> p2tPoints;
            for (const auto& edge : edges) {
                p2tPoints.push_back(new p2t::Point(edge.x, edge.y));
            }
            p2t::CDT cdt(p2tPoints);
            cdt.Triangulate();

            std::vector<Triangle> triangles;
            for (const auto& triangle : cdt.GetTriangles()) {
                triangles.emplace_back(
                    b2Vec2(triangle->GetPoint(0)->x, triangle->GetPoint(0)->y),
                    b2Vec2(triangle->GetPoint(1)->x, triangle->GetPoint(1)->y),
                    b2Vec2(triangle->GetPoint(2)->x, triangle->GetPoint(2)->y)
                );
            }

            for (auto* p : p2tPoints) {
                delete p;
            }*/
            // I would rather use poly2tri but it causes crashes (maybe bad topology on my part)
            std::vector<Triangle> triangles = TriangulatePolygon(edges);

            // STEP 6: Store the triangles for physics
            allTriangleColliders.insert(allTriangleColliders.end(), triangles.begin(), triangles.end());
            allEdges.insert(allEdges.end(), edges.begin(), edges.end());
            allEdges.push_back(b2Vec2(-1,-1)); // (-1, -1) as a separating vector
        }
    }
    // STEP 7: Update the chunk's physics body with the generated triangles
    chunk->UpdateColliders(allTriangleColliders, allEdges, worldId);
}

void GamePhysics::Generate2DCollidersForVoxelObject(PhysicsObject *object, ChunkMatrix* chunkMatrix)
{
    // STEP 1: flood fill
    std::vector<std::vector<int>> labels(
        object->GetSize().y + GRID_PADDING_FILL,
        std::vector<int>(object->GetSize().x + GRID_PADDING_FILL, 0)
    );
    std::vector<std::vector<bool>> grid(
        object->GetSize().y + GRID_PADDING_FILL,
        std::vector<bool>(object->GetSize().x + GRID_PADDING_FILL, false)
    );
    for(int y = 0; y < static_cast<int>(labels.size()); ++y) {
        for(int x = 0; x < static_cast<int>(labels[0].size()); ++x) {
            if(y < object->GetSize().y && x < object->GetSize().x &&
               object->voxels[y][x] && 
               object->voxels[y][x]->IsSolidCollider()) {
                grid[y][x] = true;
            }
        }
    }
    // Fill the grid with the flood fill algorithm
    unsigned short int currentLabel = 1;
    for(int x = 0; x < object->GetSize().x; ++x) {
        for(int y = 0; y < object->GetSize().y; ++y) {
            if(grid[y][x] && labels[y][x] == 0) {
                FloodFillObject(grid, labels, Vec2i(x, y), currentLabel);
                currentLabel++;
            }
        }
    }

    // find the most prominent label
    int maxLabel = 1;
    int maxCount = 0;
    std::unordered_map<int, int> labelCounts;
    for (int y = 0; y < static_cast<int>(labels.size()); ++y) {
        for (int x = 0; x < static_cast<int>(labels[0].size()); ++x) {
            int label = labels[y][x];
            if (label > 0) {
                labelCounts[label]++;
                if (labelCounts[label] > maxCount) {
                    maxCount = labelCounts[label];
                    maxLabel = label;
                }
            }
        }
    }
    
    // remove all non-prominent voxel groups
    if(object->CanBreakIntoParts()){
        for(int y = 0; y < static_cast<int>(object->voxels.size()); ++y) {
            for(int x = 0; x < static_cast<int>(object->voxels[0].size()); ++x) {
                if(labels[y][x] != maxLabel) { // any other label than the most prominent one get removed
                    // kick the voxel out of the voxel
                    Volume::VoxelElement* voxel = object->voxels[y][x];
                    if(voxel) {
                        voxel->position = Vec2i(x, y);
                        voxel->position = object->GetRotatedLocalPosition(voxel->position);
                        voxel->position += Vec2i(object->GetPosition().x, object->GetPosition().y) -
                            Vec2i(object->GetSize().x / 2, object->GetSize().y / 2);

                        Volume::VoxelSolid* solidVoxel = dynamic_cast<Volume::VoxelSolid*>(voxel);
                        if(solidVoxel)
                            solidVoxel->isStatic = false;

                        chunkMatrix->PlaceVoxelAt(
                            voxel,
                            false,
                            false
                        );
                        object->voxels[y][x] = nullptr;
                    }
                }
            }
        }
    }

    std::vector<Triangle> allTriangleColliders;
    std::vector<b2Vec2> allEdges;

    // STEP 2: marching squares to get edges
    std::vector<b2Vec2> edges = MarchingSquaresEdgeTrace(labels, maxLabel);

    if (edges.size() >= 3) {
        // STEP 3: Remove duplicate points
        edges = RemoveDuplicatePoints(edges, 1e-3f);

        // STEP 3.2: Remove collinear points
        edges = RemoveCollinearPoints(edges, 1e-3f);

        // STEP 3.1: Douglas-Peucker simplification
        edges = DouglasPeuckerSimplify(edges, 0.5f);

        if(edges.size() < 3) {
            return; // Not enough points to form a polygon
        }

        // Ensure the polygon is oriented counter-clockwise
        if (PolygonArea(edges) < 0) {
            std::reverse(edges.begin(), edges.end());
        }

        // STEP 5: Triangulation using poly2tri
        /*
        std::vector<p2t::Point*> p2tPoints;
        for (const auto& edge : edges) {
            p2tPoints.push_back(new p2t::Point(edge.x, edge.y));
        }
        p2t::CDT cdt(p2tPoints);
        cdt.Triangulate();
        std::vector<Triangle> triangles;
        for (const auto& triangle : cdt.GetTriangles()) {
            triangles.emplace_back(
                b2Vec2(triangle->GetPoint(0)->x, triangle->GetPoint(0)->y),
                b2Vec2(triangle->GetPoint(1)->x, triangle->GetPoint(1)->y),
                b2Vec2(triangle->GetPoint(2)->x, triangle->GetPoint(2)->y)
            );
        }
        for (auto* p : p2tPoints) {
            delete p;
        }*/
        // I would rather use poly2tri but it causes crashes (maybe bad topology on my part)
        std::vector<Triangle> triangles = TriangulatePolygon(edges);

        // STEP 6: Store the triangles for physics
        allTriangleColliders.insert(allTriangleColliders.end(), triangles.begin(), triangles.end());
        allEdges.insert(allEdges.end(), edges.begin(), edges.end());
        allEdges.push_back(b2Vec2(-1,-1)); // (-1, -1) as a separating vector
    }
    // STEP 7: Update the object's physics body with the generated triangles
    object->UpdateColliders(allTriangleColliders, allEdges, worldId);
}

/// @brief Steps the physics simulation forward by a given time step
/// @param deltaTime        time step for the simulation
void GamePhysics::Step(float deltaTime, ChunkMatrix& chunkMatrix)
{
    std::cout << "A" << std::endl;
    for(PhysicsObject* obj : physicsObjects) {
        if(obj->dirtyColliders) {
            this->Generate2DCollidersForVoxelObject(obj, &chunkMatrix);
        }
    }
    std::cout << "B" << std::endl;
    
    b2World_Step(worldId, deltaTime*SIMULATION_SPEED, this->SIMULATION_STEP_COUNT);

    std::cout << "C" << std::endl;

    // Update all physics object locations
    for(PhysicsObject* obj : physicsObjects) {
        if(b2Body_IsEnabled(obj->GetPhysicsBodyId())) {
            obj->UpdatePhysicPosition(worldId);
        }
    }

    std::cout << "D" << std::endl;
}
