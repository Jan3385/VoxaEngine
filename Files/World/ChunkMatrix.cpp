#include "World/ChunkMatrix.h"
#include "GameEngine.h"
#include "World/Particles/SolidFallingParticle.h"
#include "ChunkMatrix.h"

using namespace Volume;

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
void ChunkMatrix::RenderObjects(SDL_Renderer &renderer, Vec2f offset) const
{
    SDL_SetRenderDrawBlendMode(&renderer, SDL_BLENDMODE_BLEND);
    for (const auto& gameObject : gameObjects) {
        gameObject->Render(&renderer, offset);
    }
}
ChunkMatrix::ChunkMatrix()
{
    this->particleGenerators.reserve(15);
    this->particles.reserve(200);
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

Volume::VoxelElement* ChunkMatrix::VirtualGetAt(const Vec2i &pos)
{
    Vec2i chunkPos = WorldToChunkPosition(Vec2f(pos));

    if (!IsValidWorldPosition(pos)) return nullptr;

    Chunk *chunk = GetChunkAtChunkPosition(chunkPos);
    if(!chunk){
        chunk = GenerateChunk(chunkPos);
        GameEngine::instance->renderer->chunkCreateBuffer.push_back(chunk);
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
        GameEngine::instance->renderer->chunkCreateBuffer.push_back(chunk);
    }

    //delete the old voxel if it exists
    if(chunk->voxels[localPos.getX()][localPos.getY()]){
        delete chunk->voxels[localPos.getX()][localPos.getY()];
    }

    // Set the new voxel and mark for update
    chunk->voxels[localPos.getX()][localPos.getY()] = voxel;

    chunk->dirtyRect.Include(localPos);
    chunk->UpdateRenderBufferRanges[localPos.getX()].AddValue(localPos.getY());

    chunk->forceHeatUpdate = true;
    chunk->forcePressureUpdate = true;
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
        GameEngine::instance->renderer->chunkCreateBuffer.push_back(chunk);
    }

    // Set the new voxel and mark for update
    chunk->voxels[localPos.getX()][localPos.getY()] = voxel;

    chunk->dirtyRect.Include(localPos);
    chunk->UpdateRenderBufferRanges[localPos.getX()].AddValue(localPos.getY());

    chunk->forceHeatUpdate = true;
    chunk->forcePressureUpdate = true;
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

        if(!replacedVoxel){
            // if no voxel to replace, a problem must have happened. Stop
            delete voxel;
            return;
        }

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
        if(generator->enabled)
            generator->TickParticles();
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