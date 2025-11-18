#include "Physics/Physics.h"

#include <iostream>
#include <algorithm>
#include <queue>

#include "Physics.h"
#include "GameEngine.h"
#include "Physics/ColliderGenerator.h"

GamePhysics::GamePhysics()
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2(0.0f, PHYS_OBJECT_GRAVITY);
    
    worldId = b2CreateWorld(&worldDef);

    assert(b2World_IsValid(worldId) && "Failed to create physics world");
}

GamePhysics::~GamePhysics()
{
    b2DestroyWorld(worldId);
}

/// @brief Generates 2D colliders for a chunk using flood fill and marching squares
/// @param chunk Chunk to generate colliders for
void GamePhysics::Generate2DCollidersForChunk(Volume::Chunk *chunk)
{
    std::string chunkPosStr = "(" + std::to_string(chunk->GetPos().x) + ", " + std::to_string(chunk->GetPos().y) + ") ";

    Debug::LogSpam(chunkPosStr + "Generating 2D colliders for chunk ");

    // STEP 1: flood fill
    int labels  [Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL] = {0};

    bool grid   [Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL] = {false};

    for(int y = 0; y < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL; ++y) {
        for(int x = 0; x < Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL; ++x) {

            if( y < Volume::Chunk::CHUNK_SIZE && x < Volume::Chunk::CHUNK_SIZE &&
                    chunk->voxels[y][x] && chunk->voxels[y][x]->IsSolidCollider()) {
                grid[y][x] = true;
            }
            
        }
    }

    // Flood fill
    int currentLabel = 1;
    for(int x = 0; x < Volume::Chunk::CHUNK_SIZE; ++x) {
        for(int y = 0; y < Volume::Chunk::CHUNK_SIZE; ++y) {
            if(grid[y][x] && labels[y][x] == 0) {
                FloodFillChunk(grid, labels, Vec2i(x, y), currentLabel);
                currentLabel++;
            }
        }
    }
    Debug::LogSpam(chunkPosStr + "Flood fill complete. Found " + std::to_string(currentLabel - 1) + " labels");

    std::vector<Triangle> allTriangleColliders;
    std::vector<b2Vec2> allEdges;
    for(int label = 1; label < currentLabel; ++label) {
        // STEP 2: marching squares to get edges
        std::vector<b2Vec2> edges = MarchingSquaresEdgeTrace(labels, label);
        Debug::LogSpam(chunkPosStr + std::to_string(label) + ": Marching Squares found " + std::to_string(edges.size()) + " edge points");

        if (edges.size() >= 3) {
            // STEP 3: Remove duplicate points
            edges = RemoveDuplicatePoints(edges, 1e-3f);
            Debug::LogSpam(chunkPosStr + std::to_string(label) + ": After removing duplicate points: " + std::to_string(edges.size()) + " points");

            // STEP 3.2: Remove collinear points
            edges = RemoveCollinearPoints(edges, 1e-3f);
            Debug::LogSpam(chunkPosStr + std::to_string(label) + ": After removing collinear points: " + std::to_string(edges.size()) + " points");

            // STEP 3.1: Douglas-Peucker simplification
            edges = DouglasPeuckerSimplify(edges, 0.5f);
            Debug::LogSpam(chunkPosStr + std::to_string(label) + ": After Douglas-Peucker simplification: " + std::to_string(edges.size()) + " points");

            if(edges.size() < 3) {
                continue; // Not enough points to form a polygon
            }

            // Ensure the polygon is oriented counter-clockwise
            if (PolygonArea(edges) < 0) {
                std::reverse(edges.begin(), edges.end());
            }

            // STEP 5: Triangulation
            std::vector<Triangle> triangles = TriangulatePolygon(edges);
            Debug::LogSpam(chunkPosStr + std::to_string(label) + ": Triangulated into " + std::to_string(triangles.size()) + " triangles");

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
    Debug::LogSpam("Generating 2D colliders for VoxelObject: " + object->GetName());

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
    Debug::LogSpam(object->GetName() + ": Most prominent label is " + std::to_string(maxLabel) + " with count " + std::to_string(maxCount));
    
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

    Debug::LogSpam(object->GetName() + ": Marching squares produced " + std::to_string(edges.size()) + " edge points.");

    if (edges.size() >= 3) {
        // STEP 3: Remove duplicate points
        edges = RemoveDuplicatePoints(edges, 1e-3f);
        Debug::LogSpam(object->GetName() + ": After removing duplicates, " + std::to_string(edges.size()) + " edge points remain.");

        // STEP 3.2: Remove collinear points
        edges = RemoveCollinearPoints(edges, 1e-3f);
        Debug::LogSpam(object->GetName() + ": After removing collinear points, " + std::to_string(edges.size()) + " edge points remain.");

        // STEP 3.1: Douglas-Peucker simplification
        edges = DouglasPeuckerSimplify(edges, 0.5f);
        Debug::LogSpam(object->GetName() + ": After Douglas-Peucker simplification, " + std::to_string(edges.size()) + " edge points remain.");

        if(edges.size() < 3) {
            return; // Not enough points to form a polygon
        }

        // Ensure the polygon is oriented counter-clockwise
        if (PolygonArea(edges) < 0) {
            std::reverse(edges.begin(), edges.end());
        }

        // STEP 5: Triangulation
        std::vector<Triangle> triangles = TriangulatePolygon(edges);
        Debug::LogSpam(object->GetName() + ": Triangulated into " + std::to_string(triangles.size()) + " triangles.");

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
    for(PhysicsObject* obj : chunkMatrix.physicsObjects) {
        if(obj->dirtyColliders) {
            this->Generate2DCollidersForVoxelObject(obj, &chunkMatrix);
        }
    }
    
    b2World_Step(worldId, deltaTime*SIMULATION_SPEED, this->SIMULATION_STEP_COUNT);

    // Update all physics object locations
    for(PhysicsObject* obj : chunkMatrix.physicsObjects) {
        if(b2Body_IsEnabled(obj->GetPhysicsBodyId())) {
            obj->UpdatePhysicPosition(worldId);
        }
    }
}
