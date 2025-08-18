#pragma once

#include "GameObject/PhysicsObject.h"

#include "Math/Vector.h"
#include "Math/AABB.h"
#include "World/ChunkMatrix.h"
#include "World/ParticleGenerators/LaserParticleGenerator.h"

namespace Game{
    constexpr int CAMERA_CHUNK_PADDING = 12;
    class Player  : public PhysicsObject {
    public:
        static constexpr float DOWN_MOVEMENT_ACCELERATION = 9.81f * 30;
        static constexpr int SPEED = 10;
        static constexpr int NOCLIP_SPEED = 200;
        Player();
        Player(ChunkMatrix *matrix, std::vector<std::vector<Registry::VoxelData>> &voxelData, float densityOverride);
        ~Player();

        // Disable copy and move semantics
        Player(const Player&) = delete;
        Player(Player&&) = delete;
        Player& operator=(const Player&) = delete;
        Player& operator=(Player&&) = delete;

        void UpdatePlayer(ChunkMatrix& chunkMatrix, float deltaTime);
        bool Update(ChunkMatrix& chunkMatrix) override;

        void UpdateColliders(std::vector<Triangle> &triangles, std::vector<b2Vec2> &edges, b2WorldId worldId) override;

        bool CanBreakIntoParts() const override { return false; }

        void FireGun(ChunkMatrix& chunkMatrix);

        bool ShouldRender() const override;

        bool GetNoClip() const { return noClip; }
        void SetNoClip(bool value);
        Vec2f GetCameraPos() const;
        bool onGround = false;
        bool gunEnabled = false;

        std::vector<Volume::VoxelElement*> GetVoxelsUnder(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsAbove(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsLeft(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsRight(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsAtWaist(ChunkMatrix& chunkMatrix);
        std::vector<Volume::VoxelElement*> GetVoxelsVerticalSlice(ChunkMatrix& chunkMatrix);

        AABB Camera;
    protected:
        bool IsAbleToRotate() const override { return false; }
        bool noClip = true;
    private:
        static constexpr int PLAYER_WIDTH = 8;
        static constexpr int PLAYER_HEIGHT = PLAYER_WIDTH * 2;
        static constexpr int WAIST_HEIGHT = PLAYER_HEIGHT / 2;
        static constexpr int STEP_HEIGHT = 3;
        static constexpr float JUMP_ACCELERATION = 1300000.0f;

        Particle::LaserParticleGenerator *gunLaserParticleGenerator;

        float acceleration = 0;

        bool isOnGround(ChunkMatrix& chunkMatrix);
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

        void MoveCamera(Vec2f pos, ChunkMatrix& chunkMatrix);
        void MoveCameraTowards(Vec2f to, ChunkMatrix& chunkMatrix);
    };
}