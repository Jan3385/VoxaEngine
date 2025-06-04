#pragma once

#include <SDL.h>
#include <array>
#include <mutex>
#include <vector>
#include <SDL_ttf.h>
#include "World/Voxel.h"
#include "Math/Vector.h"
#include "Math/AABB.h"
#include "World/Particle.h"
#include "World/ParticleGenerator.h"

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

		void GetShadersData(
			float temperatureBuffer[],
			float heatCapacityBuffer[],
			float heatConductivityBuffer[],
			float pressureBuffer[],
			uint32_t idBuffer[],
			int chunkNumber
		) const;


    	void SIM_ResetVoxelUpdateData();
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