#include "Chunk.h"
#include "Voxel.h"
#include "voxelTypes.h"	
#include <math.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include "../GameEngine.h"

using namespace Volume; 
Volume::Chunk::Chunk(const Vec2i &pos) : m_x(pos.getX()), m_y(pos.getY())
{
    this->font = TTF_OpenFont("Fonts/RobotoFont.ttf", 12);
    if(!this->font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }
    std::cout << "Chunk created at: " << m_x << "," << m_y << std::endl;
}

Volume::Chunk::~Chunk()
{

    if (font != nullptr) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    SDL_FreeSurface(this->chunkSurface);
}
bool Volume::Chunk::ShouldChunkDelete(AABB &Camera) const
{
    if(lastCheckedCountDown > 0) return false;
    if(!this->dirtyRect.IsEmpty()) return false;
    if(this->GetAABB().Overlaps(Camera)) return false;

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
void Volume::Chunk::UpdateHeat(ChunkMatrix *matrix, bool offsetCalculations)
{
    m_lastMaxHeatDifference = 0;
    m_lastMaxHeatTransfer = 0;
    forceHeatUpdate = false;

    constexpr int sections = 8;
    constexpr int sectionSize = CHUNK_SIZE / sections;
    constexpr int sectionVolume = sectionSize * sectionSize;

    const int offset = offsetCalculations ? sectionSize/2 : 0;

    Chunk *chunks[4] = {
        this,
        matrix->GetChunkAtChunkPosition(Vec2i(m_x + 1, m_y)),
        matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y + 1)),
        matrix->GetChunkAtChunkPosition(Vec2i(m_x + 1, m_y + 1))
    };

    for(int s_x = 0; s_x < sections; ++s_x){
        int sectionOffsetX = s_x * sectionSize;
        for(int s_y = 0; s_y < sections; ++s_y){
            int sectionOffsetY = s_y * sectionSize;

            double sectionHeatSum = 0;
            for(int x = offset; x < sectionSize+offset; ++x){
                for(int y = offset; y < sectionSize+offset; ++y){
                    int vX = sectionOffsetX + x;
                    int vY = sectionOffsetY + y;

                    int chunkIndex = 0;
                    if(vX >= CHUNK_SIZE){
                        vX -= CHUNK_SIZE;
                        chunkIndex += 1;
                    }
                    if(vY >= CHUNK_SIZE){
                        vY -= CHUNK_SIZE;
                        chunkIndex += 2;
                    }
                    
                    if(chunks[chunkIndex] != nullptr)
                        sectionHeatSum += chunks[chunkIndex]->voxels[vX][vY]->temperature.GetCelsius();
                }
            }
            double sectionHeatAverage = sectionHeatSum / sectionVolume;

            for(int x = offset; x < sectionSize+offset; ++x){
                for(int y = offset; y < sectionSize+offset; ++y){
                    int vX = sectionOffsetX + x;
                    int vY = sectionOffsetY + y;

                    int chunkIndex = 0;
                    if(vX >= CHUNK_SIZE){
                        vX -= CHUNK_SIZE;
                        chunkIndex += 1;
                    }
                    if(vY >= CHUNK_SIZE){
                        vY -= CHUNK_SIZE;
                        chunkIndex += 2;
                    }

                    if(chunks[chunkIndex] == nullptr) continue;

                    auto voxel = chunks[chunkIndex]->voxels[vX][vY];
                    float heat = voxel->temperature.GetCelsius();
                    float heatDifference = heat - sectionHeatAverage;
                    
                    float heatTransfer = heatDifference*voxel->properties->HeatConductivity*Temperature::HEAT_TRANSFER_SPEED/voxel->properties->HeatCapacity;

                    voxel->temperature.SetCelsius(heat - heatTransfer);
                    voxel->CheckTransitionTemps(*matrix);

                    chunks[chunkIndex]->m_lastMaxHeatDifference = std::max(m_lastMaxHeatDifference, std::abs(heatDifference));
                    chunks[chunkIndex]->m_lastMaxHeatTransfer = std::max(m_lastMaxHeatTransfer, std::abs(heatTransfer));
                }
            }
        }
    }

    //std::cout << "Heat difference: " << m_lastMaxHeatDifference << " Heat transfer: " << m_lastMaxHeatTransfer << std::endl;
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

void Volume::Chunk::ResetVoxelUpdateData(ChunkMatrix *matrix)
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
                    chunk->voxels[x][y] = std::make_shared<VoxelImmovableSolid>
                        ("Grass", 
                        Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE,y + pos.getY() * Chunk::CHUNK_SIZE), 
                        Temperature(21));
                }else{
                    chunk->voxels[x][y] = std::make_shared<VoxelImmovableSolid>
                        ("Dirt", 
                        Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE,y + pos.getY() * Chunk::CHUNK_SIZE), 
                        Temperature(21));
                }
            }
            else {
                chunk->voxels[x][y] = std::make_shared<VoxelGas>
                    ("Oxygen", 
                    Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE, y + pos.getY() * Chunk::CHUNK_SIZE),
                    Temperature(21));
            }
    		if (pos.getY() == 4 && pos.getX() <= 3) {
    			chunk->voxels[x][y] = std::make_shared<VoxelMovableSolid>
                    ("Sand", 
                    Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE, y + pos.getY() * Chunk::CHUNK_SIZE),
                    Temperature(21));
    		}
        }
    }

    int AssignedGridPass = 0;
    if (pos.getX() % 2 != 0) AssignedGridPass += 1;
    if (pos.getY() % 2 != 0) AssignedGridPass += 2;
    this->GridSegmented[AssignedGridPass].push_back(chunk);

    return chunk;
}

void ChunkMatrix::DeleteChunk(const Vec2i &pos)
{
    int AssignedGridPass = 0;
    if (pos.getX() % 2 != 0) AssignedGridPass += 1;
    if (pos.getY() % 2 != 0) AssignedGridPass += 2;

    for (size_t i = this->GridSegmented[AssignedGridPass].size()-1; i >= 0; --i)
    {
        if(this->GridSegmented[AssignedGridPass][i]->GetPos() == pos)
        {
            Chunk *c = this->GridSegmented[AssignedGridPass][i];
            this->GridSegmented[AssignedGridPass].erase(this->GridSegmented[AssignedGridPass].begin() + i);
            delete c;
            return;
        }
    }
}

std::shared_ptr<Volume::VoxelElement> ChunkMatrix::VirtualGetAt(const Vec2i &pos)
{
    Vec2i chunkPos = WorldToChunkPosition(Vec2f(pos));

    if (!IsValidWorldPosition(pos)) return nullptr;

    Chunk *chunk = GetChunkAtChunkPosition(chunkPos);
    if(!chunk){
        chunk = GenerateChunk(chunkPos);
    }

    std::shared_ptr<Volume::VoxelElement> voxel = chunk->voxels[abs(pos.getX() % Chunk::CHUNK_SIZE)][abs(pos.getY() % Chunk::CHUNK_SIZE)];

    if(!voxel){
        return nullptr;
    } 

    chunk->lastCheckedCountDown = 20;

    return voxel;
}

std::shared_ptr<Volume::VoxelElement> ChunkMatrix::VirtualGetAtNoLoad(const Vec2i &pos)
{
    Vec2i chunkPos = WorldToChunkPosition(Vec2f(pos));

    if (!IsValidWorldPosition(pos)) return nullptr;

    Chunk *chunk = GetChunkAtChunkPosition(chunkPos);
    if(!chunk){
        return nullptr;
    }

    std::shared_ptr<Volume::VoxelElement> voxel = chunk->voxels[abs(pos.getX() % Chunk::CHUNK_SIZE)][abs(pos.getY() % Chunk::CHUNK_SIZE)];

    if(!voxel){
        return nullptr;
    } 

    chunk->lastCheckedCountDown = 20;

    return voxel;
}

void ChunkMatrix::VirtualSetAt(std::shared_ptr<Volume::VoxelElement> voxel)
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
    std::shared_ptr<Volume::VoxelElement> voxel;

    VoxelProperty* prop = VoxelRegistry::GetProperties(id);    

    if(id == "Fire")
        voxel = std::make_shared<Volume::FireVoxel>(pos, temp);
    else{
        if(prop->state == State::Gas)
            voxel = std::make_shared<Volume::VoxelGas>(id, pos, temp);
        else if(prop->state == State::Liquid)
            voxel = std::make_shared<Volume::VoxelLiquid>(id, pos, temp);
        else{
            if(placeUnmovableSolids)
                voxel = std::make_shared<Volume::VoxelImmovableSolid>(id, pos, temp);
            else
                voxel = std::make_shared<Volume::VoxelMovableSolid>(id, pos, temp);
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
            std::shared_ptr<Volume::VoxelElement> voxel = VirtualGetAt(Vec2i(currentPos));
    		if (voxel == nullptr) continue;

    		if (j < radius * 0.2f) {
                PlaceVoxelAt(currentPos, "Fire", Temperature(std::min(300, radius * 70)), false);
            }
            else {
                //destroy gas and immovable solids.. create particles for other
    			if (voxel->GetState() == VoxelState::Gas || voxel->GetState() == VoxelState::ImmovableSolid) 
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
    this->start = m_startW-Vec2i(2,2);  
    this->end = m_endW+Vec2i(2,2);

    m_startW = Vec2i(INT_MAX, INT_MAX);
    m_endW = Vec2i(INT_MIN, INT_MIN);
}

bool DirtyRect::IsEmpty() const
{
    return this->start.getX() == INT_MAX-2;
}
