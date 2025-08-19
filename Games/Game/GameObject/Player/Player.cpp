#include "Player.h"
#include "GameEngine.h"
#include "World/Particles/BulletParticle.h"

#include <iostream>
#include <cmath>
#include <algorithm>

Player::Player(ChunkMatrix *matrix, std::vector<std::vector<Registry::VoxelData>> &voxelData, float densityOverride)
    : PhysicsObject(
        Vec2f(100.0f, 0.0f),
        voxelData,
        densityOverride,
        "Player"
    )
{
    this->Camera = AABB(
        Vec2f((800.0/Volume::Chunk::RENDER_VOXEL_SIZE)/2, (600.0/Volume::Chunk::RENDER_VOXEL_SIZE)/2), 
        Vec2f(800.0/Volume::Chunk::RENDER_VOXEL_SIZE, 600.0/Volume::Chunk::RENDER_VOXEL_SIZE));

    this->gunLaserParticleGenerator = new Particle::LaserParticleGenerator(matrix);
    this->gunLaserParticleGenerator->length = 50;
    this->gunLaserParticleGenerator->enabled = false;
}

Player::~Player()
{
    delete this->gunLaserParticleGenerator;
    this->gunLaserParticleGenerator = nullptr;
    
    // Clean up the player object from the physics objects
    auto it = std::find(GameEngine::physics->physicsObjects.begin(), GameEngine::physics->physicsObjects.end(), this);
    if (it != GameEngine::physics->physicsObjects.end()) {
        GameEngine::physics->physicsObjects.erase(it);
    }
}

void Player::UpdatePlayer(ChunkMatrix& chunkMatrix, float deltaTime)
{
    if(!b2Body_IsValid(m_physicsBody)) return;

    chunkMatrix.voxelMutex.lock();

    this->onGround = this->isOnGround(chunkMatrix);

    this->gunLaserParticleGenerator->enabled = this->gunEnabled;

    if (GameEngine::MovementKeysHeld[0]) // W
    {
        float verticalVelocity = b2Body_GetLinearVelocity(m_physicsBody).y;

        if(this->noClip){
            this->position.y -= NOCLIP_SPEED * deltaTime;
        } else if(this->onGround && verticalVelocity <= 0) {      // Jumping
            // reset vertical velocity
            b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
            b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(velocity.x, 0.0f));

            velocity = b2Vec2(0.0f, -JUMP_ACCELERATION);
            b2Body_ApplyLinearImpulseToCenter(m_physicsBody, velocity, true);
        }
    }
    if (GameEngine::MovementKeysHeld[1]){ // S
        if(this->noClip){
            this->position.y += NOCLIP_SPEED * deltaTime;
        } else {
            b2Vec2 velocity = b2Vec2(0.0f, DOWN_MOVEMENT_ACCELERATION);
            b2Body_ApplyLinearImpulseToCenter(m_physicsBody, velocity, true);
        }
    }
    if (GameEngine::MovementKeysHeld[2]){ // A
        if(this->noClip){
            this->position.x -= NOCLIP_SPEED * deltaTime;
        }else{
            int voxelHeightLeft = touchLeftWall(chunkMatrix);

            if(voxelHeightLeft > 0 && voxelHeightLeft <= STEP_HEIGHT) {
                this->position.y -= voxelHeightLeft + 1;
                b2Body_SetTransform(m_physicsBody, b2Vec2(this->position.x, this->position.y), b2Rot{1.0f, 0.0f});
            }

            b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
            b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(-SPEED, velocity.y));
        }
    }
    if (GameEngine::MovementKeysHeld[3]){ // D
        if(this->noClip){
            this->position.x += NOCLIP_SPEED * deltaTime;
        }else{
            int voxelHeightRight = touchRightWall(chunkMatrix);

            if(voxelHeightRight > 0 && voxelHeightRight <= STEP_HEIGHT) {
                this->position.y -= voxelHeightRight + 1;
                b2Body_SetTransform(m_physicsBody, b2Vec2(this->position.x, this->position.y), b2Rot{1.0f, 0.0f});
            }

            b2Vec2 velocity = b2Body_GetLinearVelocity(m_physicsBody);
            b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(SPEED, velocity.y));
        }
    }

    // Apply friction when not moving horizontally and on ground
    if(!GameEngine::MovementKeysHeld[2] && !GameEngine::MovementKeysHeld[3] && this->onGround && !this->noClip){
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

bool Player::Update(ChunkMatrix &chunkMatrix)
{
    return true;
}

void Player::UpdateColliders(std::vector<Triangle> &triangles, std::vector<b2Vec2> &edges, b2WorldId worldId)
{
    PhysicsObject::UpdateColliders(triangles, edges, worldId);

    // enable/disable based on noClip
    if(this->noClip)
        b2Body_Disable(m_physicsBody);
    else 
        b2Body_Enable(m_physicsBody);
}

void Player::FireGun(ChunkMatrix &chunkMatrix)
{
    if(!this->gunEnabled) return;

    Vec2f mousePos = chunkMatrix.MousePosToWorldPos(GameEngine::instance->mousePos, this->Camera.corner);
    
    Vec2f direction = mousePos - this->position;
    
    Particle::AddBulletParticle(
        &chunkMatrix,
        std::atan2(direction.y, direction.x),
        20.0f,
        3.0f,
        this->position
    );
}
bool Player::ShouldRender() const
{
    return !noClip;
}
void Player::SetNoClip(bool value)
{
    if(this->noClip == value) return;

    this->noClip = value;

    if(this->noClip){
        b2Body_SetGravityScale(m_physicsBody, 0);
        b2Body_SetLinearVelocity(m_physicsBody, b2Vec2(0, 0));
        b2Body_Disable(m_physicsBody);
    } else {
        b2Body_Enable(m_physicsBody);
        b2Body_SetGravityScale(m_physicsBody, 1);

        b2Body_SetTransform(m_physicsBody, b2Vec2(this->position.x, this->position.y), b2Rot{1.0f, 0.0f});
    }
}
Vec2f Player::GetCameraPos() const
{
    return Vec2f(Camera.corner + Vec2f(Camera.size.x/2, Camera.size.y/2));
}

std::vector<Volume::VoxelElement*> Player::GetVoxelsUnder(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int x = -PLAYER_WIDTH/2+1; x < PLAYER_WIDTH/2-1; ++x) {
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(x, PLAYER_HEIGHT/2 + 1), true);
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement *> Player::GetVoxelsAbove(ChunkMatrix &chunkMatrix)
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

std::vector<Volume::VoxelElement*> Player::GetVoxelsLeft(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int y = -PLAYER_HEIGHT/2; y <= PLAYER_HEIGHT/2; ++y) {
        Vec2f localPos = Vec2i(this->position) + Vec2i(-PLAYER_WIDTH/2 + 1, y); // +1 because the feet sprite isnt at the exact edge
        auto voxel = chunkMatrix.VirtualGetAt(Vec2i(floor(localPos.x), floor(localPos.y)));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement*> Player::GetVoxelsRight(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int y = -PLAYER_HEIGHT/2; y <= PLAYER_HEIGHT/2; ++y) {
        Vec2f localPos = Vec2i(this->position) + Vec2i(PLAYER_WIDTH/2 - 1, y); // -1 because the feet sprite isnt at the exact edge
        auto voxel = chunkMatrix.VirtualGetAt(Vec2i(floor(localPos.x), floor(localPos.y)));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement *> Player::GetVoxelsAtWaist(ChunkMatrix &chunkMatrix)
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

std::vector<Volume::VoxelElement*> Player::GetVoxelsVerticalSlice(ChunkMatrix &chunkMatrix)
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

bool Player::isOnGround(ChunkMatrix &chunkMatrix)
{
    if(this->noClip) return false;

    std::vector<Volume::VoxelElement*> voxels = this->GetVoxelsUnder(chunkMatrix);
    for (const auto& voxel : voxels) {
        if(voxel->GetState() == Volume::State::Solid){
            return true;
            break;
        }
    }
    return false;
}

int Player::touchLeftWall(ChunkMatrix &chunkMatrix)
{
    if(this->noClip) return 0;

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

int Player::touchRightWall(ChunkMatrix &chunkMatrix)
{
    if(this->noClip) return 0;

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

void Player::MoveCamera(Vec2f pos, ChunkMatrix &chunkMatrix)
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

void Player::MoveCameraTowards(Vec2f to, ChunkMatrix &chunkMatrix)
{
    Vec2f from = GetCameraPos();

    Vec2f pos = Vec2f(std::lerp(from.x, to.x, 0.1), std::lerp(from.y, to.y, 0.1));


    this->MoveCamera(pos, chunkMatrix);
}