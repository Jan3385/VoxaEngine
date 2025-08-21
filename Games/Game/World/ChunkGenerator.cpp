#include "ChunkGenerator.h"
#include "Math/Random.h"
#include "World/ChunkMatrix.h"

using namespace Volume;

Chunk* ChunkGenerator::GenerateChunk(const Vec2i &chunkPos, ChunkMatrix &chunkMatrix){
    constexpr int seed = 123;

    Random gen(chunkPos.x * 1234 + chunkPos.y * 5678 + seed);
    Chunk* chunk = new Chunk(chunkPos);

    // some basic debug stuff
    if (chunkPos.y == 4 && chunkPos.x <= 3)
        if(chunkPos.x == 2)
            return FillChunkWith("Water", false, chunk);
        else
            return FillChunkWith("Sand", false, chunk);
    else if (chunkPos.y < 5)
        return FillChunkWith("Oxygen", false, chunk);

    ChunkBoolArray chunkData = FillChunkDataRandom(gen, chunkMatrix, chunkPos);
    
    for(int i = 0; i < 4; ++i)
        chunkData = SmoothOutChunkData(chunkData, chunkPos, chunkMatrix);

    for(int x = 0; x < Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Chunk::CHUNK_SIZE; ++y){
            if(chunkData[y][x]){
                chunk->voxels[y][x] = CreateVoxelElement(
                    "Dirt",
                    Vec2i(x + chunkPos.x * Chunk::CHUNK_SIZE, y + chunkPos.y * Chunk::CHUNK_SIZE),
                    20,
                    Temperature(21),
                    true
                );
            }else{
                chunk->voxels[y][x] = CreateVoxelElement(
                    "Oxygen",
                    Vec2i(x + chunkPos.x * Chunk::CHUNK_SIZE, y + chunkPos.y * Chunk::CHUNK_SIZE),
                    0,
                    Temperature(21),
                    true
                );
            }
        }
    }

    return chunk;
}

Volume::Chunk *ChunkGenerator::FillChunkWith(std::string materialID, bool unmovable, Volume::Chunk *chunk)
{
    Volume::VoxelProperty *prop = Registry::VoxelRegistry::GetProperties(materialID);
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            Volume::VoxelElement* voxel = CreateVoxelElement(
                prop,
                materialID,
                Vec2i(x + chunk->GetPos().x * Chunk::CHUNK_SIZE, y + chunk->GetPos().y * Chunk::CHUNK_SIZE),
                20,
                Temperature(21),
                unmovable
            );
            chunk->voxels[y][x] = voxel;
        }
    }
    return chunk;
}

ChunkGenerator::ChunkBoolArray ChunkGenerator::FillChunkDataRandom(Random &gen, ChunkMatrix &chunkMatrix, const Vec2i &chunkPos)
{
    ChunkBoolArray chunkData = {};
    for (int y = 0; y < Volume::Chunk::CHUNK_SIZE; ++y) {
        for (int x = 0; x < Volume::Chunk::CHUNK_SIZE; ++x) {
            chunkData[y][x] = gen.GetInt(0, 100) < GEN_FILL_PERCENTAGE;
        }
    }

    // copy the edges of nearby chunk onto this chunks edges
    Chunk *cUp = chunkMatrix.GetChunkAtChunkPosition(chunkPos + vector::UP);
    if(cUp){
        for(int i = 0; i < Chunk::CHUNK_SIZE; ++i){
            bool solidOnEdge = cUp->voxels[Chunk::CHUNK_SIZE-1][i]->IsUnmoveableSolid();
            chunkData[0][i] = solidOnEdge;
        }
    }
    Chunk *cDown = chunkMatrix.GetChunkAtChunkPosition(chunkPos + vector::DOWN);
    if(cDown){
        for(int i = 0; i < Chunk::CHUNK_SIZE; ++i){
            bool solidOnEdge = cDown->voxels[0][i]->IsUnmoveableSolid();
            chunkData[Chunk::CHUNK_SIZE-1][i] = solidOnEdge;
        }
    }
    Chunk *cLeft = chunkMatrix.GetChunkAtChunkPosition(chunkPos + vector::LEFT);
    if(cLeft){
        for(int i = 0; i < Chunk::CHUNK_SIZE; ++i){
            bool solidOnEdge = cLeft->voxels[i][Chunk::CHUNK_SIZE-1]->IsUnmoveableSolid();
            chunkData[i][0] = solidOnEdge;
        }
    }
    Chunk *cRight = chunkMatrix.GetChunkAtChunkPosition(chunkPos + vector::RIGHT);
    if(cRight){
        for(int i = 0; i < Chunk::CHUNK_SIZE; ++i){
            bool solidOnEdge = cRight->voxels[i][0]->IsUnmoveableSolid();
            chunkData[i][Chunk::CHUNK_SIZE-1] = solidOnEdge;
        }
    }

    return chunkData;
}

constexpr int SEARCH_RADIUS = 8;
constexpr int HALF_NEIGHBOR_COUNT = (SEARCH_RADIUS * 2) * (SEARCH_RADIUS * 2)/1.6f;
ChunkGenerator::ChunkBoolArray ChunkGenerator::SmoothOutChunkData(const ChunkBoolArray &chunkData, Vec2i chunkPos, ChunkMatrix &chunkMatrix)
{
    ChunkBoolArray smoothData = chunkData;
    for (int y = 0; y < Volume::Chunk::CHUNK_SIZE; ++y) {
        for (int x = 0; x < Volume::Chunk::CHUNK_SIZE; ++x) {
            int neighborCount = GetSurroundingSolidCount(chunkData, x, y, chunkPos, chunkMatrix);
            smoothData[y][x] = neighborCount > HALF_NEIGHBOR_COUNT ? 1 : neighborCount < HALF_NEIGHBOR_COUNT ? 0 : smoothData[y][x];
        }
    }
    return smoothData;
}

int ChunkGenerator::GetSurroundingSolidCount(const ChunkBoolArray &chunkData, int x, int y, Vec2i chunkPos, ChunkMatrix &chunkMatrix)
{
    int count = 0;
    for(int ix = -SEARCH_RADIUS; ix <= SEARCH_RADIUS; ++ix){
        for(int iy = -SEARCH_RADIUS; iy <= SEARCH_RADIUS; ++iy){
            if(ix == 0 && iy == 0) continue;

            int nx = x + ix;
            int ny = y + iy;

            //check for OOB
            if(nx >= 0 && nx < Volume::Chunk::CHUNK_SIZE && ny >= 0 && ny < Volume::Chunk::CHUNK_SIZE){
                if(chunkData[ny][nx]){
                    count++;
                }
            }else{
                if(rand() % 100 < 50){
                    count++;
                }
                //VoxelElement *voxel = chunkMatrix.VirtualGetAt_NoLoad(Vec2i(nx + chunkPos.x * Volume::Chunk::CHUNK_SIZE, ny + chunkPos.y * Volume::Chunk::CHUNK_SIZE), true);
                //if(!voxel || voxel->IsUnmoveableSolid()){
                //    count++;
                //}
            }
        }
    }

    return count;
}
