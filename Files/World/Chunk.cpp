#include "Chunk.h"
#include "Voxel.h"
#include "voxelTypes.h"	
#include <math.h>
#include <cmath>
#include <iostream>

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
    if (this->font != nullptr) {
        TTF_CloseFont(this->font);
        this->font = nullptr;
    }
}
bool Volume::Chunk::ShouldChunkDelete(AABB &Camera)
{
    if(lastCheckedCountDown > 0) return false;
    if(updateVoxelsNextFrame) return false;
    if(this->GetAABB().Overlaps(Camera)) return false;

    return true;
}
void Volume::Chunk::UpdateVoxels(ChunkMatrix *matrix)
{
    this->updateVoxelsNextFrame = false;
    for (int x = CHUNK_SIZE - 1; x >= 0; --x)
    {
        for (int y = 0; y < CHUNK_SIZE; ++y)
        {
    		if (voxels[x][y]->Step(matrix)) {
    			this->updateVoxelsNextFrame = true;
    			//if voxel is at the edge of chunk, update neighbour chunk
    			if (x == 0) {
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x - 1, m_y));
                    if(c) c->updateVoxelsNextFrame = true;
    			}
    			else if (x == CHUNK_SIZE - 1) {
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x + 1, m_y));
                    if (c) c->updateVoxelsNextFrame = true;
    			}
    			if (y == 0) {
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y - 1));
                    if (c) c->updateVoxelsNextFrame = true;
    			}
    			else if (y == CHUNK_SIZE - 1) {
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y + 1));
                    if (c) c->updateVoxelsNextFrame = true;
    			}
    		}

        }
    }
}

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

SDL_Surface* Volume::Chunk::Render(SDL_Renderer &WindowRenderer, Vec2f offset) const //Old renderer took ~16 600 microseconds
{
    SDL_Surface surface = SDL_CreateRGBSurfaceWithFormat(0, CHUNK_SIZE * RENDER_VOXEL_SIZE, CHUNK_SIZE * RENDER_VOXEL_SIZE, 32, SDL_PIXELFORMAT_RGBA8888);
    
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            const RGB& color = voxels[x][y]->color;
            //const RGB& color = RGB(voxels[x][y].get()->position.getX() / 2, 0, voxels[x][y].get()->position.getY() / 2);

            SDL_Rect rect = {
                static_cast<int>((x + m_x * CHUNK_SIZE) * RENDER_VOXEL_SIZE + offset.getX()),
                static_cast<int>((y + m_y * CHUNK_SIZE) * RENDER_VOXEL_SIZE + offset.getY()),
                RENDER_VOXEL_SIZE,
                RENDER_VOXEL_SIZE
            };
            SDL_FillRect(surface, &rect, SDL_MapRGBA(surface->format, color.r, color.g, color.b, 255));
        }
    }

    // Render border
    SDL_Color borderColor = updateVoxelsNextFrame ? SDL_Color{255, 255, 255, 255} : SDL_Color{255, 0, 0, 255};
    SDL_SetRenderDrawColor(&WindowRenderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);

    int x1 = m_x * CHUNK_SIZE * RENDER_VOXEL_SIZE + offset.getX();
    int y1 = m_y * CHUNK_SIZE * RENDER_VOXEL_SIZE + offset.getY();
    int x2 = (m_x + 1) * CHUNK_SIZE * RENDER_VOXEL_SIZE + offset.getX();
    int y2 = (m_y + 1) * CHUNK_SIZE * RENDER_VOXEL_SIZE + offset.getY();

    SDL_RenderDrawLine(&WindowRenderer, x1, y1, x2, y1); // Top
    SDL_RenderDrawLine(&WindowRenderer, x2, y1, x2, y2); // Right
    SDL_RenderDrawLine(&WindowRenderer, x2, y2, x1, y2); // Bottom
    SDL_RenderDrawLine(&WindowRenderer, x1, y2, x1, y1); // Left

    //Render chunk position

    SDL_Color textColor = {255, 255, 255, 255};
    std::string text = std::to_string(m_x) + "," + std::to_string(m_y);
    SDL_Surface* textSurface = TTF_RenderText_Solid(this->font, text.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(&WindowRenderer, textSurface);
    SDL_Rect textRect = {x1, y1, textSurface->w, textSurface->h};
    SDL_RenderCopy(&WindowRenderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
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
        for(auto& chunk : GridSegmented[i])
        {
            delete chunk;
        }
        GridSegmented[i].clear();
    }
    //Grid.clear();
    
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

void ChunkMatrix::PlaceVoxelsAtMousePosition(const Vec2f &pos, Volume::VoxelType elementType, Vec2f offset)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);

    constexpr int size = 5;

    for (int x = -size; x <= size; x++)
    {
        for (int y = -size; y <= size; y++)
        {
            PlaceVoxelAt(MouseWorldPosI + Vec2i(x, y), elementType);
        }
    }
}

void ChunkMatrix::PlaceParticleAtMousePosition(const Vec2f &pos, Volume::VoxelType particleType, Vec2f offset, float angle, float speed)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);

    AddParticle(particleType, MouseWorldPosI, angle, speed);
}

void ChunkMatrix::RemoveVoxelAtMousePosition(const Vec2f &pos, Vec2f offset)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);
    PlaceVoxelAt(MouseWorldPosI, Volume::VoxelType::Oxygen);
}

void ChunkMatrix::ExplodeAtMousePosition(const Vec2f &pos, short int radius, Vec2f offset)
{
    Vec2f MouseWorldPos = MousePosToWorldPos(pos, offset);
    Vec2i MouseWorldPosI(MouseWorldPos);
    this->ExplodeAt(MouseWorldPosI, radius);
}

Volume::Chunk* ChunkMatrix::GenerateChunk(const Vec2i &pos)
{
    /*
    if (this->Grid.size() <= pos.x)
    	this->Grid.resize(static_cast<std::size_t>(pos.x) + 1);
    if (this->Grid[pos.x].size() <= pos.y)
    	this->Grid[pos.x].resize(static_cast<std::size_t>(pos.y) + 1);
    */

    Chunk *chunk = new Chunk(pos);
    // Fill the voxels array with Voxel objects
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
    		//Create voxel determining on its position
            if (pos.getY() > 4) {
                chunk->voxels[x][y] = std::make_shared<VoxelImmovableSolid>(VoxelType::Dirt, Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE,y + pos.getY() * Chunk::CHUNK_SIZE));
            }
            else {
                chunk->voxels[x][y] = std::make_shared<VoxelGas>(VoxelType::Oxygen, Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE, y + pos.getY() * Chunk::CHUNK_SIZE));
            }
    		if (pos.getY() == 4 && pos.getX() <= 3) {
    			chunk->voxels[x][y] = std::make_shared<VoxelMovableSolid>(VoxelType::Sand, Vec2i(x + pos.getX() * Chunk::CHUNK_SIZE, y + pos.getY() * Chunk::CHUNK_SIZE));
    		}
        }
    }
    //this->Grid[pos.getX()][pos.getY()] = chunk;

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

    for (size_t i = 0; i < this->GridSegmented[AssignedGridPass].size(); i++)
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

    if(!chunk->voxels[abs(pos.getX() % Chunk::CHUNK_SIZE)][pos.getY() % Chunk::CHUNK_SIZE]){
        return nullptr;
    } 

    chunk->lastCheckedCountDown = 20;

    return chunk->voxels[abs(pos.getX() % Chunk::CHUNK_SIZE)][abs(pos.getY() % Chunk::CHUNK_SIZE)];
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
    chunk->updateVoxelsNextFrame = true;

    //update grids near if its on their border
    if (localPos.getX() == 0) {
        Chunk* c = this->GetChunkAtChunkPosition(Vec2i(chunkPos.getX() - 1, chunkPos.getY()));
        if (c) c->updateVoxelsNextFrame = true;
    }
    else if (localPos.getX() == Chunk::CHUNK_SIZE - 1) {
        Chunk* c = this->GetChunkAtChunkPosition(Vec2i(chunkPos.getX() + 1, chunkPos.getY()));
        if (c) c->updateVoxelsNextFrame = true;
    }

    if (localPos.getY() == 0) {
        Chunk* c = this->GetChunkAtChunkPosition(Vec2i(chunkPos.getX(), chunkPos.getY() - 1));
        if (c) c->updateVoxelsNextFrame = true;
    }
    else if (localPos.getY() == Chunk::CHUNK_SIZE - 1) {
        Chunk* c = this->GetChunkAtChunkPosition(Vec2i(chunkPos.getX(), chunkPos.getY() + 1));
        if (c) c->updateVoxelsNextFrame = true;
    }
}

void ChunkMatrix::PlaceVoxelAt(const Vec2i &pos, Volume::VoxelType type)
{
    Vec2i chunkPos = WorldToChunkPosition(Vec2f(pos));
    std::shared_ptr<Volume::VoxelElement> voxel;
    switch (type)
    {
    case Volume::VoxelType::Fire:
        voxel = std::make_shared<Volume::FireVoxel>(pos);
        break;
    
    default:
        voxel = std::make_shared<Volume::VoxelMovableSolid>(type, pos);
        break;
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
    return pos.getX() >= 0 && pos.getX() &&
	    pos.getY() >= 0 && pos.getY();
}

bool ChunkMatrix::IsValidChunkPosition(const Vec2i &pos) const
{
    return pos.getX() >= 0 && pos.getX() &&
	    pos.getY() >= 0 && pos.getY();
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

    		if (j < radius * 0.4f) {
                PlaceVoxelAt(currentPos, Volume::VoxelType::Fire);
            }
            else {
                //destroy gas and immovable solids.. create particles for other
    			if (voxel->GetState() == VoxelState::Gas || voxel->GetState() == VoxelState::ImmovableSolid) 
                    PlaceVoxelAt(currentPos, Volume::VoxelType::Fire);
                else {
    				AddParticle(voxel->type, Vec2i(currentPos), static_cast<float>(angle), (radius*1.1f - j)*0.7f);
                    PlaceVoxelAt(currentPos, Volume::VoxelType::Fire);
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

void ChunkMatrix::AddParticle(Volume::VoxelType type, const Vec2i &position, float angle, float speed)
{
    this->newParticles.push_back(new VoxelParticle(type, position, angle, speed));
}

