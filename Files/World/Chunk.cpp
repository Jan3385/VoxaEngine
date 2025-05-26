#include "Chunk.h"
#include "Voxel.h"
#include "VoxelTypes.h"	
#include <math.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>
#include "GameEngine.h"
#include "World/Particles/SolidFallingParticle.h"
#include "World/ParticleGenerators/LaserParticleGenerator.h"


using namespace Volume; 

GLuint Volume::Chunk::computeShaderHeat_Program = 0;
const char* Chunk::computeShaderHeat = R"glsl(#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

#define TEMPERATURE_TRANSITION_SPEED 80

#define CHUNK_SIZE 64
#define CHUNK_SIZE_SQUARED 4096

#define DIRECTION_COUNT 4

const ivec2 directions[DIRECTION_COUNT] = {
    ivec2(0, -1),
    ivec2(-1, 0),
    ivec2(1, 0),
    ivec2(0, 1),
};

struct VoxelHeatData{
    float temperature;
    float capacity;
    float conductivity;
};
struct ChunkConnectivityData{
    int chunk;
    int chunkUp;
    int chunkDown;
    int chunkLeft;
    int chunkRight;
};

layout(std430, binding = 0) buffer InputBuffer {
    uint NumberOfVoxels;
    // flattened array (c = chunk, x = x, y = y)
    VoxelHeatData voxelTemps[];
};
layout(std430, binding = 1) buffer ChunkBuffer {
    ChunkConnectivityData chunkData[];
};
layout(std430, binding = 2) buffer OutputVoxelBuffer {
    float voxelTempsOut[];
};
layout(std430, binding = 3) buffer OutputChunkBuffer {
    uint maxHeatDiffChunk[];
};

void main(){
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint c = gl_GlobalInvocationID.z;

    uint localX = x % CHUNK_SIZE;
    uint localY = y % CHUNK_SIZE;

    uint index = c * CHUNK_SIZE_SQUARED + y * CHUNK_SIZE + x;

    uint numberOfChunks = NumberOfVoxels / CHUNK_SIZE_SQUARED;

    float sum = 0.0;

    ivec2 pos = ivec2(x, y);
    ivec2 localPos = ivec2(localX, localY);

    uint NumOfValidDirections = 0;
    for(int i = 0; i < DIRECTION_COUNT; ++i){
        ivec2 testPos = localPos + directions[i];

        ivec2 nPos = pos + directions[i];
        uint nIndex;

        // forbid diagonal heat transfer
        if((testPos.x < 0 || testPos.x >= CHUNK_SIZE) && (testPos.y < 0 || testPos.y >= CHUNK_SIZE))
            continue;

        // if out of bounds from current chunk
        if(testPos.x < 0 || testPos.x >= CHUNK_SIZE || testPos.y < 0 || testPos.y >= CHUNK_SIZE) {
            if(testPos.x < 0){ // right
                uint nC;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkRight == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + CHUNK_SIZE + (nPos.x - CHUNK_SIZE);
                ++NumOfValidDirections;
            }else if (testPos.x >= CHUNK_SIZE){ // left
                uint nC;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkLeft == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + (CHUNK_SIZE - nPos.x);
                ++NumOfValidDirections;
            }
            if(testPos.y >= CHUNK_SIZE){ // up
                uint nC;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkUp == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + (CHUNK_SIZE - nPos.y) * CHUNK_SIZE + nPos.x;
                ++NumOfValidDirections;
            }else if(testPos.y < 0){ // down
                uint nC;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkDown == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + (CHUNK_SIZE + nPos.y) * CHUNK_SIZE + nPos.x;
                ++NumOfValidDirections;
            }
        }else{ // if in bounds
            nIndex = c * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + nPos.x;
            ++NumOfValidDirections;
        }
        
        float heatCapacity = voxelTemps[index].capacity/TEMPERATURE_TRANSITION_SPEED;
        float heatDiff = voxelTemps[nIndex].temperature - voxelTemps[index].temperature;
        float heatTrans = heatDiff * voxelTemps[nIndex].conductivity / heatCapacity;

        if(heatCapacity <= 0.0f)
            heatTrans = 0.0f;

        sum += heatTrans;

        // Update heat value for the chunk
        atomicMax(maxHeatDiffChunk[c], uint(heatDiff*1000));
    }

    if(NumOfValidDirections == 0) NumOfValidDirections = 1;
    
    voxelTempsOut[index] = voxelTemps[index].temperature + (sum / NumOfValidDirections);
}
)glsl";

Volume::Chunk::Chunk(const Vec2i &pos) : m_x(pos.getX()), m_y(pos.getY())
{
    this->font = TTF_OpenFont("Fonts/RobotoFont.ttf", 12);
    
    if(!this->font)
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    //std::cout << "Chunk created at: " << m_x << "," << m_y << std::endl;
}

Volume::Chunk::~Chunk()
{

    if (this->font != nullptr) {
        TTF_CloseFont(this->font);
        this->font = nullptr;
    }
    SDL_FreeSurface(this->chunkSurface);

    for (uint8_t i = 0; i < static_cast<uint8_t>(voxels.size()); i++)
    {
        for (uint8_t j = 0; j < static_cast<uint8_t>(voxels[i].size()); j++)
        {
            delete voxels[i][j];
        }
    }
}
bool Volume::Chunk::ShouldChunkDelete(AABB &Camera) const
{
    if(lastCheckedCountDown > 0) return false;
    if(!this->dirtyRect.IsEmpty()) return false;

    return true;
}
bool Volume::Chunk::ShouldChunkCalculateHeat() const
{
    return forceHeatUpdate;
}
bool Volume::Chunk::ShouldChunkCalculatePressure() const
{
    return true;
    //TODO: fix when the vibe is right
    //return forcePressureUpdate;
}
void Volume::Chunk::UpdateVoxels(ChunkMatrix *matrix)
{
    for (int x = dirtyRect.end.getX(); x >= dirtyRect.start.getX(); --x)
    {
        //for (int y = dirtyRect.end.getY(); y >= dirtyRect.start.getY(); --y) -> better gasses, worse solids
        for (int y = dirtyRect.start.getY(); y <= dirtyRect.end.getY(); ++y) //better solids, worse gasses
        {
            if(x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE) continue;
    		if (voxels[x][y]->Step(matrix)) {
                //add to dirty rect
                dirtyRect.Include(Vec2i(x, y));

                //if voxel is at the edge of chunk, update neighbour chunk
    			if (x == 0) { // left
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x - 1, m_y));
                    if(c)c->dirtyRect.Include(Vec2i(CHUNK_SIZE - 1, y));
    			}
    			else if (x == CHUNK_SIZE - 1) { // right
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x + 1, m_y));
                    if(c)c->dirtyRect.Include(Vec2i(0, y));
    			}
    			if (y == 0) { // top
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y - 1));
                    if(c)c->dirtyRect.Include(Vec2i(x, CHUNK_SIZE - 1));
    			}
    			else if (y == CHUNK_SIZE - 1) { // bottom
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y + 1));
                    if(c)c->dirtyRect.Include(Vec2i(x, 0));
    			}
    		}

        }
    }
}

//transfer heat within the chunk
void Volume::Chunk::GetHeatMap(ChunkMatrix *matrix,
    Volume::VoxelHeatData HeatDataArray[],  // flattened array
    int chunkNumber)
{
    const uint16_t ChunkSizeSquared = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE;
    #pragma omp parallel for collapse(2)
    for(int x = 0; x < Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Chunk::CHUNK_SIZE; ++y){
            
            int index = chunkNumber * ChunkSizeSquared + y * Chunk::CHUNK_SIZE + x;
            HeatDataArray[index].temperature = this->voxels[x][y]->temperature.GetCelsius();
            HeatDataArray[index].capacity = this->voxels[x][y]->properties->HeatCapacity;
            HeatDataArray[index].conductivity = this->voxels[x][y]->properties->HeatConductivity;
        }
    }
}

void Volume::Chunk::GetPressureMap(ChunkMatrix *matrix,
    Volume::VoxelPressureData PressureDataArray[], // flattened array
    int chunkNumber)
{
    const uint16_t ChunkSizeSquared = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE;
    #pragma omp parallel for collapse(2)
    for(int x = 0; x < Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Chunk::CHUNK_SIZE; ++y){
            
            int index = chunkNumber * ChunkSizeSquared + y * Chunk::CHUNK_SIZE + x;
            PressureDataArray[index].pressure = this->voxels[x][y]->amount;
            PressureDataArray[index].id = this->voxels[x][y]->properties->id | 
                (this->voxels[x][y]->GetState() != State::Gas)  << 31 | 
                (this->voxels[x][y]->GetState() == State::Liquid) << 30;
        }
    }
}

// resets voxel update data, DO NOT CALL WITHOUT LOCKING THE VOXELMUTEX, main use for simulation thread
void Volume::Chunk::SIM_ResetVoxelUpdateData()
{
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < CHUNK_SIZE; ++y)
    {
        for (int x = 0; x < CHUNK_SIZE; ++x)
        {
            voxels[x][y]->updatedThisFrame = false;
        }
    }
}

SDL_Surface* Volume::Chunk::Render(bool debugRender)
{
    uint8_t x1 = 0;
    uint8_t y1 = 0;
    uint16_t x2 = CHUNK_SIZE * RENDER_VOXEL_SIZE;
    uint16_t y2 = CHUNK_SIZE * RENDER_VOXEL_SIZE;

    if(this->chunkSurface == nullptr) {
        this->chunkSurface = SDL_CreateRGBSurfaceWithFormat(0, CHUNK_SIZE * RENDER_VOXEL_SIZE, CHUNK_SIZE * RENDER_VOXEL_SIZE, 32, SDL_PIXELFORMAT_RGBA8888);
    }
    else if(this->dirtyRender || debugRender){
        this->dirtyRender = false;
        //SDL_FreeSurface(this->chunkSurface);

        //render individual voxels
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                RGBA& color = voxels[x][y]->color;
                SDL_Rect rect = {
                    x*RENDER_VOXEL_SIZE,
                    y*RENDER_VOXEL_SIZE,
                    RENDER_VOXEL_SIZE,
                    RENDER_VOXEL_SIZE
                };
                SDL_FillRect(this->chunkSurface, &rect, SDL_MapRGBA(this->chunkSurface->format, color.r, color.g, color.b, color.a));
            }
        }
        if(debugRender){
            // Render chunk position as text
            SDL_Color textColor = {255, 255, 255, 255};
            std::string text = std::to_string(m_x) + "," + std::to_string(m_y);

            // Render text to a surface
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), textColor);

            // Copy the text surface to the target surface
            SDL_Rect textRect = {x1, y1, textSurface->w, textSurface->h};
            SDL_BlitSurface(textSurface, nullptr, this->chunkSurface, &textRect);

            // Free the text surface
            SDL_FreeSurface(textSurface);
        }
        
    }

    if(!debugRender) return this->chunkSurface;

    // Determine border color based on update state
    Uint32 borderColor = SDL_MapRGBA(this->chunkSurface->format, 255, 0, 0, 255);

    // Draw border (manual pixel manipulation for lines)
    
    SDL_Rect topLine = {x1, y1, x2 - x1, 1};           // Top
    SDL_Rect bottomLine = {x1, y2 - 1, x2 - x1, 1};    // Bottom
    SDL_Rect leftLine = {x1, y1, 1, y2 - y1};          // Left
    SDL_Rect rightLine = {x2 - 1, y1, 1, y2 - y1};     // Right

    SDL_FillRect(this->chunkSurface, &topLine, borderColor);
    SDL_FillRect(this->chunkSurface, &bottomLine, borderColor);
    SDL_FillRect(this->chunkSurface, &leftLine, borderColor);
    SDL_FillRect(this->chunkSurface, &rightLine, borderColor);

    //draws a blue box at the corner of the chunk if the heat updated on the chunk
    if(ShouldChunkCalculateHeat()){
        SDL_Rect box = {x1 + CHUNK_SIZE * RENDER_VOXEL_SIZE - 15, y1 + CHUNK_SIZE * RENDER_VOXEL_SIZE - 15, 10, 10};
        SDL_FillRect(this->chunkSurface, &box, SDL_MapRGBA(this->chunkSurface->format, 0, 0, 255, 255));
    }

    //draws a red box at the corner of the chunk if the pressure updated on the chunk
    if(ShouldChunkCalculatePressure()){
        SDL_Rect box = {x1 + CHUNK_SIZE * RENDER_VOXEL_SIZE - 15, y1 + CHUNK_SIZE * RENDER_VOXEL_SIZE - 25, 10, 10};
        SDL_FillRect(this->chunkSurface, &box, SDL_MapRGBA(this->chunkSurface->format, 255, 0, 0, 255));
    }

    //draws the dirty rect borders
    if(!this->dirtyRect.IsEmpty()){
        SDL_Rect dirtyRect = {
            this->dirtyRect.start.getX() * RENDER_VOXEL_SIZE,
            this->dirtyRect.start.getY() * RENDER_VOXEL_SIZE,
            (this->dirtyRect.end.getX() - this->dirtyRect.start.getX() + 1) * RENDER_VOXEL_SIZE,
            (this->dirtyRect.end.getY() - this->dirtyRect.start.getY() + 1) * RENDER_VOXEL_SIZE
        };

        // Draw the dirty rect borders
        SDL_Rect topLine = {dirtyRect.x, dirtyRect.y, dirtyRect.w, 1};           // Top
        SDL_Rect bottomLine = {dirtyRect.x, dirtyRect.y + dirtyRect.h - 1, dirtyRect.w, 1};    // Bottom
        SDL_Rect leftLine = {dirtyRect.x, dirtyRect.y, 1, dirtyRect.h};          // Left
        SDL_Rect rightLine = {dirtyRect.x + dirtyRect.w - 1, dirtyRect.y, 1, dirtyRect.h};     // Right

        SDL_FillRect(this->chunkSurface, &topLine, SDL_MapRGBA(this->chunkSurface->format, 0, 255, 0, 255));
        SDL_FillRect(this->chunkSurface, &bottomLine, SDL_MapRGBA(this->chunkSurface->format, 0, 255, 0, 255));
        SDL_FillRect(this->chunkSurface, &leftLine, SDL_MapRGBA(this->chunkSurface->format, 0, 255, 0, 255));
        SDL_FillRect(this->chunkSurface, &rightLine, SDL_MapRGBA(this->chunkSurface->format, 0, 255, 0, 255));
    }

    return this->chunkSurface;
}
Vec2i Volume::Chunk::GetPos() const
{
    return Vec2i(m_x, m_y);
}
AABB Volume::Chunk::GetAABB() const
{
    return AABB(
        Vec2f(m_x * CHUNK_SIZE, m_y * CHUNK_SIZE), 
        Vec2f(CHUNK_SIZE, CHUNK_SIZE));
}
void ChunkMatrix::RenderParticles(SDL_Renderer &renderer, Vec2f offset) const
{
    SDL_SetRenderDrawBlendMode(&renderer, SDL_BLENDMODE_BLEND);
    for (auto& particle : particles) {
        const RGBA& color = particle->color;

        SDL_SetRenderDrawColor(&renderer, color.r, color.g, color.b, color.a);

        Vec2f particlePos = particle->GetPosition();
        SDL_Rect rect = {
            static_cast<int>((particlePos.getX()) * Chunk::RENDER_VOXEL_SIZE + offset.getX()),
            static_cast<int>((particlePos.getY()) * Chunk::RENDER_VOXEL_SIZE + offset.getY()),
            Chunk::RENDER_VOXEL_SIZE,
            Chunk::RENDER_VOXEL_SIZE
        };
        SDL_RenderFillRect(&renderer, &rect);
    }
}
ChunkMatrix::ChunkMatrix()
{
    this->particleGenerators.reserve(15);
    this->particles.reserve(200);

    Particle::LaserParticleGenerator* laserGenerator = new Particle::LaserParticleGenerator(this);
    this->particleGenerators.push_back(laserGenerator);
    laserGenerator->length = 20;
    laserGenerator->angle = M_PI * 2;
    laserGenerator->position = Vec2f(5, 5);
}

ChunkMatrix::~ChunkMatrix()
{
    this->cleanup();
}

void ChunkMatrix::cleanup()
{
    if(cleaned) return;
    cleaned = true;

    for(uint8_t i = 0; i < 4; ++i)
    {
        GridSegmented[i].clear();
    }
    for(int16_t i = Grid.size() - 1; i >= 0; --i)
    {
        delete Grid[i];
    }
    Grid.clear();
    
    for (auto& particle : particles) {
    	delete particle;
    }
    particles.clear();
    
    for (auto& particle : newParticles) {
        delete particle;
    }
    newParticles.clear();
}

Vec2i ChunkMatrix::WorldToChunkPosition(const Vec2f &pos)
{
    return Vec2i(
	    static_cast<int>(pos.getX() / Chunk::CHUNK_SIZE),
	    static_cast<int>(pos.getY() / Chunk::CHUNK_SIZE)
    );
}

Vec2f ChunkMatrix::ChunkToWorldPosition(const Vec2i &pos)
{
    return Vec2f(
	    static_cast<float>(pos.getX() * Chunk::CHUNK_SIZE),
        static_cast<float>(pos.getY() * Chunk::CHUNK_SIZE)
    );
}

Vec2f ChunkMatrix::MousePosToWorldPos(const Vec2f &mousePos, Vec2f offset)
{
    return Vec2f(
        (mousePos.getX() + offset.getX()) / Chunk::RENDER_VOXEL_SIZE,
        (mousePos.getY() + offset.getY())/ Chunk::RENDER_VOXEL_SIZE
    );
}

Volume::Chunk *ChunkMatrix::GetChunkAtWorldPosition(const Vec2f &pos)
{
    Vec2i chunkPos = WorldToChunkPosition(pos);
	if (!IsValidChunkPosition(chunkPos)) return nullptr;

    uint8_t AssignedGridPass = 0;
    if (chunkPos.getX() % 2 != 0) AssignedGridPass += 1;
    if (chunkPos.getY() % 2 != 0) AssignedGridPass += 2;

    for (size_t i = 0; i < this->GridSegmented[AssignedGridPass].size(); i++)
    {
        if(this->GridSegmented[AssignedGridPass][i]->GetPos() == chunkPos)
        {
            return this->GridSegmented[AssignedGridPass][i];
        }
    }
    

	return nullptr;
}

Volume::Chunk *ChunkMatrix::GetChunkAtChunkPosition(const Vec2i &pos)
{
    if (!IsValidChunkPosition(pos)) return nullptr;

    uint8_t AssignedGridPass = 0;
    if (pos.getX() % 2 != 0) AssignedGridPass += 1;
    if (pos.getY() % 2 != 0) AssignedGridPass += 2;


    for (size_t i = 0; i < this->GridSegmented[AssignedGridPass].size(); i++)
    {
        if(this->GridSegmented[AssignedGridPass][i]->GetPos() == pos)
        {
            return this->GridSegmented[AssignedGridPass][i];
        }
    }

    return nullptr;
}

void ChunkMatrix::PlaceVoxelsAtMousePosition(const Vec2f &pos, std::string id, Vec2f offset, Temperature temp)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);

    if(!IsValidWorldPosition(MouseWorldPos)) return;

    const int size = GameEngine::placementRadius;

    for (int x = -size; x <= size; x++)
    {
        for (int y = -size; y <= size; y++)
        {
            PlaceVoxelAt(MouseWorldPosI + Vec2i(x, y), id, temp, GameEngine::placeUnmovableSolidVoxels, GameEngine::placeVoxelAmount, false);
        }
    }
}

void ChunkMatrix::RemoveVoxelAtMousePosition(const Vec2f &pos, Vec2f offset)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);

    if(!IsValidWorldPosition(MouseWorldPos)) return;

    PlaceVoxelAt(MouseWorldPosI, "Oxygen", Temperature(21), false, 1, true);
}

void ChunkMatrix::ExplodeAtMousePosition(const Vec2f &pos, short int radius, Vec2f offset)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);

    if(!IsValidWorldPosition(MouseWorldPos)) return;

    this->ExplodeAt(MouseWorldPosI, radius);
}

// Generates a chunk and inputs it into the grid, returns a pointer to it
Volume::Chunk* ChunkMatrix::GenerateChunk(const Vec2i &pos)
{
    Chunk *chunk = new Chunk(pos);
    // Fill the voxels array with Voxel objects
    #pragma omp parallel for collapse(2)
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
    		//Create voxel determining on its position
            if (pos.getY() > 4) {
                if(pos.getY() == 5 && y <= 5){
                    chunk->voxels[x][y] = new VoxelSolid
                        ("Grass", 
                        Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE,y + pos.getY() * Chunk::CHUNK_SIZE), 
                        Temperature(21), 
                        true, 20);
                }else{
                    chunk->voxels[x][y] = new VoxelSolid
                        ("Dirt", 
                        Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE,y + pos.getY() * Chunk::CHUNK_SIZE), 
                        Temperature(21), 
                        true, 20);
                }
            }
            else {
                chunk->voxels[x][y] = new VoxelGas
                    ("Oxygen", 
                    Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE, y + pos.getY() * Chunk::CHUNK_SIZE),
                    Temperature(21), 1);
            }
    		if (pos.getY() == 4 && pos.getX() <= 3) {
    			chunk->voxels[x][y] = new VoxelSolid
                    ("Sand", 
                    Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE, y + pos.getY() * Chunk::CHUNK_SIZE),
                    Temperature(21),
                    false, 20);
    		}
        }
    }

    uint8_t AssignedGridPass = 0;
    if (pos.getX() % 2 != 0) AssignedGridPass += 1;
    if (pos.getY() % 2 != 0) AssignedGridPass += 2;

    this->GridSegmented[AssignedGridPass].push_back(chunk);

    this->Grid.push_back(chunk);

    return chunk;
}

void ChunkMatrix::DeleteChunk(const Vec2i &pos)
{
    uint8_t AssignedGridPass = 0;
    if (pos.getX() % 2 != 0) AssignedGridPass += 1;
    if (pos.getY() % 2 != 0) AssignedGridPass += 2;

    for (int32_t i = this->GridSegmented[AssignedGridPass].size()-1; i >= 0; --i)
    {
        if(this->GridSegmented[AssignedGridPass][i]->GetPos() == pos)
        {
            this->GridSegmented[AssignedGridPass].erase(this->GridSegmented[AssignedGridPass].begin() + i);
            break;
        }
    }

    for (int32_t i = this->Grid.size()-1; i >= 0; --i)
    {
        if(this->Grid[i]->GetPos() == pos)
        {
            Chunk *c = this->Grid[i];
            this->Grid.erase(this->Grid.begin() + i);
            delete c;
            return;
        }
    }
}

void ChunkMatrix::UpdateGridHeat(bool oddHeatUpdatePass)
{
    std::lock_guard<std::mutex> lock(this->voxelMutex);

    std::vector<Volume::VoxelHeatData> VoxelHeatArray(0);

    // doesnt create critical section, because the passes dont overlap
    uint16_t chunkIndex = 0;
    std::vector<Volume::Chunk*> chunksToUpdate;
    for(uint16_t i = 0; i < static_cast<uint16_t>(this->Grid.size()); ++i){
        if (this->Grid[i]->ShouldChunkCalculateHeat()){
            chunksToUpdate.push_back(this->Grid[i]);

            VoxelHeatArray.resize(VoxelHeatArray.size() + Volume::Chunk::CHUNK_SIZE * Volume::Chunk::CHUNK_SIZE);

            //load the heat maps from chunk
            this->Grid[i]->GetHeatMap(
                this, VoxelHeatArray.data(), chunkIndex++);
            
        }
    }

    uint16_t NumberOfChunks = chunksToUpdate.size();

    uint32_t NumberOfVoxels = Volume::Chunk::CHUNK_SIZE * Volume::Chunk::CHUNK_SIZE * NumberOfChunks;


    GLuint inputSSBO, chunkDataSSBO, outputVoxelSSBO, outputChunkSSBO;

    glGenBuffers(1, &inputSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
        sizeof(uint32_t) + sizeof(Volume::VoxelHeatData) * NumberOfVoxels,
        nullptr, GL_DYNAMIC_COPY);

    //buffer mapping
    void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 
        sizeof(uint32_t) + sizeof(Volume::VoxelHeatData) * NumberOfVoxels,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    
    uint32_t* VoxelsSizePtr = static_cast<uint32_t*>(ptr);
    *VoxelsSizePtr = NumberOfVoxels;

    Volume::VoxelHeatData* dataPtr = reinterpret_cast<Volume::VoxelHeatData*>(VoxelsSizePtr + 1);
    std::memcpy(dataPtr, VoxelHeatArray.data(), NumberOfVoxels * sizeof(Volume::VoxelHeatData));

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputSSBO); // Binding = 0

    //chunk info input
    std::vector<ChunkConnectivityData> chunkData;
    for(uint32_t i = 0; i < static_cast<uint32_t>(this->Grid.size()); ++i){
        if (this->Grid[i]->ShouldChunkCalculateHeat()){
            ChunkConnectivityData d;
            d.chunk = i;
            d.chunkUp = -1;
            d.chunkDown = -1;
            d.chunkLeft = -1;
            d.chunkRight = -1;

            Vec2i pos = this->Grid[i]->GetPos();
            Vec2i posUp = pos + vector::UP;
            Vec2i posDown = pos + vector::DOWN;
            Vec2i posLeft = pos + vector::LEFT;
            Vec2i posRight = pos + vector::RIGHT;
            for(uint32_t j = 0; j < static_cast<uint32_t>(this->Grid.size()); ++j){
                if(this->Grid[j]->GetPos() == posUp)
                    d.chunkUp = j;
                else if(this->Grid[j]->GetPos() == posDown)
                    d.chunkDown = j;
                else if(this->Grid[j]->GetPos() == posLeft)
                    d.chunkLeft = j;
                else if(this->Grid[j]->GetPos() == posRight)
                    d.chunkRight = j;
            }
            chunkData.push_back(d);
        }
    }

    glGenBuffers(1, &chunkDataSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkDataSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, chunkData.size() * sizeof(ChunkConnectivityData), chunkData.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, chunkDataSSBO); // Binding = 1

    // Output Buffer
    glGenBuffers(1, &outputVoxelSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputVoxelSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NumberOfVoxels * sizeof(float), nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, outputVoxelSSBO); // Binding = 2

    glGenBuffers(1, &outputChunkSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputChunkSSBO);
    uint32_t* zeroData = new uint32_t[NumberOfChunks]();
    std::fill(zeroData, zeroData + NumberOfChunks, 0);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NumberOfChunks * sizeof(uint32_t), zeroData, GL_DYNAMIC_COPY);
    delete[] zeroData;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, outputChunkSSBO); // Binding = 3

    // Compute Shader
    glUseProgram(Volume::Chunk::computeShaderHeat_Program);
    glDispatchCompute(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, NumberOfChunks);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputVoxelSSBO);
    float* VoxelHeatData = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NumberOfVoxels * sizeof(float), GL_MAP_READ_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputChunkSSBO);
    uint32_t* heatDiff = (uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NumberOfChunks * sizeof(uint32_t), GL_MAP_READ_BIT);


    uint16_t ChunkSizeSquared = static_cast<uint16_t>(Volume::Chunk::CHUNK_SIZE * Volume::Chunk::CHUNK_SIZE);

    #pragma omp parallel for
    for(uint32_t i = 0; i < NumberOfVoxels; ++i){
        uint16_t chunkIndex = i / ChunkSizeSquared;
        uint16_t voxelIndex = i % ChunkSizeSquared;
        uint16_t x = voxelIndex % Volume::Chunk::CHUNK_SIZE;
        uint16_t y = voxelIndex / Volume::Chunk::CHUNK_SIZE;

        auto& chunk = chunksToUpdate[chunkIndex];
        chunk->voxels[x][y]->temperature.SetCelsius(VoxelHeatData[i]);
        chunk->voxels[x][y]->CheckTransitionTemps(*this);
    }

    for(uint16_t i = 0; i < NumberOfChunks; ++i){
        if((heatDiff[i]/1000.0f) > 0.2f){
            chunksToUpdate[i]->forceHeatUpdate = true;
            //also force heat update on neighbours
            Vec2i pos = chunksToUpdate[i]->GetPos();
            Vec2i positions[4] = {
                pos + vector::UP,
                pos + vector::DOWN,
                pos + vector::LEFT,
                pos + vector::RIGHT
            };

            for(auto& p : positions){
                Volume::Chunk *c = GetChunkAtChunkPosition(p);
                if(c) c->forceHeatUpdate = true;
            }
        }else{
            chunksToUpdate[i]->forceHeatUpdate = false;
        }
    }
    
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glDeleteBuffers(1, &inputSSBO);
    glDeleteBuffers(1, &outputVoxelSSBO);
    glDeleteBuffers(1, &outputChunkSSBO);
    glDeleteBuffers(1, &chunkDataSSBO);
}

void ChunkMatrix::UpdateGridPressure(bool oddPressureUpdatePass)
{
    std::lock_guard<std::mutex> lock(this->voxelMutex);

    std::vector<Volume::VoxelPressureData> VoxelPressureArray(0);

    uint16_t chunkIndex = 0;
    std::vector<Volume::Chunk*> chunksToUpdate;
    for(uint16_t i = 0; i < static_cast<uint16_t>(this->Grid.size()); ++i){
        if (this->Grid[i]->ShouldChunkCalculatePressure()){
            chunksToUpdate.push_back(this->Grid[i]);

            VoxelPressureArray.resize(VoxelPressureArray.size() + Volume::Chunk::CHUNK_SIZE_SQUARED);

            //load the heat maps from chunk
            this->Grid[i]->GetPressureMap(
                this, VoxelPressureArray.data(), chunkIndex++);
            
        }
    }

    uint16_t NumberOfChunks = chunksToUpdate.size();

    uint32_t NumberOfVoxels = Volume::Chunk::CHUNK_SIZE_SQUARED * NumberOfChunks;


    GLuint inputSSBO, chunkDataSSBO, outputVoxelSSBO, outputChunkSSBO;

    glGenBuffers(1, &inputSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
        sizeof(uint32_t) + sizeof(Volume::VoxelPressureData) * NumberOfVoxels,
        nullptr, GL_DYNAMIC_COPY);

    //buffer mapping
    void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 
        sizeof(uint32_t) + sizeof(Volume::VoxelPressureData) * NumberOfVoxels,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    
    uint32_t* VoxelsSizePtr = static_cast<uint32_t*>(ptr);
    *VoxelsSizePtr = NumberOfVoxels;

    Volume::VoxelPressureData* dataPtr = reinterpret_cast<Volume::VoxelPressureData*>(VoxelsSizePtr + 1);
    std::memcpy(dataPtr, VoxelPressureArray.data(), NumberOfVoxels * sizeof(Volume::VoxelPressureData));

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, inputSSBO); // Binding = 0

    //chunk info input
    std::vector<ChunkConnectivityData> chunkData;
    for(uint32_t i = 0; i < static_cast<uint32_t>(this->Grid.size()); ++i){
        if (this->Grid[i]->ShouldChunkCalculatePressure()){
            ChunkConnectivityData d;
            d.chunk = i;
            d.chunkUp = -1;
            d.chunkDown = -1;
            d.chunkLeft = -1;
            d.chunkRight = -1;

            Vec2i pos = this->Grid[i]->GetPos();
            Vec2i posUp = pos + vector::UP;
            Vec2i posDown = pos + vector::DOWN;
            Vec2i posLeft = pos + vector::LEFT;
            Vec2i posRight = pos + vector::RIGHT;
            for(uint32_t j = 0; j < static_cast<uint32_t>(this->Grid.size()); ++j){
                if(this->Grid[j]->GetPos() == posUp)
                    d.chunkUp = j;
                else if(this->Grid[j]->GetPos() == posDown)
                    d.chunkDown = j;
                else if(this->Grid[j]->GetPos() == posLeft)
                    d.chunkLeft = j;
                else if(this->Grid[j]->GetPos() == posRight)
                    d.chunkRight = j;
            }
            chunkData.push_back(d);
        }
    }

    glGenBuffers(1, &chunkDataSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkDataSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, chunkData.size() * sizeof(ChunkConnectivityData), chunkData.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, chunkDataSSBO); // Binding = 1

    // Output Buffer
    glGenBuffers(1, &outputVoxelSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputVoxelSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NumberOfVoxels * sizeof(float), nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, outputVoxelSSBO); // Binding = 2

    glGenBuffers(1, &outputChunkSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputChunkSSBO);
    uint32_t* zeroData = new uint32_t[NumberOfChunks]();
    std::fill(zeroData, zeroData + NumberOfChunks, 0);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NumberOfChunks * sizeof(uint32_t), zeroData, GL_DYNAMIC_COPY);
    delete[] zeroData;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, outputChunkSSBO); // Binding = 3

    // Compute Shader
    glUseProgram(Volume::VoxelElement::computeShaderPressure_Program);
    glDispatchCompute(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, NumberOfChunks);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputVoxelSSBO);
    float* VoxelPressureData = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NumberOfVoxels * sizeof(float), GL_MAP_READ_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputChunkSSBO);
    uint32_t* pressureDiff = (uint32_t*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NumberOfChunks * sizeof(uint32_t), GL_MAP_READ_BIT);

    #pragma omp parallel for
    for(uint32_t i = 0; i < NumberOfVoxels; ++i){
        uint16_t chunkIndex = i / Volume::Chunk::CHUNK_SIZE_SQUARED;
        uint16_t voxelIndex = i % Volume::Chunk::CHUNK_SIZE_SQUARED;
        uint16_t x = voxelIndex % Volume::Chunk::CHUNK_SIZE;
        uint16_t y = voxelIndex / Volume::Chunk::CHUNK_SIZE;

        auto& chunk = chunksToUpdate[chunkIndex];
        chunk->voxels[x][y]->amount = VoxelPressureData[i];
    }

    for(uint16_t i = 0; i < NumberOfChunks; ++i){
        if((pressureDiff[i]/1000.0f) > 0.0001f){
            chunksToUpdate[i]->forcePressureUpdate = true;
            //also force pressure update on neighbours
            Vec2i pos = chunksToUpdate[i]->GetPos();
            Vec2i positions[4] = {
                pos + vector::UP,
                pos + vector::DOWN,
                pos + vector::LEFT,
                pos + vector::RIGHT
            };

            for(auto& p : positions){
                Volume::Chunk *c = GetChunkAtChunkPosition(p);
                if(c) c->forcePressureUpdate = true;
            }
        }else{
            chunksToUpdate[i]->forcePressureUpdate = false;
        }
    }
    
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glDeleteBuffers(1, &inputSSBO);
    glDeleteBuffers(1, &outputVoxelSSBO);
    glDeleteBuffers(1, &chunkDataSSBO);
    glDeleteBuffers(1, &outputChunkSSBO);
}

Volume::VoxelElement* ChunkMatrix::VirtualGetAt(const Vec2i &pos)
{
    Vec2i chunkPos = WorldToChunkPosition(Vec2f(pos));

    if (!IsValidWorldPosition(pos)) return nullptr;

    Chunk *chunk = GetChunkAtChunkPosition(chunkPos);
    if(!chunk){
        chunk = GenerateChunk(chunkPos);
    }

    Volume::VoxelElement *voxel = chunk->voxels[abs(pos.getX() % Chunk::CHUNK_SIZE)][abs(pos.getY() % Chunk::CHUNK_SIZE)];

    if(!voxel){
        return nullptr;
    } 

    chunk->lastCheckedCountDown = 20;

    return voxel;
}

Volume::VoxelElement* ChunkMatrix::VirtualGetAt_NoLoad(const Vec2i &pos)
{
    Vec2i chunkPos = WorldToChunkPosition(Vec2f(pos));

    if (!IsValidWorldPosition(pos)) return nullptr;

    Chunk *chunk = GetChunkAtChunkPosition(chunkPos);
    if(!chunk){
        return nullptr;
    }

    Volume::VoxelElement *voxel = chunk->voxels[abs(pos.getX() % Chunk::CHUNK_SIZE)][abs(pos.getY() % Chunk::CHUNK_SIZE)];

    if(!voxel){
        return nullptr;
    } 

    chunk->lastCheckedCountDown = 20;

    return voxel;
}
// Set a voxel at a specific position, deleting the old one
void ChunkMatrix::VirtualSetAt(Volume::VoxelElement *voxel)
{
    if (!voxel) return; // Check for null pointer

    // Check if chunkPos is within bounds of the Grid
    if (!IsValidWorldPosition(voxel->position)) {
        return;
    }

    // Calculate positions in the chunk and local position
    Vec2i chunkPos = WorldToChunkPosition(Vec2f(voxel->position));
    Vec2i localPos = Vec2i(
        abs(voxel->position.getX() % Chunk::CHUNK_SIZE), 
        abs(voxel->position.getY() % Chunk::CHUNK_SIZE));


    Chunk *chunk = GetChunkAtChunkPosition(chunkPos);

    if (!chunk) {
        chunk = GenerateChunk(chunkPos);
    }

    //delete the old voxel if it exists
    if(chunk->voxels[localPos.getX()][localPos.getY()]){
        delete chunk->voxels[localPos.getX()][localPos.getY()];
    }

    // Set the new voxel and mark for update
    chunk->voxels[localPos.getX()][localPos.getY()] = voxel;

    chunk->dirtyRect.Include(localPos);

    chunk->forceHeatUpdate = true;
    chunk->forcePressureUpdate = true;
    chunk->dirtyRender = true; // Mark the chunk as dirty for rendering
}
// Same as VirtualSetAt but does not delete the old voxel
void ChunkMatrix::VirtualSetAt_NoDelete(Volume::VoxelElement *voxel)
{
    if (!voxel) return; // Check for null pointer

    // Check if chunkPos is within bounds of the Grid
    if (!IsValidWorldPosition(voxel->position)) {
        return;
    }

    // Calculate positions in the chunk and local position
    Vec2i chunkPos = WorldToChunkPosition(Vec2f(voxel->position));
    Vec2i localPos = Vec2i(
        abs(voxel->position.getX() % Chunk::CHUNK_SIZE), 
        abs(voxel->position.getY() % Chunk::CHUNK_SIZE));
    
    Chunk *chunk = GetChunkAtChunkPosition(chunkPos);

    if (!chunk) {
        chunk = GenerateChunk(chunkPos);
    }

    // Set the new voxel and mark for update
    chunk->voxels[localPos.getX()][localPos.getY()] = voxel;

    chunk->dirtyRect.Include(localPos);

    chunk->forceHeatUpdate = true;
    chunk->forcePressureUpdate = true;
    chunk->dirtyRender = true; // Mark the chunk as dirty for rendering
}

void ChunkMatrix::PlaceVoxelAt(const Vec2i &pos, std::string id, Temperature temp, bool placeUnmovableSolids, float amount, bool destructive)
{
    Volume::VoxelElement *voxel = CreateVoxelElement(id, pos, amount, temp, placeUnmovableSolids);

    PlaceVoxelAt(voxel, destructive);
}
void ChunkMatrix::PlaceVoxelAt(Volume::VoxelElement *voxel, bool destructive)
{
    if(!voxel) return; // Check for null pointer

    if(!destructive){
        Volume::VoxelElement* replacedVoxel = this->VirtualGetAt(voxel->position);

        //still remain destructive if the voxel its trying to move is unmovable
        if(replacedVoxel->IsUnmoveableSolid()){
            VirtualSetAt(voxel);
            return;
        }

        //look for the same voxel around this one
        for(Vec2i dir : vector::AROUND4){
            Volume::VoxelElement* neighbour = this->VirtualGetAt_NoLoad(voxel->position + dir);
            if(neighbour && neighbour->properties == replacedVoxel->properties){
                neighbour->amount += replacedVoxel->amount;

                if(replacedVoxel->amount != 0 || replacedVoxel->amount != 0){
                    neighbour->temperature.SetCelsius(
                        (neighbour->temperature.GetCelsius() * neighbour->amount + replacedVoxel->temperature.GetCelsius() * replacedVoxel->amount)
                         / (neighbour->amount + replacedVoxel->amount));
                }
                //whem amount is 0, set temperature to 21
                else neighbour->temperature.SetCelsius(21);


                VirtualSetAt(voxel);
                return;
            }
        }

        //just look up for a while if you find anything to merge to or to push away
        for(uint8_t i = 2; i < 10; i++){
            Vec2i dir = replacedVoxel->position + Vec2i(0, -i);
            VoxelElement* neighbour = this->VirtualGetAt_NoLoad(dir);

            if(!neighbour){
                VirtualSetAt(voxel);
                return;
            }

            //merge into any voxel thats the same type
            if(neighbour->properties == replacedVoxel->properties){
                neighbour->amount += replacedVoxel->amount;

                neighbour->temperature.SetCelsius(
                    (neighbour->temperature.GetCelsius() * neighbour->amount + 
                    replacedVoxel->temperature.GetCelsius() * replacedVoxel->amount)
                     / (neighbour->amount + replacedVoxel->amount));

                VirtualSetAt(voxel);
                return;
            }

            //push away elements with the same state
            if(neighbour->GetState() == replacedVoxel->GetState()){
                PlaceVoxelAt(dir, replacedVoxel->id, replacedVoxel->temperature, replacedVoxel->IsUnmoveableSolid(), replacedVoxel->amount, false);
                VirtualSetAt(voxel);
                return;
            }
        }
        
    }
        
    VirtualSetAt(voxel);
}
void ChunkMatrix::SetFireAt(const Vec2i &pos, std::optional<Volume::Temperature> temp)
{
    VoxelElement *ingitedVoxel = this->VirtualGetAt(pos);
    if(!ingitedVoxel) return;

    std::string fireId;
    if(ingitedVoxel->GetState() == State::Solid)
        fireId = "Fire_Solid";
    else if(ingitedVoxel->GetState() == State::Liquid)
        fireId = "Fire_Liquid";
    else
        fireId = "Fire";
    
    if(temp == std::nullopt){
        temp = ingitedVoxel->temperature;

        if(temp->GetCelsius() < 250)
            temp->SetCelsius(250);
    }

    bool placeAsUnmovableSolid = ingitedVoxel->IsUnmoveableSolid();
    if(placeAsUnmovableSolid){
        // 15% chance of fire becoming movable
        if(rand() % 100 < 15){
            placeAsUnmovableSolid = false;
        }
    }

    // Create a new fire voxel element
    Volume::VoxelElement *fireVoxel = CreateVoxelElement(fireId, pos, ingitedVoxel->amount, *temp, placeAsUnmovableSolid);
    this->PlaceVoxelAt(fireVoxel, true);
}

bool ChunkMatrix::TryToDisplaceGas(const Vec2i &pos, std::string id, Volume::Temperature temp, float amount, bool placeUnmovableSolids)
{
    Volume::VoxelElement *displacedGas = this->VirtualGetAt(pos);
    if(!displacedGas || displacedGas->GetState() != State::Gas || displacedGas->id == "Empty") return false;

    Volume::VoxelElement *voxel = CreateVoxelElement(id, pos, amount, temp, placeUnmovableSolids);

    for(Vec2i dir : vector::AROUND4){
        Volume::VoxelElement* neighbour = this->VirtualGetAt_NoLoad(pos + dir);
        if(neighbour && neighbour->properties == displacedGas->properties){
            neighbour->amount += displacedGas->amount;
            this->VirtualSetAt(voxel);
            return true;
        }
    }

    delete voxel;
    return false;
}

void ChunkMatrix::GetVoxelsInChunkAtWorldPosition(const Vec2f &pos)
{
    //TODO: Implement
}

void ChunkMatrix::GetVoxelsInCubeAtWorldPosition(const Vec2f &start, const Vec2f &end)
{
    //TODO: Implement
}

bool ChunkMatrix::IsValidWorldPosition(const Vec2i &pos) const
{
    return pos.getX() >= Volume::Chunk::CHUNK_SIZE &&
	    pos.getY() >= Volume::Chunk::CHUNK_SIZE;
}

bool ChunkMatrix::IsValidChunkPosition(const Vec2i &pos) const
{
    return pos.getX() > 0 &&
	    pos.getY() > 0;
}

void ChunkMatrix::ExplodeAt(const Vec2i &pos, short int radius)
{
    const uint16_t numOfRays = 360;
    const double angleStep = M_PI * 2 / numOfRays;

    for (uint16_t i = 0; i < numOfRays; i++)
    {
    	double angle = angleStep * i;
    	float dx = static_cast<float>(std::cos(angle));
    	float dy = static_cast<float>(std::sin(angle));

    	Vec2f currentPos = Vec2f(pos);
    	for (int j = 0; j < radius; j++)
    	{
    		currentPos.x(currentPos.getX() + dx);
    		currentPos.y(currentPos.getY() + dy);
            Volume::VoxelElement *voxel = VirtualGetAt(currentPos);
    		if (voxel == nullptr) continue;

    		if (j < radius * 0.2f) {
                PlaceVoxelAt(currentPos, "Fire", Temperature(std::min(300, radius * 70)), false, 5.0f, true);
            }
            else {
                //destroy gas and immovable solids.. create particles for other
    			if (voxel->GetState() == State::Gas || voxel->IsUnmoveableSolid()) 
                    PlaceVoxelAt(currentPos, "Fire", Temperature(radius * 100), false, 1.3f, false);
                else {
                    // +- 0.05 degrees radian
                    double smallAngleDeviation = ((rand() % 1000) / 10000.0f - 0.05f);
                    Particle::AddSolidFallingParticle(this,voxel ,angle + smallAngleDeviation, (radius*1.1f - j)*0.7f);
                    
                    VoxelElement *fireVoxel = CreateVoxelElement("Fire", currentPos, 1.3f, Temperature(radius * 100), false);
                    VirtualSetAt_NoDelete(fireVoxel);
                }
            }
    	}
    }
}

void ChunkMatrix::UpdateParticles()
{
    std::lock_guard<std::mutex> lock(this->voxelMutex);
    // Process particle generators
    for (auto& generator : particleGenerators) {
        generator->TickParticles();
        generator->position = GameEngine::instance->Player.GetCameraPos();
    }

    if (particles.empty() && newParticles.empty()) return;

    // Merge new particles into the main vector before processing
    particles.insert(particles.end(), newParticles.begin(), newParticles.end());
    newParticles.clear();

    for (size_t i = particles.size(); i-- > 0;) {
        if (particles.at(i)->Step(this)) {
            delete particles.at(i);
            particles.erase(particles.begin() + i);
        }
    }
}

void DirtyRect::Include(Vec2i pos)
{
    m_startW.x(std::min(m_startW.getX(), pos.getX()));
    m_startW.y(std::min(m_startW.getY(), pos.getY()));
    m_endW.x(std::max(m_endW.getX(), pos.getX()));
    m_endW.y(std::max(m_endW.getY(), pos.getY()));
}

void DirtyRect::Update()
{
    this->start = m_startW-Vec2i(DIRTY_RECT_PADDING, DIRTY_RECT_PADDING);  
    this->end = m_endW+Vec2i(DIRTY_RECT_PADDING, DIRTY_RECT_PADDING);

    m_startW = Vec2i(INT_MAX, INT_MAX);
    m_endW = Vec2i(INT_MIN, INT_MIN);
}

bool DirtyRect::IsEmpty() const
{
    return this->start.getX() == INT_MAX-DIRTY_RECT_PADDING;
}
