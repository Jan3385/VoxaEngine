#pragma once

#include <SDL.h>
#include <array>
#include <mutex>
#include <vector>
#include <GL/glew.h>

#include <glm/glm.hpp>

#include <box2d/box2d.h>

#include "Physics/Triangle.h"

#include "Math/Math.h"

#include "World/Voxel.h"
#include "World/Particle.h"
#include "World/ParticleGenerator.h"

class VoxelObject;

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
	struct VoxelRenderData{
		glm::ivec2 position; // position in chunk
		glm::vec4 color; 	// RGBA color
	};
	struct ChunkConnectivityData{
		int32_t chunk;
		int32_t chunkUp;
		int32_t chunkDown;
		int32_t chunkLeft;
		int32_t chunkRight;
	};
    class Chunk {
    public:
    	static const unsigned short int RENDER_VOXEL_SIZE = 4; // 4
    	static const unsigned short int CHUNK_SIZE = 64; // 64
		static const unsigned short int CHUNK_SIZE_SQUARED = CHUNK_SIZE * CHUNK_SIZE; // 4096
    	
		VoxelElement* voxels[CHUNK_SIZE][CHUNK_SIZE];
		std::vector<VoxelObject*> voxelObjectInChunk;

    	Chunk(const Vec2i& pos);
    	~Chunk();

		void InitializeBuffers();
		bool IsInitialized() const { return this->idVBO.IsInitialized(); };

		bool ShouldChunkDelete(AABB Camera) const;
		bool ShouldChunkCalculateHeat() const;
		bool ShouldChunkCalculatePressure() const;

		void UpdateComputeGPUBuffers();
		void UpdateRenderGPUBuffers();
		void SetTemperatureAt(Vec2i pos, Temperature temperature);
		void SetPressureAt(Vec2i pos, float pressure);
		void UpdatedVoxelAt(Vec2i pos);

    	void UpdateVoxels(ChunkMatrix* matrix);

		// Physics
		bool dirtyColliders = true;
		void UpdateColliders(std::vector<Triangle> &triangles, std::vector<b2Vec2> &edges, b2WorldId worldId);
		b2BodyId GetPhysicsBody() const { return m_physicsBody; }
		std::vector<Triangle> &GetColliders() { return m_triangleColliders; }
		std::vector<b2Vec2> &GetEdges() { return m_edges; }

		void GetShadersData(
			float temperatureBuffer[],
			float heatCapacityBuffer[],
			float heatConductivityBuffer[],
			float pressureBuffer[],
			uint32_t idBuffer[],
			int chunkNumber
		) const;


    	void SIM_ResetVoxelUpdateData();
    	void Render(bool debugRender);
		Vec2i GetPos() const;
		AABB GetAABB() const;

		uint8_t lastCheckedCountDown = 20;
		bool forceHeatUpdate = true;
		bool forcePressureUpdate = true;
		DirtyRect dirtyRect = DirtyRect();

		// connectivity data
		Shader::GLVertexArray renderVoxelVAO;
		Shader::GLVertexArray heatRenderingVAO;

		Shader::GLBuffer<float, GL_SHADER_STORAGE_BUFFER> temperatureVBO;
		Shader::GLBuffer<float, GL_ARRAY_BUFFER> renderTemperatureVBO;
		Shader::GLBuffer<float, GL_SHADER_STORAGE_BUFFER> pressureVBO;

		Shader::GLBuffer<float, GL_SHADER_STORAGE_BUFFER> heatCapacityVBO;
		Shader::GLBuffer<float, GL_SHADER_STORAGE_BUFFER> heatConductivityVBO;
		Shader::GLBuffer<uint32_t, GL_SHADER_STORAGE_BUFFER> idVBO;
    private:
    	short int m_x;
    	short int m_y;	

		b2BodyId m_physicsBody = b2_nullBodyId;
		std::vector<Triangle> m_triangleColliders;
		std::vector<b2Vec2> m_edges;

		void DestroyPhysicsBody();
		void CreatePhysicsBody(b2WorldId worldId);

		// rendering data
		VoxelRenderData renderData[CHUNK_SIZE][CHUNK_SIZE];
		Shader::GLBuffer<VoxelRenderData, GL_ARRAY_BUFFER> renderVBO;
		// knows where to update render for chunk
		Math::Range updateRenderBufferRanges[CHUNK_SIZE]; 
		Math::Range updateVoxelIdRanges[CHUNK_SIZE];
		Math::Range updateVoxelTemperatureRanges[CHUNK_SIZE];
		Math::Range updateVoxelPressureRanges[CHUNK_SIZE];
    };
}