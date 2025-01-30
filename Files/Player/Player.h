#pragma once

#include "../Math/Vector.h"
#include "../Math/AABB.h"
#include "../World/Chunk.h"

namespace Game{
    constexpr int CAMERA_CHUNK_BUFFER = 10;
    class Player{
    public:
        static bool NoClip;
        static constexpr float GRAVITY = 9.81f * Volume::VOXEL_SIZE_METERS * 4;
        Player();
        void LoadPlayerTexture(SDL_Renderer* renderer);
        ~Player();
        void Update(ChunkMatrix& chunkMatrix, float deltaTime);
        void Render(SDL_Renderer* renderer);

        Vec2f GetCameraPos() const;
        bool onGround = false;

        std::vector<Volume::VoxelElement*> GetVoxelsUnder(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsAbove(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsLeft(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsRight(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsAtWaist(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsVerticalSlice(ChunkMatrix& chunkMatrix);

        AABB Camera;
    private:
        static constexpr int PLAYER_WIDTH = 8;
        static constexpr int PLAYER_HEIGHT = PLAYER_WIDTH * 2;
        static constexpr int WAIST_HEIGHT = PLAYER_HEIGHT / 2;
        static constexpr int STEP_HEIGHT = 2;
        static constexpr float JUMP_ACCELERATION = 1.4;

        float acceleration = 0;

        bool isOnGround(ChunkMatrix& chunkMatrix);
        bool isOnCeiling(ChunkMatrix& chunkMatrix);
        /**
         * @brief Gets the highest level voxel on the left side of the player
         * \returns 0 if not touching at all, N if touching
         */
        int touchLeftWall(ChunkMatrix& chunkMatrix);
        /**
         * @brief Gets the highest level voxel on the left side of the player
         * \returns 0 if not touching at all, N if touching
         */
        int touchRightWall(ChunkMatrix& chunkMatrix);

        void MovePlayer(Vec2f pos, ChunkMatrix& chunkMatrix);
        void MovePlayerBy(Vec2f pos, ChunkMatrix& chunkMatrix);
        void MoveCamera(Vec2f pos, ChunkMatrix& chunkMatrix);
        void MoveCameraTowards(Vec2f to, ChunkMatrix& chunkMatrix);
        Vec2f position;
        SDL_Texture* playerTexture;
    };
}