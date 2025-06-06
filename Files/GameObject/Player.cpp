#include "GameObject/Player.h"
#include "GameEngine.h"

#include "World/Particles/BulletParticle.h"

#include <future>
#include <iostream>
#include <cmath>
#include "Player.h"

bool Game::Player::NoClip = true;

Game::Player::Player()
    : GameObject()
{
    
}

Game::Player::Player(ChunkMatrix *matrix, SDL_Texture* texture)
    : GameObject(texture, Vec2f(100.0f, 0.0f))
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

}

void Game::Player::Update(ChunkMatrix& chunkMatrix, float deltaTime)
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
        this->acceleration /= 1.025f;
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
            this->acceleration += (1+(LiquidPercentile*0.005f)) * GRAVITY * deltaTime;
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

    this->MoveCameraTowards(Vec2f(this->position) + Vec2f(PLAYER_WIDTH/2, PLAYER_HEIGHT/2), chunkMatrix);

    // Move the player downwards
    if(acceleration != 0) MovePlayerBy(Vec2f(0, this->acceleration), chunkMatrix);

    //update player laser
    this->gunLaserParticleGenerator->position = Vec2f(this->position) + Vec2f(PLAYER_WIDTH/2, PLAYER_HEIGHT/2);
    Vec2f mousePos = chunkMatrix.MousePosToWorldPos(GameEngine::instance->mousePos, vector::ZERO) + Camera.corner;
    Vec2f direction = mousePos - (this->position + Vec2f(PLAYER_WIDTH/2, PLAYER_HEIGHT/2));
    float angle = std::atan2(direction.getY(), direction.getX());
    this->gunLaserParticleGenerator->angle = angle;

    chunkMatrix.voxelMutex.unlock();
}

void Game::Player::Render(SDL_Renderer* renderer, const Vec2f &offset)
{
    if(this->NoClip) return;
    GameObject::Render(renderer, offset);   
}

void Game::Player::FireGun(ChunkMatrix &chunkMatrix)
{
    if(!this->gunEnabled) return;

    Vec2f mousePos = chunkMatrix.MousePosToWorldPos(GameEngine::instance->mousePos, this->Camera.corner * Volume::Chunk::RENDER_VOXEL_SIZE);
    Vec2f playerPos = Vec2f(this->position) + Vec2f(PLAYER_WIDTH/2, PLAYER_HEIGHT/2);
    
    Vec2f direction = mousePos - playerPos;
    
    Particle::AddBulletParticle(
        &chunkMatrix,
        std::atan2(direction.getY(), direction.getX()),
        20.0f,
        3.0f,
        playerPos
    );
}
Vec2f Game::Player::GetCameraPos() const
{
    return Vec2f(Camera.corner + Vec2f(Camera.size.getX()/2, Camera.size.getY()/2));
}

std::vector<Volume::VoxelElement*> Game::Player::GetVoxelsUnder(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int x = 1; x < PLAYER_WIDTH; ++x) {
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(x, PLAYER_HEIGHT));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement *> Game::Player::GetVoxelsAbove(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int x = 1; x < PLAYER_WIDTH; ++x) {
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(x, -PLAYER_HEIGHT));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement*> Game::Player::GetVoxelsLeft(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int y = 1; y < PLAYER_HEIGHT; ++y) {
        Vec2f localPos = Vec2i(this->position) + Vec2i(0, y);
        auto voxel = chunkMatrix.VirtualGetAt(Vec2i(floor(localPos.getX()), floor(localPos.getY())));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement*> Game::Player::GetVoxelsRight(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int y = 1; y < PLAYER_HEIGHT; ++y) {
        Vec2f localPos = Vec2i(this->position) + Vec2i(PLAYER_WIDTH, y);
        auto voxel = chunkMatrix.VirtualGetAt(Vec2i(floor(localPos.getX()), floor(localPos.getY())));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement *> Game::Player::GetVoxelsAtWaist(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int x = 1; x < PLAYER_WIDTH; ++x) {
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(x,  WAIST_HEIGHT/2));
        if (voxel) {
            voxels.push_back(voxel);
        }
    }
    return voxels;
}

std::vector<Volume::VoxelElement*> Game::Player::GetVoxelsVerticalSlice(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::VoxelElement*> voxels;
    for (int y = 0; y < PLAYER_HEIGHT; ++y) {
        auto voxel = chunkMatrix.VirtualGetAt(Vec2f(this->position) + Vec2f(PLAYER_WIDTH/2, y));
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
        if(chunkMatrix.VirtualGetAt(Vec2i(floor(pos.getX()), floor(pos.getY())))->GetState() == Volume::State::Solid){
            return;
        }
    }

    this->position = pos;
    //this->MoveCamera(Vec2f(this->position), chunkMatrix);
}

void Game::Player::MovePlayerBy(Vec2f pos, ChunkMatrix &chunkMatrix)
{
    // Move the player by X
    for(int i = 0; i < floor(abs(pos.getX())); ++i){
        int leftWallLevel = this->touchLeftWall(chunkMatrix);
        if (leftWallLevel > STEP_HEIGHT && pos.getX() < 0) {
            break;
        }
        int rightWallLevel = this->touchRightWall(chunkMatrix);
        if (rightWallLevel > STEP_HEIGHT && pos.getX() > 0) {
            break;
        }
        this->position.x(this->position.getX() + (pos.getX() > 0 ? 1 : -1));
        if(pos.getX() < 0)this->position.y(this->position.getY() - leftWallLevel);
        if(pos.getX() > 0)this->position.y(this->position.getY() - rightWallLevel);
    }
    int leftWallLevel = this->touchLeftWall(chunkMatrix);
    int rightWallLevel = this->touchRightWall(chunkMatrix);
    if((leftWallLevel < STEP_HEIGHT && pos.getX() < 0) || 
        (rightWallLevel < STEP_HEIGHT && pos.getX() > 0)){

        this->position.x(this->position.getX() + std::fmod(pos.getX(), 1.0f)); // Move the remaining part
        if(pos.getX() < 0)this->position.y(this->position.getY() - leftWallLevel);
        if(pos.getX() > 0)this->position.y(this->position.getY() - rightWallLevel);
    }

    // Move to the ground by one step
    int move = pos.getY() > 0 ? 1 : -1;
    for (int i = 0; i < floor(abs(pos.getY())); ++i) {
        if ((this->isOnGround(chunkMatrix) && move > 0) || (this->isOnCeiling(chunkMatrix) && move < 0)) {
            break;
        }
        this->position.y(this->position.getY() + move);
    }

    if((!this->isOnGround(chunkMatrix) || pos.getY() < 0) && (!this->isOnCeiling(chunkMatrix) || pos.getY() > 0)){
        this->position.y(this->position.getY() + std::fmod(pos.getY(), 1.0f)); // Move the remaining part
    }

    if(this->isOnGround(chunkMatrix)){
        this->position.y(floor(this->position.getY())); //Snap to ground
    }
}

void Game::Player::MoveCamera(Vec2f pos, ChunkMatrix &chunkMatrix)
{
    using namespace Volume;

    Camera.corner = (pos-Vec2f(Camera.size.getX()/2, Camera.size.getY()/2));

    //Spawn chunks that are in the view but don´t exits
    Vec2i cameraChunkPos = chunkMatrix.WorldToChunkPosition(Camera.corner - Vec2f(Game::CAMERA_CHUNK_PADDING/2, Game::CAMERA_CHUNK_PADDING/2));

    int ChunksHorizontal = ceil((Camera.size.getX() + Game::CAMERA_CHUNK_PADDING*2) 
                                    / Chunk::CHUNK_SIZE)+1;
    
    int ChunksVertical =   ceil((Camera.size.getY() + Game::CAMERA_CHUNK_PADDING*2) 
                                    / Chunk::CHUNK_SIZE)+1;

    std::vector<Vec2i> chunksToLoad;
    for (int x = 0; x < ChunksHorizontal; ++x) {
        for (int y = 0; y < ChunksVertical; ++y) {
            Vec2i chunkPos = cameraChunkPos + Vec2i(x, y);
            Chunk* chunk = chunkMatrix.GetChunkAtChunkPosition(chunkPos);
            if (!chunk) {
                chunksToLoad.push_back(chunkPos);
            }else{
                chunk->lastCheckedCountDown = 20;
            }
        }
    }

    std::vector<std::future<void>> futures;
    for (const auto& chunkPos : chunksToLoad) {
        futures.emplace_back(std::async(std::launch::async, &GameEngine::LoadChunkInView, GameEngine::instance, chunkPos));
    }

    // Wait for all chunks to finish loading
    for (auto& future : futures) {
        future.get();
    }
}

void Game::Player::MoveCameraTowards(Vec2f to, ChunkMatrix &chunkMatrix)
{
    Vec2f from = GetCameraPos();

    Vec2f pos = Vec2f(std::lerp(from.getX(), to.getX(), 0.1), std::lerp(from.getY(), to.getY(), 0.1));


    this->MoveCamera(pos, chunkMatrix);
}
