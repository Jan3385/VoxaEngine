#pragma once

#include <SDL.h>
#include <glew.h>
#include <array>
#include <mutex>
#include <vector>
#include <SDL_ttf.h>
#include "Voxel.h"
#include "../Math/Vector.h"
#include "../Math/AABB.h"

class DirtyRect{
public:
	Vec2i start;
	Vec2i end;
	DirtyRect(Vec2i start, Vec2i end) : start(start), end(end) {};
	DirtyRect() : start(Vec2i(INT_MAX, INT_MAX)), end(Vec2i(INT_MIN, INT_MIN)) {};
	//Takes in local voxel position
	void Include(Vec2i pos);
	void Update();
	bool IsEmpty() const;
private:
	static constexpr int DIRTY_RECT_PADDING = 1;
	Vec2i m_startW = Vec2i(INT_MAX, INT_MAX); // working dirty rect
	Vec2i m_endW = Vec2i(INT_MIN, INT_MIN);
};

namespace Volume
{
	struct ChunkConnectivityData{
		int32_t chunk;
		int32_t chunkUp;
		int32_t chunkDown;
		int32_t chunkLeft;
		int32_t chunkRight;
	};
    class Chunk {
    public:
    	static const unsigned short int RENDER_VOXEL_SIZE = 5; // 5
    	static const unsigned short int CHUNK_SIZE = 64; // 64
		static const unsigned short int CHUNK_SIZE_SQUARED = CHUNK_SIZE * CHUNK_SIZE; // 4096
    	//VoxelElement*** voxels;
    	std::array<std::array<VoxelElement*, CHUNK_SIZE>, CHUNK_SIZE> voxels;

    	Chunk(const Vec2i& pos);
    	~Chunk();

		bool ShouldChunkDelete(AABB &Camera) const;
		bool ShouldChunkCalculateHeat() const;
		bool ShouldChunkCalculatePressure() const;
    	void UpdateVoxels(ChunkMatrix* matrix);

    	void GetHeatMap(ChunkMatrix *matrix,
			Volume::VoxelHeatData HeatDataArray[],  // flattened arrays
			int chunkNumber);

		void GetPressureMap(ChunkMatrix *matrix,
			Volume::VoxelPressureData PressureDataArray[],  // flattened arrays
			int chunkNumber);

    	void ResetVoxelUpdateData();
    	SDL_Surface* Render(bool debugRender);
		Vec2i GetPos() const;
		AABB GetAABB() const;

		uint8_t lastCheckedCountDown = 20;
		bool forceHeatUpdate = true;
		bool forcePressureUpdate = true;
		bool dirtyRender = true;
		DirtyRect dirtyRect = DirtyRect();

		static const char* computeShaderHeat;
		static GLuint computeShaderHeat_Program;
    private:
    	short int m_x;
    	short int m_y;	
		TTF_Font* font = nullptr;
		SDL_Surface* chunkSurface = nullptr;
    };
}

class ChunkMatrix {
public:
	ChunkMatrix();
	~ChunkMatrix();

	// cleans the chunkMatrix, so that its safe to invoke TFF_CloseFont
	void cleanup();

	//grid storage logic
	std::mutex gridMutex;
	//not precomputed array of chunks
	std::vector<Volume::Chunk*> Grid;
	//precomputed grids for simulation passing - 0 - 3 passees
	std::vector<Volume::Chunk*> GridSegmented[4];

	std::vector<Volume::VoxelParticle*> newParticles;
	std::vector<Volume::VoxelParticle*> particles;

	Volume::Chunk* GetChunkAtWorldPosition(const Vec2f& pos);
	Volume::Chunk* GetChunkAtChunkPosition(const Vec2i& pos);

	void PlaceVoxelsAtMousePosition(const Vec2f &pos, std::string id, Vec2f offset, Volume::Temperature temp);
	void RemoveVoxelAtMousePosition(const Vec2f& pos, Vec2f offset);
	void ExplodeAtMousePosition(const Vec2f& pos, short int radius, Vec2f offset);

	Volume::Chunk* GenerateChunk(const Vec2i& pos);
	void DeleteChunk(const Vec2i& pos);

	void UpdateGridHeat(bool oddHeatUpdatePass);
	void UpdateGridPressure(bool oddPressureUpdatePass);

	//Virtual setter / getter
	//Accesses a virtual 2D array that ignores chunks
	Volume::VoxelElement* VirtualGetAt(const Vec2i& pos);
	Volume::VoxelElement* VirtualGetAt_NoLoad(const Vec2i& pos);
	void VirtualSetAt(Volume::VoxelElement *voxel);
	void VirtualSetAt_NoDelete(Volume::VoxelElement *voxel);

	void PlaceVoxelAt(const Vec2i &pos, std::string id, Volume::Temperature temp, bool placeUnmovableSolids, float amount, bool destructive);

	void GetVoxelsInChunkAtWorldPosition(const Vec2f& pos);
	void GetVoxelsInCubeAtWorldPosition(const Vec2f& start, const Vec2f& end);

	bool IsValidWorldPosition(const Vec2i& pos) const;
	bool IsValidChunkPosition(const Vec2i& pos) const;

	void ExplodeAt(const Vec2i& pos, short int radius);

	//particle functions
	void UpdateParticles();
	void AddParticle(std::string id, const Vec2i &position, Volume::Temperature temp, float amount, float angle, float speed);
	void RenderParticles(SDL_Renderer& renderer, Vec2f offset) const;

	//Static functions
	static Vec2i WorldToChunkPosition(const Vec2f& pos);
	static Vec2f ChunkToWorldPosition(const Vec2i& pos);
	static Vec2f MousePosToWorldPos(const Vec2f& mousePos, Vec2f offset);
private:
	bool cleaned = false;
};
