#pragma once

#include <glew.h>

#include "World/Chunk.h"
#include "GameObject/Player.h"

class ChunkMatrix {
public:
	ChunkMatrix();
	~ChunkMatrix();

	// cleans the chunkMatrix
	void cleanup();
	
	//mutex for changing the voxels, mainly handeled by the simulation thread
	std::mutex voxelMutex;
	//not precomputed array of chunks
	std::vector<Volume::Chunk*> Grid;
	//precomputed grids for simulation passing -> 0 - 3 passees
	std::vector<Volume::Chunk*> GridSegmented[4];

	std::vector<Particle::VoxelParticle*> newParticles;
	std::vector<Particle::VoxelParticle*> particles;
	std::vector<Particle::ParticleGenerator*> particleGenerators;

	std::vector<GameObject*> gameObjects;

	Volume::Chunk* GetChunkAtWorldPosition(const Vec2f& pos);
	Volume::Chunk* GetChunkAtChunkPosition(const Vec2i& pos);

	void PlaceVoxelsAtMousePosition(const Vec2f &pos, std::string id, Vec2f offset, Volume::Temperature temp);
	void RemoveVoxelAtMousePosition(const Vec2f& pos, Vec2f offset);
	void ExplodeAtMousePosition(const Vec2f& pos, short int radius, Vec2f offset);

	Volume::Chunk* GenerateChunk(const Vec2i& pos);
	void DeleteChunk(const Vec2i& pos);

	//Virtual setter / getter
	//Accesses a virtual 2D array that ignores chunks
	Volume::VoxelElement* VirtualGetAt(const Vec2i& pos);
	Volume::VoxelElement* VirtualGetAt_NoLoad(const Vec2i& pos);
	void VirtualSetAt(Volume::VoxelElement *voxel);
	void VirtualSetAt_NoDelete(Volume::VoxelElement *voxel);

	void PlaceVoxelAt(const Vec2i &pos, std::string id, Volume::Temperature temp, bool placeUnmovableSolids, float amount, bool destructive);
	void PlaceVoxelAt(Volume::VoxelElement *voxel, bool destructive);
	void SetFireAt(const Vec2i &pos, std::optional<Volume::Temperature> temp = std::nullopt);
	// returns true if the gas was displaced. False if no change accured
	bool TryToDisplaceGas(const Vec2i& pos, std::string id, Volume::Temperature temp, float amount, bool placeUnmovableSolids);

	void GetVoxelsInChunkAtWorldPosition(const Vec2f& pos);
	void GetVoxelsInCubeAtWorldPosition(const Vec2f& start, const Vec2f& end);

	bool IsValidWorldPosition(const Vec2i& pos) const;
	bool IsValidChunkPosition(const Vec2i& pos) const;

	void ExplodeAt(const Vec2i& pos, short int radius);

	//particle functions
	void UpdateParticles();
	void RenderParticles(SDL_Renderer& renderer, Vec2f offset) const;
	void RenderObjects(SDL_Renderer& renderer, Vec2f offset) const;

	//Static functions
	static Vec2i WorldToChunkPosition(const Vec2f& pos);
	static Vec2f ChunkToWorldPosition(const Vec2i& pos);
	static Vec2f MousePosToWorldPos(const Vec2f& mousePos, Vec2f offset);
private:
	bool cleaned = false;
};

