#include "GameObject/Player.h"
#include "GameEngine.h"

#include "World/Particles/BulletParticle.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include "Player.h"

bool Game::Player::NoClip = true;

Game::Player::Player() = default;

Game::Player::Player(ChunkMatrix *matrix, std::vector<std::vector<Registry::VoxelData>> &voxelData)
    : PhysicsObject(
        Vec2f(100.0f, 0.0f),
        voxelData
    )
{
    this->Camera = AABB(
        Vec2f((800.0/Volume::Chunk::RENDER_VOXEL_SIZE)/2, (600.0/Volume::Chunk::RENDER_VOXEL_SIZE)/2), 
        Vec2f(800.0/Volume::Chunk::RENDER_VOXEL_SIZE, 600.0/Volume::Chunk::RENDER_VOXEL_SIZE));

    this->gunLaserParticleGenerator = new Particle::LaserParticleGenerator(matrix);
    this->gunLaserParticleGenerator->length = 50;
    this->gunLaserParticleGenerator->enabled = false;
}

Game::Player::~Player()
{
    delete this->gunLaserParticleGenerator;
    this->gunLaserParticleGenerator = nullptr;
    
    // Clean up the player object from the physics objects
    auto it = std::find(GameEngine::instance->physics->physicsObjects.begin(), GameEngine::instance->physics->physicsObjects.end(), this);
    if (it != GameEngine::instance->physics->physicsObjects.end()) {
        GameEngine::instance->physics->physicsObjects.erase(it);
    }
}

void Game::Player::UpdatePlayer(ChunkMatrix& chunkMatrix, float deltaTime)
{
    if(!b2Body_IsValid(m_physicsBody)) return;

    chunkMatrix.voxelMutex.lock();
    this->onGround = this->isOnGround(chunkMatrix);

    this->gunLaserParticleGenerator->enabled = this->gunEnabled;

    //TODO: change only when noclip changes and remove all velocity
    if(NoClip){
        b2Body_SetGravityScale(m_physicsBody, 0.0f);
        b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(0.0f, 0.0f));
    }
    else
        b2Body_SetGravityScale(m_physicsBody, 1.0f);

    bool keyHeld = false;
    if (GameEngine::MovementKeysHeld[0]) // W
    {
        keyHeld = true;
        if(NoClip){
            b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
            b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(velocity.x, -NOCLIP_SPEED));
        } else {
            if(this->onGround){
                b2Vec2 velocity = b2Vec2(0.0f, -JUMP_ACCELERATION);
                b2Body_ApplyLinearImpulseToCenter(m_physicsBody, velocity, true);
            }
        }
    }
    if (GameEngine::MovementKeysHeld[1]){ // S
        keyHeld = true;
        if(NoClip){
            b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
            b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(velocity.x, NOCLIP_SPEED));
        } else {
            b2Vec2 velocity = b2Vec2(0.0f, DOWN_MOVEMENT_ACCELERATION);
            b2Body_ApplyLinearImpulseToCenter(m_physicsBody, velocity, true);
        }
    }
    if (GameEngine::MovementKeysHeld[2]){ // A
        keyHeld = true;
        if(NoClip){
            b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
            b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(-NOCLIP_SPEED, velocity.y));
        }else{
            b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
            b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(-SPEED, velocity.y));
        }
    }
    if (GameEngine::MovementKeysHeld[3]){ // D
        keyHeld = true;
        if(NoClip){
            b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
            b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(NOCLIP_SPEED, velocity.y));
        }else{
            b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
            b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(SPEED, velocity.y));
        }
    }

    if(!GameEngine::MovementKeysHeld[2] && !GameEngine::MovementKeysHeld[3] && !NoClip){
        b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
        b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(velocity.x / 1.3f, velocity.y));
    }

    this->MoveCamera(Vec2f(this->position), chunkMatrix);

    //update player laser
    this->gunLaserParticleGenerator->position = Vec2f(this->position);
    Vec2f mousePos = chunkMatrix.MousePosToWorldPos(GameEngine::instance->mousePos, vector::ZERO) + Camera.corner;
    Vec2f direction = mousePos - (this->position);
    float angle = std::atan2(direction.y, direction.x);
    this->gunLaserParticleGenerator->angle = angle;

    chunkMatrix.voxelMutex.unlock();
}

bool Game::Player::Update(ChunkMatrix &chunkMatrix)
{
    return true;
}

void Game::Player::FireGun(ChunkMatrix &chunkMatrix)
{
    if(!this->gunEnabled) return;

    Vec2f mousePos = chunkMatrix.MousePosToWorldPos(GameEngine::instance->mousePos, this->Camera.corner * Volume::Chunk::RENDER_VOXEL_SIZE);
    
    Vec2f direction = mousePos - this->position;
    
    Particle::AddBulletParticle(
        &chunkMatrix,
        std::atan2(direction.y, direction.x),
        20.0f,
        3.0f,
        this->position
    );
}
bool Game::Player::ShouldRender() const
{
    return !NoClip;
}
Vec2f Game::Player::GetCameraPos() const
{
    return Vec2f(Camera.corner + Vec2f(Camera.size.x/2, Camera.size.y/2));
}

std::vector<Volume::VoxelElement*> Game::Player::GetVoxelsUnder(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int x = -PLAYER_WIDTH/2; x < PLAYER_WIDTH/2; ++x) {
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(x, PLAYER_HEIGHT/2 + 1), true);
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement *> Game::Player::GetVoxelsAbove(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int x = -PLAYER_WIDTH/2; x < PLAYER_WIDTH/2; ++x) {
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(x, -PLAYER_HEIGHT/2));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement*> Game::Player::GetVoxelsLeft(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int y = -PLAYER_HEIGHT/2; y < PLAYER_HEIGHT/2; ++y) {
        Vec2f localPos = Vec2i(this->position) + Vec2i(-PLAYER_WIDTH/2, y);
        auto voxel = chunkMatrix.VirtualGetAt(Vec2i(floor(localPos.x), floor(localPos.y)));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement*> Game::Player::GetVoxelsRight(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int y = -PLAYER_HEIGHT/2; y < PLAYER_HEIGHT/2; ++y) {
        Vec2f localPos = Vec2i(this->position) + Vec2i(PLAYER_WIDTH/2, y);
        auto voxel = chunkMatrix.VirtualGetAt(Vec2i(floor(localPos.x), floor(localPos.y)));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement *> Game::Player::GetVoxelsAtWaist(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int x = -PLAYER_WIDTH/2; x < PLAYER_WIDTH/2; ++x) {
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(x, 0));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement*> Game::Player::GetVoxelsVerticalSlice(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int y = -PLAYER_HEIGHT/2; y < PLAYER_HEIGHT/2; ++y) {
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(0, y));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

bool Game::Player::isOnGround(ChunkMatrix &chunkMatrix)
{
    if(this->NoClip) return false;

    std::vector<Volume::VoxelElement*> voxels = this->GetVoxelsUnder(chunkMatrix);
    for (const auto& voxel : voxels) {
        if(voxel->GetState() == Volume::State::Solid){
            return true;
            break;
        }
    }
    return false;
}

int Game::Player::touchLeftWall(ChunkMatrix &chunkMatrix)
{
    if(this->NoClip) return 0;

    std::vector<Volume::VoxelElement*> voxels = this->GetVoxelsLeft(chunkMatrix);
    int highest = 0;
    for(size_t i = 0; i < voxels.size(); ++i){
        if(voxels[i]->GetState() == Volume::State::Solid){
            highest = voxels.size() - i;
            break;
        }
    }

    return highest;
}

int Game::Player::touchRightWall(ChunkMatrix &chunkMatrix)
{
    if(this->NoClip) return 0;

    std::vector<Volume::VoxelElement*> voxels = this->GetVoxelsRight(chunkMatrix);
    int highest = 0;
    for(size_t i = 0; i < voxels.size(); ++i){
        if(voxels[i]->GetState() == Volume::State::Solid){
            highest = voxels.size() - i;
            break;
        }
    }
    return highest;
}

void Game::Player::MoveCamera(Vec2f pos, ChunkMatrix &chunkMatrix)
{
    using namespace Volume;

    Camera.corner = (pos-Vec2f(Camera.size.x/2, Camera.size.y/2));

    Vec2f cameraMin = Camera.corner;
    Vec2f cameraMax = Camera.corner + Camera.size;

    float halfChunk = Chunk::CHUNK_SIZE / 2.0f;
    Vec2f adjustedMin = cameraMin - Vec2f(halfChunk, halfChunk);
    Vec2f adjustedMax = cameraMax + Vec2f(halfChunk, halfChunk);

    Vec2i chunkMin = chunkMatrix.WorldToChunkPosition(adjustedMin);
    Vec2i chunkMax = chunkMatrix.WorldToChunkPosition(adjustedMax);

    std::vector<Vec2i> chunksToLoad;
    for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
        for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
            if(!chunkMatrix.IsValidChunkPosition(Vec2i(x, y))) continue;    // Skip invalid chunk positions
            if(chunkMatrix.GetChunkAtChunkPosition(Vec2i(x, y))) continue;  // If the chunk already exists, skip it
            chunksToLoad.push_back(Vec2i(x, y));
        }
    }

    for (const auto& chunkPos : chunksToLoad) {
        GameEngine::instance->LoadChunkInView(chunkPos);
    }
}

void Game::Player::MoveCameraTowards(Vec2f to, ChunkMatrix &chunkMatrix)
{
    Vec2f from = GetCameraPos();

    Vec2f pos = Vec2f(std::lerp(from.x, to.x, 0.1), std::lerp(from.y, to.y, 0.1));


    this->MoveCamera(pos, chunkMatrix);
}
