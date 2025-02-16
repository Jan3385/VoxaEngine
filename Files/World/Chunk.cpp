#include "Chunk.h"
#include "Voxel.h"
#include "voxelTypes.h"	
#include <math.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>
#include "../GameEngine.h"

using namespace Volume; 

GLuint Volume::Chunk::computeShaderHeat_Program = 0;
const char* Chunk::computeShaderHeat = R"(#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

#define TEMPERATURE_TRANSITION_SPEED 80

#define CHUNK_SIZE 64
#define CHUNK_SIZE_SQUARED 4096

#define DIRECTION_COUNT 12

const ivec2 directions[DIRECTION_COUNT] = {
    ivec2(-1, -1),
    ivec2(0, -1),
    ivec2(1, -1),
    ivec2(-1, 0),
    ivec2(1, 0),
    ivec2(-1, 1),
    ivec2(0, 1),
    ivec2(1, 1),
    ivec2(-2, 0),
    ivec2(2, 0),
    ivec2(0, -2),
    ivec2(0, 2)
};

struct VoxelHeatData{
    float temperature;
    float capacity;
    float conductivity;
};
struct ChunkConnectivityData{
	uint chunk;
	uint chunkUp;
	uint chunkDown;
	uint chunkLeft;
	uint chunkRight;
};

layout(std430, binding = 0) buffer InputBuffer {
    int NumberOfVoxels;
    // flattened array (c = chunk, x = x, y = y)
    VoxelHeatData voxelTemps[];
};
layout(std430, binding = 1) buffer ChunkBuffer {
    ChunkConnectivityData chunkData[];
};
layout(std430, binding = 2) buffer OutputBuffer {
    float voxelTempsOut[];
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
    int maxHeatDiff = 0;
    int maxHeatTrans = 0;

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
                NumOfValidDirections++;
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
                NumOfValidDirections++;
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
                NumOfValidDirections++;
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
                NumOfValidDirections++;
            }
        }else{
            nIndex = c * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + nPos.x;
            NumOfValidDirections++;
        }
        
        float heatCapacity = voxelTemps[index].capacity/TEMPERATURE_TRANSITION_SPEED;
        float heatDiff = voxelTemps[nIndex].temperature - voxelTemps[index].temperature;
        float heatTrans = heatDiff * voxelTemps[nIndex].conductivity / heatCapacity;

        sum += heatTrans;
    }
    
    voxelTempsOut[index] = voxelTemps[index].temperature + (sum / NumOfValidDirections);
}
)";

//auto voxel = chunks[chunkIndex]->voxels[vX][vY];
//float heat = voxel->temperature.GetCelsius();
//float heatDifference = heat - sectionHeatAverage;
//float heatTransfer = heatDifference*voxel->properties->HeatConductivity*Temperature::HEAT_TRANSFER_SPEED/voxel->properties->HeatCapacity;
//voxel->temperature.SetCelsius(heat - heatTransfer);
//voxel->CheckTransitionTemps(*matrix);
//chunks[chunkIndex]->m_lastMaxHeatDifference = std::max(m_lastMaxHeatDifference, std::abs(heatDifference));
//chunks[chunkIndex]->m_lastMaxHeatTransfer = std::max(m_lastMaxHeatTransfer, std::abs(heatTransfer));

Volume::Chunk::Chunk(const Vec2i &pos) : m_x(pos.getX()), m_y(pos.getY())
{
    this->font = TTF_OpenFont("Fonts/RobotoFont.ttf", 12);
    
    if(!this->font)
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    //std::cout << "Chunk created at: " << m_x << "," << m_y << std::endl;
}

Volume::Chunk::~Chunk()
{

    if (font != nullptr) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    SDL_FreeSurface(this->chunkSurface);

    for (int i = 0; i < static_cast<int>(voxels.size()); i++)
    {
        for (int j = 0; j < static_cast<int>(voxels[i].size()); j++)
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
    return(
        m_lastMaxHeatDifference > 0.75f || m_lastMaxHeatTransfer > 0.3f ||
        forceHeatUpdate
    );
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
void Volume::Chunk::GetHeatMap(ChunkMatrix *matrix, bool offsetCalculations, 
    Volume::VoxelHeatData HeatDataArray[],  // flattened array
    int chunkNumber)
{
    m_lastMaxHeatDifference = 0;
    m_lastMaxHeatTransfer = 0;
    forceHeatUpdate = true;

    //const int offset = offsetCalculations ? Chunk::CHUNK_SIZE/2 : 0;
    const int offset = 0;

    Chunk *chunks[4] = {
        this,
        matrix->GetChunkAtChunkPosition(Vec2i(m_x + 1, m_y)),
        matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y + 1)),
        matrix->GetChunkAtChunkPosition(Vec2i(m_x + 1, m_y + 1))
    };


    #pragma omp parallel for collapse(2)
    for(int x = 0; x < Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Chunk::CHUNK_SIZE; ++y){
            int vX = offset + x;
            int vY = offset + y;
            int chunkIndex = 0;
            if(vX >= CHUNK_SIZE){
                vX -= CHUNK_SIZE;
                chunkIndex += 1;
            }
            if(vY >= CHUNK_SIZE){
                vY -= CHUNK_SIZE;
                chunkIndex += 2;
            }
            
            if(chunks[chunkIndex] != nullptr){
                int index = chunkNumber * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE + vY * Chunk::CHUNK_SIZE + vX;
                HeatDataArray[index].temperature = chunks[chunkIndex]->voxels[vX][vY]->temperature.GetCelsius();
                HeatDataArray[index].capacity = chunks[chunkIndex]->voxels[vX][vY]->properties->HeatCapacity;
                HeatDataArray[index].conductivity = chunks[chunkIndex]->voxels[vX][vY]->properties->HeatConductivity;
            }
        }
    }
}

//transfer heat to the bordering chunks
/*void Volume::Chunk::TransferBorderHeat(ChunkMatrix *matrix)
{
    Chunk* ChunkUp = matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y - 1));
    Chunk* ChunkDown = matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y + 1));
    for (short int x = 0; x < Chunk::CHUNK_SIZE; x++)
    {
        VoxelElement* voxelUp = voxels[x][0].get();
        VoxelElement* voxelDown = voxels[x][Chunk::CHUNK_SIZE - 1].get();

        VoxelElement* neighbourVoxelUp = ChunkUp ? ChunkUp->voxels[x][Chunk::CHUNK_SIZE - 1].get() : nullptr;
        VoxelElement* neighbourVoxelDown = ChunkDown ? ChunkDown->voxels[x][0].get() : nullptr;

        if(neighbourVoxelUp){
            float heatDifference = voxelUp->temperature.GetCelsius() - neighbourVoxelUp->temperature.GetCelsius();
            float maxHeatTransfer = heatDifference * 0.6f; // Limit to 60%

            float heatTransfer = std::clamp(
                heatDifference * voxelUp->properties->HeatConductivity * Temperature::HEAT_TRANSFER_SPEED,
                -maxHeatTransfer,
                maxHeatTransfer
            );

            voxelUp->temperature.SetCelsius(voxelUp->temperature.GetCelsius() - heatTransfer / voxelUp->properties->HeatCapacity);
            neighbourVoxelUp->temperature.SetCelsius(neighbourVoxelUp->temperature.GetCelsius() + heatTransfer / neighbourVoxelUp->properties->HeatCapacity);
        
            if(heatDifference > .5f){
                ChunkUp->forceHeatUpdate = true;
            }
        }
        if(neighbourVoxelDown){
            float heatDifference = voxelDown->temperature.GetCelsius() - neighbourVoxelDown->temperature.GetCelsius();
            float maxHeatTransfer = heatDifference * 0.6f; // Limit to 60%

            float heatTransfer = std::clamp(
                heatDifference * voxelDown->properties->HeatConductivity * Temperature::HEAT_TRANSFER_SPEED,
                -maxHeatTransfer,
                maxHeatTransfer
            );

            voxelDown->temperature.SetCelsius(voxelDown->temperature.GetCelsius() - heatTransfer / voxelDown->properties->HeatCapacity);
            neighbourVoxelDown->temperature.SetCelsius(neighbourVoxelDown->temperature.GetCelsius() + heatTransfer / neighbourVoxelDown->properties->HeatCapacity);
        
            if(heatDifference > .5f){
                ChunkDown->forceHeatUpdate = true;
            }
        }
    }
    
    Chunk* ChunkLeft = matrix->GetChunkAtChunkPosition(Vec2i(m_x - 1, m_y));
    Chunk* ChunkRight = matrix->GetChunkAtChunkPosition(Vec2i(m_x + 1, m_y));
    for (short int y = 0; y < Chunk::CHUNK_SIZE; y++)
    {
        VoxelElement* voxelLeft = voxels[0][y].get();
        VoxelElement* voxelRight = voxels[Chunk::CHUNK_SIZE - 1][y].get();

        VoxelElement* neighbourVoxelLeft = ChunkLeft ? ChunkLeft->voxels[Chunk::CHUNK_SIZE - 1][y].get() : nullptr;
        VoxelElement* neighbourVoxelRight = ChunkRight ? ChunkRight->voxels[0][y].get() : nullptr;

        if(neighbourVoxelLeft){
            float heatDifference = voxelLeft->temperature.GetCelsius() - neighbourVoxelLeft->temperature.GetCelsius();
            float maxHeatTransfer = heatDifference * 0.6f; // Limit to 60%

            float heatTransfer = std::clamp(
                heatDifference * voxelLeft->properties->HeatConductivity * Temperature::HEAT_TRANSFER_SPEED,
                -maxHeatTransfer,
                maxHeatTransfer
            );

            voxelLeft->temperature.SetCelsius(voxelLeft->temperature.GetCelsius() - heatTransfer / voxelLeft->properties->HeatCapacity);
            neighbourVoxelLeft->temperature.SetCelsius(neighbourVoxelLeft->temperature.GetCelsius() + heatTransfer / neighbourVoxelLeft->properties->HeatCapacity);

            if(heatDifference > .5f){
                ChunkLeft->forceHeatUpdate = true;
            }
        }
        if(neighbourVoxelRight){
            float heatDifference = voxelRight->temperature.GetCelsius() - neighbourVoxelRight->temperature.GetCelsius();
            float maxHeatTransfer = heatDifference * 0.6f; // Limit to 60%

            float heatTransfer = std::clamp(
                heatDifference * voxelRight->properties->HeatConductivity * Temperature::HEAT_TRANSFER_SPEED,
                -maxHeatTransfer,
                maxHeatTransfer
            );

            voxelRight->temperature.SetCelsius(voxelRight->temperature.GetCelsius() - heatTransfer / voxelRight->properties->HeatCapacity);
            neighbourVoxelRight->temperature.SetCelsius(neighbourVoxelRight->temperature.GetCelsius() + heatTransfer / neighbourVoxelRight->properties->HeatCapacity);

            if(heatDifference > .5f){
                ChunkRight->forceHeatUpdate = true;
            }
        }
    }
}*/

void Volume::Chunk::ResetVoxelUpdateData()
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
    // Calculate the chunk coordinates
    int x1 = 0;
    int y1 = 0;
    int x2 = CHUNK_SIZE * RENDER_VOXEL_SIZE;
    int y2 = CHUNK_SIZE * RENDER_VOXEL_SIZE;

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
                //color.r = SDL_clamp(color.r, 0, 255);
                //color.g = 0;
                //color.b = SDL_clamp(255 - voxels[x][y]->temperature.GetCelsius(), 0, 255);
                //const RGB& color = RGB(voxels[x][y].get()->position.getX() / 2, 0, voxels[x][y].get()->position.getY() / 2);

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
    for (auto& particle : particles) {
        const RGB& color = particle->color;

        SDL_SetRenderDrawColor(&renderer, color.r, color.g, color.b, 255);

        SDL_Rect rect = {
            static_cast<int>((particle->position.getX()) * Chunk::RENDER_VOXEL_SIZE + offset.getX()),
            static_cast<int>((particle->position.getY()) * Chunk::RENDER_VOXEL_SIZE + offset.getY()),
            Chunk::RENDER_VOXEL_SIZE,
            Chunk::RENDER_VOXEL_SIZE
        };
        SDL_RenderFillRect(&renderer, &rect);
    }
}
ChunkMatrix::ChunkMatrix()
{
}

ChunkMatrix::~ChunkMatrix()
{
    this->cleanup();
}

void ChunkMatrix::cleanup()
{
    if(cleaned) return;

    for(int i = 0; i < 4; ++i)
    {
        for(int j = GridSegmented[i].size() - 1; j >= 0; --j)
        {
            delete GridSegmented[i][j];
        }
        GridSegmented[i].clear();
    }
    
    for (auto& particle : particles) {
    	delete particle;
    }
    particles.clear();
    
    for (auto& particle : newParticles) {
        delete particle;
    }
    newParticles.clear();
    
    cleaned = true;
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

    int AssignedGridPass = 0;
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

    int AssignedGridPass = 0;
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
            PlaceVoxelAt(MouseWorldPosI + Vec2i(x, y), id, temp, GameEngine::placeUnmovableSolidVoxels);
        }
    }
}

void ChunkMatrix::PlaceParticleAtMousePosition(const Vec2f &pos, std::string id, Vec2f offset, float angle, float speed)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);

    if(!IsValidWorldPosition(MouseWorldPos)) return;

    AddParticle(id, MouseWorldPosI, Temperature(21), angle, speed);
}

void ChunkMatrix::RemoveVoxelAtMousePosition(const Vec2f &pos, Vec2f offset)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);

    if(!IsValidWorldPosition(MouseWorldPos)) return;

    PlaceVoxelAt(MouseWorldPosI, "Oxygen", Temperature(21), false);
}

void ChunkMatrix::ExplodeAtMousePosition(const Vec2f &pos, short int radius, Vec2f offset)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);

    if(!IsValidWorldPosition(MouseWorldPos)) return;

    this->ExplodeAt(MouseWorldPosI, radius);
}

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
                        true);
                }else{
                    chunk->voxels[x][y] = new VoxelSolid
                        ("Dirt", 
                        Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE,y + pos.getY() * Chunk::CHUNK_SIZE), 
                        Temperature(21), 
                        true);
                }
            }
            else {
                chunk->voxels[x][y] = new VoxelGas
                    ("Oxygen", 
                    Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE, y + pos.getY() * Chunk::CHUNK_SIZE),
                    Temperature(21));
            }
    		if (pos.getY() == 4 && pos.getX() <= 3) {
    			chunk->voxels[x][y] = new VoxelSolid
                    ("Sand", 
                    Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE, y + pos.getY() * Chunk::CHUNK_SIZE),
                    Temperature(21),
                    false);
    		}
        }
    }

    int AssignedGridPass = 0;
    if (pos.getX() % 2 != 0) AssignedGridPass += 1;
    if (pos.getY() % 2 != 0) AssignedGridPass += 2;
    this->GridSegmented[AssignedGridPass].push_back(chunk);

    this->Grid.push_back(chunk);

    return chunk;
}

void ChunkMatrix::DeleteChunk(const Vec2i &pos)
{
    int AssignedGridPass = 0;
    if (pos.getX() % 2 != 0) AssignedGridPass += 1;
    if (pos.getY() % 2 != 0) AssignedGridPass += 2;

    for (long long int i = this->GridSegmented[AssignedGridPass].size()-1; i >= 0; --i)
    {
        if(this->GridSegmented[AssignedGridPass][i]->GetPos() == pos)
        {
            this->GridSegmented[AssignedGridPass].erase(this->GridSegmented[AssignedGridPass].begin() + i);
            break;
        }
    }

    for (long long int i = this->Grid.size()-1; i >= 0; --i)
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
    int NumberOfChunks = this->Grid.size();

    int NumberOfVoxels = Volume::Chunk::CHUNK_SIZE * Volume::Chunk::CHUNK_SIZE * NumberOfChunks;

    std::vector<Volume::VoxelHeatData> VoxelHeatArray(NumberOfVoxels);

    // doesnt create critical section, because the passes dont overlap
    int chunkIndex = 0;
    std::vector<Volume::Chunk*> chunksToUpdate;
    for(int i = 0; i < static_cast<int>(this->Grid.size()); ++i){
        if (this->Grid[i]->ShouldChunkCalculateHeat()){
            chunksToUpdate.push_back(this->Grid[i]);

            //load the heat maps from chunk
            this->Grid[i]->GetHeatMap(
                this, oddHeatUpdatePass,
                VoxelHeatArray.data(), chunkIndex++);
            
        }
    }

    GLuint inputSSBO, outputSSBO;

    glGenBuffers(1, &inputSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, inputSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
        sizeof(int) + sizeof(Volume::VoxelHeatData) * NumberOfVoxels,
        nullptr, GL_DYNAMIC_COPY);

    //buffer mapping
    void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 
        sizeof(int) + sizeof(Volume::VoxelHeatData) * NumberOfVoxels,
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    
    int* VoxelsSizePtr = static_cast<int*>(ptr);
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

            Vec2i pos = this->Grid[i]->GetPos();
            Vec2i posUp = pos + Vec2i(0, -1);
            Vec2i posDown = pos + Vec2i(0, 1);
            Vec2i posLeft = pos + Vec2i(-1, 0);
            Vec2i posRight = pos + Vec2i(1, 0);
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

    GLuint chunkDataSSBO;
    glGenBuffers(1, &chunkDataSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkDataSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, chunkData.size() * sizeof(ChunkConnectivityData), chunkData.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, chunkDataSSBO); // Binding = 1

    // Output Buffer
    glGenBuffers(1, &outputSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NumberOfVoxels * sizeof(Volume::VoxelHeatData), nullptr, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, outputSSBO); // Binding = 2

    // Compute Shader
    glUseProgram(Volume::Chunk::computeShaderHeat_Program);
    glDispatchCompute(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, NumberOfChunks);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Read back the data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputSSBO);
    float* outputData = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NumberOfVoxels * sizeof(float), GL_MAP_READ_BIT);

    const int ChunkSizeSquared = Volume::Chunk::CHUNK_SIZE * Volume::Chunk::CHUNK_SIZE;

    #pragma omp parallel for
    for(int i = 0; i < NumberOfVoxels; ++i){
        int chunkIndex = i / ChunkSizeSquared;
        int voxelIndex = i % ChunkSizeSquared;
        int x = voxelIndex % Volume::Chunk::CHUNK_SIZE;
        int y = voxelIndex / Volume::Chunk::CHUNK_SIZE;

        auto& chunk = chunksToUpdate[chunkIndex];
        chunk->voxels[x][y]->temperature.SetCelsius(outputData[i]);
        chunk->voxels[x][y]->CheckTransitionTemps(*this);
    }
    
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glDeleteBuffers(1, &inputSSBO);
    glDeleteBuffers(1, &outputSSBO);
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
    chunk->dirtyRender = true; // Mark the chunk as dirty for rendering
}

void ChunkMatrix::PlaceVoxelAt(const Vec2i &pos, std::string id, Temperature temp, bool placeUnmovableSolids)
{
    Vec2i chunkPos = WorldToChunkPosition(Vec2f(pos));
    Volume::VoxelElement *voxel;

    VoxelProperty* prop = VoxelRegistry::GetProperties(id);    

    if(id == "Fire")
        voxel = new Volume::FireVoxel(pos, temp);
    else{
        if(prop->state == State::Gas)
            voxel = new Volume::VoxelGas(id, pos, temp);
        else if(prop->state == State::Liquid)
            voxel = new Volume::VoxelLiquid(id, pos, temp);
        else{
            voxel = new Volume::VoxelSolid(id, pos, temp, placeUnmovableSolids);
        }
    } 
        
    VirtualSetAt(voxel);
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
    return pos.getX() >= Volume::Chunk::CHUNK_SIZE && pos.getX() &&
	    pos.getY() >= Volume::Chunk::CHUNK_SIZE && pos.getY();
}

bool ChunkMatrix::IsValidChunkPosition(const Vec2i &pos) const
{
    return pos.getX() > 0 && pos.getX() &&
	    pos.getY() > 0 && pos.getY();
}

void ChunkMatrix::ExplodeAt(const Vec2i &pos, short int radius)
{
    const short int numOfRays = 360;
    const double angleStep = M_PI * 2 / numOfRays;

    for (int i = 0; i < numOfRays; i++)
    {
    	double angle = angleStep * i;
    	float dx = static_cast<float>(std::cos(angle));
    	float dy = static_cast<float>(std::sin(angle));

    	Vec2f currentPos = Vec2f(pos);
    	for (int j = 0; j < radius; j++)
    	{
    		currentPos.x(currentPos.getX() + dx);
    		currentPos.y(currentPos.getY() + dy);
            Volume::VoxelElement *voxel = VirtualGetAt(Vec2i(currentPos));
    		if (voxel == nullptr) continue;

    		if (j < radius * 0.2f) {
                PlaceVoxelAt(currentPos, "Fire", Temperature(std::min(300, radius * 70)), false);
            }
            else {
                //destroy gas and immovable solids.. create particles for other
    			if (voxel->GetState() == State::Gas || voxel->IsUnmoveableSolid()) 
                    PlaceVoxelAt(currentPos, "Fire", Temperature(radius * 100), false);
                else {
    				AddParticle(voxel->id, Vec2i(currentPos), voxel->temperature, static_cast<float>(angle), (radius*1.1f - j)*0.7f);
                    PlaceVoxelAt(currentPos, "Fire", Temperature(radius * 100), false);
                }
            }
    	}
    }
}

void ChunkMatrix::UpdateParticles()
{
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

void ChunkMatrix::AddParticle(std::string id, const Vec2i &position, Temperature temp, float angle, float speed)
{
    this->newParticles.push_back(new VoxelParticle(id, position, temp, angle, speed));
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
