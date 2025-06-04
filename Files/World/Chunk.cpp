#include "World/Chunk.h"
#include "World/Voxel.h"
#include "World/VoxelTypes.h"	
#include <math.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>
#include "GameEngine.h"
#include "World/Particles/SolidFallingParticle.h"
#include "World/ParticleGenerators/LaserParticleGenerator.h"


using namespace Volume; 

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

/**
 * Fills the provided buffers with data for shaders.
 * provides flattened arrays of voxel data for shaders
 */
void Volume::Chunk::GetShadersData(
    float temperatureBuffer[], 
    float heatCapacityBuffer[], 
    float heatConductivityBuffer[], 
    float pressureBuffer[], 
    uint32_t idBuffer[], 
    int chunkNumber) const
{
    #pragma omp parallel for collapse(2)
    for(int x = 0; x < Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Chunk::CHUNK_SIZE; ++y){
            int index = chunkNumber * Chunk::CHUNK_SIZE_SQUARED + y * Chunk::CHUNK_SIZE + x;

            Volume::VoxelElement* voxel = this->voxels[x][y];
            const Volume::VoxelProperty* props = voxel->properties;

            temperatureBuffer[index] = voxel->temperature.GetCelsius();
            heatCapacityBuffer[index] = props->HeatCapacity;
            heatConductivityBuffer[index] = props->HeatConductivity;
            pressureBuffer[index] = voxel->amount;
            idBuffer[index] = props->id |
                (static_cast<uint32_t>(voxel->GetState() != State::Gas) << 31) |
                (static_cast<uint32_t>(voxel->GetState() == State::Liquid) << 30);
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

