#pragma once

#include <SDL2/SDL.h>
#include <array>
#include <mutex>
#include <vector>
#include "Voxel.h"
#include "../Math/Vector.h"

namespace Volume
{
    class Chunk {
    public:
    	static const short int RENDER_VOXEL_SIZE = 3;
    	static const short int CHUNK_SIZE = 32;
    	//VoxelElement*** voxels;
    	std::array<std::array<std::shared_ptr<VoxelElement>, CHUNK_SIZE>, CHUNK_SIZE> voxels;

    	Chunk();
    	Chunk(const Vec2i& pos);
    	~Chunk();

    	void UpdateVoxels(ChunkMatrix* matrix);
    	void ResetVoxelUpdateData(ChunkMatrix* matrix);
    	void Render(SDL_Renderer& WindowRenderer) const;

    	bool updateVoxelsNextFrame = true;
    private:
    	short int m_x;
    	short int m_y;
    };
}

class ChunkMatrix {
public:
	ChunkMatrix();
	~ChunkMatrix();


	//grid storage logic
	std::vector<std::vector<Volume::Chunk*>> Grid;
	//precomputed grids for simulation passing - 0 - 3 passees
	std::vector<Volume::Chunk*> GridSegmented[4];

	std::vector<Volume::VoxelParticle*> newParticles;
	std::vector<Volume::VoxelParticle*> particles;

	Volume::Chunk* GetChunkAtWorldPosition(const Vec2f& pos);
	Volume::Chunk* GetChunkAtChunkPosition(const Vec2i& pos);
	void PlaceVoxelsAtMousePosition(const Vec2f& pos, Volume::VoxelType elementType);
	void PlaceParticleAtMousePosition(const Vec2f& pos, Volume::VoxelType particleType, float angle, float speed);
	void RemoveVoxelAtMousePosition(const Vec2f& pos);
	void ExplodeAtMousePosition(const Vec2f& pos, short int radius);
	void GenerateChunk(const Vec2i& pos);

	//Virtual setter / getter
	//Accesses a virtual 2D array that ignores chunks
	std::shared_ptr<Volume::VoxelElement> VirtualGetAt(const Vec2i& pos);
	void VirtualSetAt(std::shared_ptr<Volume::VoxelElement> voxel);

	void GetVoxelsInChunkAtWorldPosition(const Vec2f& pos);
	void GetVoxelsInCubeAtWorldPosition(const Vec2f& start, const Vec2f& end);


	bool IsValidWorldPosition(const Vec2i& pos) const;
	bool IsValidChunkPosition(const Vec2i& pos) const;

	void ExplodeAt(const Vec2i& pos, short int radius);

	//particle functions
	void UpdateParticles();
	void AddParticle(Volume::VoxelType type, const Vec2i& position, float angle, float speed);
	void RenderParticles(SDL_Renderer& renderer);

	//Static functions
	static Vec2i WorldToChunkPosition(const Vec2f& pos);
	static Vec2f ChunkToWorldPosition(const Vec2i& pos);
	static Vec2f MousePosToWorldPos(const Vec2f& mousePos);
};
