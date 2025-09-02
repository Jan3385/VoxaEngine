#pragma once

#include "Shader/Computing/ComputeShader.h"
#include "Shader/Buffer/GLBuffer.h"
#include "Shader/Buffer/GLGroupStorageBuffer.h"

#include <cstdint>
#include <GL/glew.h>

class ChunkMatrix; // Forward declaration
struct ChunkConnectivityData; // Forward declaration for template usage

namespace Shader{
       class ChunkShaderManager {
       public:
              ChunkShaderManager();
              ~ChunkShaderManager();

              // collects data for shaders & runs a batch of several chunk shaders
              void BatchRunChunkShaders(ChunkMatrix& chunkMatrix);

              void BindHeatShaderBuffers();
              void BindPressureShaderBuffers();
              void BindReactionShaderBuffers();

              StorageBufferTicket GenerateChunkTicket();
              void DiscardChunkTicket(StorageBufferTicket ticket);
       private:
              // ----- Buffers -----
              // Chunk connectivity buffer
              GLBuffer<ChunkConnectivityData, GL_SHADER_STORAGE_BUFFER> chunkConnectivityBuffer;

              // Buffer for storing temperature in FLOAT as Celsius
              GLGroupStorageBuffer<float> voxelTemperatureBuffer;
              // Buffer for storing heat capacity in FLOAT
              GLGroupStorageBuffer<float> voxelHeatCapacityBuffer;
              // Buffer for storing heat conductivity in FLOAT
              GLGroupStorageBuffer<float> voxelConductivityBuffer;
              // Buffer for storing pressure in FLOAT
              GLGroupStorageBuffer<float> voxelPressureBuffer;
              // Buffer for storing voxel IDs in UINT
              GLGroupStorageBuffer<uint32_t> voxelIdBuffer;

              GLBuffer<float, GL_SHADER_STORAGE_BUFFER> floatOutputDataBuffer;

              struct ChemicalVoxelChanges {
                     uint32_t voxelID;
                     uint32_t localPosX;
                     uint32_t localPosY;
                     uint32_t chunk;
              };
              GLBuffer<ChemicalVoxelChanges, GL_SHADER_STORAGE_BUFFER> chemicalOutputDataBuffer;

              GLBuffer<uint32_t, GL_ATOMIC_COUNTER_BUFFER> atomicCounterBuffer;

              // -------------------


              ComputeShader *heatShader = nullptr;
              ComputeShader *pressureShader = nullptr;
              ComputeShader *reactionShader = nullptr;
              ComputeShader *clearBufferShader = nullptr;
       };
}