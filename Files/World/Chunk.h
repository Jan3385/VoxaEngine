#pragma once

#include <SDL2/SDL.h>
#include <array>
#include <mutex>
#include <vector>
#include <SDL2/SDL_ttf.h>
#include "Voxel.h"
#include "../Math/Vector.h"
#include "../Math/AABB.h"

namespace Volume
{
    class Chunk {
    public:
    	static const unsigned short int RENDER_VOXEL_SIZE = 5;
    	static const unsigned short int CHUNK_SIZE = 64;
    	//VoxelElement*** voxels;
    	std::array<std::array<std::shared_ptr<VoxelElement>, CHUNK_SIZE>, CHUNK_SIZE> voxels;

    	Chunk(const Vec2i& pos);
    	~Chunk();

		bool ShouldChunkDelete(AABB &Camera);
    	void UpdateVoxels(ChunkMatrix* matrix);
    	void ResetVoxelUpdateData(ChunkMatrix* matrix);
    	SDL_Surface* Render() const;
		Vec2i GetPos() const;
		AABB GetAABB() const;

		uint8_t lastCheckedCountDown = 20;
    	bool updateVoxelsNextFrame = true;
    private:
    	short int m_x;
    	short int m_y;
		TTF_Font* font;
    };
}

class ChunkMatrix {
public:
	ChunkMatrix();
	~ChunkMatrix();


	//grid storage logic
	//std::vector<std::vector<Volume::Chunk*>> Grid;
	//precomputed grids for simulation passing - 0 - 3 passees
	std::vector<Volume::Chunk*> GridSegmented[4];

	std::vector<Volume::VoxelParticle*> newParticles;
	std::vector<Volume::VoxelParticle*> particles;

	Volume::Chunk* GetChunkAtWorldPosition(const Vec2f& pos);
	Volume::Chunk* GetChunkAtChunkPosition(const Vec2i& pos);

	void PlaceVoxelsAtMousePosition(const Vec2f& pos, Volume::VoxelType elementType, Vec2f offset);
	void PlaceParticleAtMousePosition(const Vec2f& pos, Volume::VoxelType particleType, Vec2f offset, float angle, float speed);
	void RemoveVoxelAtMousePosition(const Vec2f& pos, Vec2f offset);
	void ExplodeAtMousePosition(const Vec2f& pos, short int radius, Vec2f offset);

	Volume::Chunk* GenerateChunk(const Vec2i& pos);
	void DeleteChunk(const Vec2i& pos);

	//Virtual setter / getter
	//Accesses a virtual 2D array that ignores chunks
	std::shared_ptr<Volume::VoxelElement> VirtualGetAt(const Vec2i& pos);
	void PlaceVoxelAt(const Vec2i& pos, Volume::VoxelType type);
	void VirtualSetAt(std::shared_ptr<Volume::VoxelElement> voxel);

	void GetVoxelsInChunkAtWorldPosition(const Vec2f& pos);
	void GetVoxelsInCubeAtWorldPosition(const Vec2f& start, const Vec2f& end);


	bool IsValidWorldPosition(const Vec2i& pos) const;
	bool IsValidChunkPosition(const Vec2i& pos) const;

	void ExplodeAt(const Vec2i& pos, short int radius);

	//particle functions
	void UpdateParticles();
	void AddParticle(Volume::VoxelType type, const Vec2i& position, float angle, float speed);
	void RenderParticles(SDL_Renderer& renderer, Vec2f offset) const;

	//Static functions
	static Vec2i WorldToChunkPosition(const Vec2f& pos);
	static Vec2f ChunkToWorldPosition(const Vec2i& pos);
	static Vec2f MousePosToWorldPos(const Vec2f& mousePos, Vec2f offset);
};
