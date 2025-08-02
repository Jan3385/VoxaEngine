#include "GameObject/Player.h"
#include "GameEngine.h"

#include "World/Particles/BulletParticle.h"

#include <future>
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
    chunkMatrix.voxelMutex.lock();
    this->onGround = this->isOnGround(chunkMatrix);

    this->gunLaserParticleGenerator->enabled = this->gunEnabled;

    // Apply gravity
    if(!this->NoClip){
        if (this->onGround) {
            this->acceleration = 0;
        } else if(this->isOnCeiling(chunkMatrix)){
            this->acceleration = GRAVITY * deltaTime;
        }else {
            this->acceleration += GRAVITY * deltaTime;
        }
    }else{
        this->acceleration = 0;
    }
    
    std::vector<Volume::VoxelElement*> verticalVoxels = this->GetVoxelsVerticalSlice(chunkMatrix);
    int LiquidPercentile = Volume::GetLiquidVoxelPercentile(verticalVoxels);
    if(this->NoClip) LiquidPercentile = 0;

    bool isInWater = LiquidPercentile > 20; // 20% of the voxels are liquid

    if(isInWater){
        this->acceleration -= (1+(LiquidPercentile*0.01f)) * GRAVITY * deltaTime; // 1 - 2 times gravity
        this->acceleration /= (1.001f * (1 + deltaTime));
    }

    if (GameEngine::MovementKeysHeld[0]) // W
    {
        if(isInWater){
            this->MovePlayerBy(Vec2f(0, -30 * deltaTime), chunkMatrix);
        }

        if (this->onGround) {
            this->acceleration = -JUMP_ACCELERATION;
        }else if(this->NoClip){
            this->MovePlayerBy(Vec2f(0, -100 * deltaTime), chunkMatrix);
        }
    }
    if (GameEngine::MovementKeysHeld[1]){ // S
        if(isInWater){
            this->acceleration += (1+(LiquidPercentile*0.01f)) * GRAVITY * deltaTime;
        }

        if(this->NoClip){
            this->MovePlayerBy(Vec2f(0, 100 * deltaTime), chunkMatrix);
        }
    }
    if (GameEngine::MovementKeysHeld[2]){ // A
        if(this->NoClip)
            this->MovePlayerBy(Vec2f(-100 * deltaTime,0), chunkMatrix);
        else
            this->MovePlayerBy(Vec2f(-Player::SPEED * deltaTime, 0), chunkMatrix);
    }
    if (GameEngine::MovementKeysHeld[3]){ // D
        if(this->NoClip)
            this->MovePlayerBy(Vec2f(100 * deltaTime,0), chunkMatrix);
        else
            this->MovePlayerBy(Vec2f(Player::SPEED * deltaTime, 0), chunkMatrix);
    }

    this->MoveCameraTowards(Vec2f(this->position), chunkMatrix);

    // Move the player downwards
    if(acceleration != 0) MovePlayerBy(Vec2f(0, this->acceleration * deltaTime), chunkMatrix);

    //update player laser
    this->gunLaserParticleGenerator->position = Vec2f(this->position);
    Vec2f mousePos = chunkMatrix.MousePosToWorldPos(GameEngine::instance->mousePos, vector::ZERO) + Camera.corner;
    Vec2f direction = mousePos - (this->position);
    float angle = std::atan2(direction.y, direction.x);
    this->gunLaserParticleGenerator->angle = angle;

    chunkMatrix.voxelMutex.unlock();
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
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(x, PLAYER_HEIGHT/2 + 1));
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

bool Game::Player::isOnCeiling(ChunkMatrix &chunkMatrix)
{
    if(this->NoClip) return false;

    std::vector<Volume::VoxelElement*> voxels = this->GetVoxelsAbove(chunkMatrix);
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

void Game::Player::MovePlayer(Vec2f pos, ChunkMatrix &chunkMatrix)
{
    //check if the player isnt being teleported into a solid voxel
    if(!this->NoClip){
        if(chunkMatrix.VirtualGetAt(Vec2i(floor(pos.x), floor(pos.y)))->GetState() == Volume::State::Solid){
            return;
        }
    }

    this->position = pos;
    //this->MoveCamera(Vec2f(this->position), chunkMatrix);
}

void Game::Player::MovePlayerBy(Vec2f pos, ChunkMatrix &chunkMatrix)
{
    // Move the player by X
    for(int i = 0; i < floor(abs(pos.x)); ++i){
        int leftWallLevel = this->touchLeftWall(chunkMatrix);
        if (leftWallLevel > STEP_HEIGHT && pos.x < 0) {
            break;
        }
        int rightWallLevel = this->touchRightWall(chunkMatrix);
        if (rightWallLevel > STEP_HEIGHT && pos.x > 0) {
            break;
        }
        this->position.x += pos.x > 0 ? 1 : -1;
        if(pos.x < 0) this->position.y -= leftWallLevel;
        if(pos.x > 0) this->position.y -= rightWallLevel;
    }
    int leftWallLevel = this->touchLeftWall(chunkMatrix);
    int rightWallLevel = this->touchRightWall(chunkMatrix);
    if((leftWallLevel < STEP_HEIGHT && pos.x < 0) || 
        (rightWallLevel < STEP_HEIGHT && pos.x > 0)){

        this->position.x += std::fmod(pos.x, 1.0f); // Move the remaining part
        if(pos.x < 0)this->position.y -= leftWallLevel;
        if(pos.x > 0)this->position.y -= rightWallLevel;
    }

    // Move to the ground by one step
    int move = pos.y > 0 ? 1 : -1;
    for (int i = 0; i < floor(abs(pos.y)); ++i) {
        if ((this->isOnGround(chunkMatrix) && move > 0) || (this->isOnCeiling(chunkMatrix) && move < 0)) {
            break;
        }
        this->position.y += move;
    }

    if((!this->isOnGround(chunkMatrix) || pos.y < 0) && (!this->isOnCeiling(chunkMatrix) || pos.y > 0)){
        this->position.y += std::fmod(pos.y, 1.0f); // Move the remaining part
    }

    if(this->isOnGround(chunkMatrix)){
        this->position.y = floor(this->position.y); //Snap to ground
    }
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
