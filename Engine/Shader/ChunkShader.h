#pragma once

#include "Shader/Computing/ComputeShader.h"
#include "Shader/GLBuffer.h"

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

              template<typename T>
              void ClearOutputBuffer(GLuint size);
       private:
              // ----- Buffers -----
              // Chunk connectivity buffer
              GLBuffer<ChunkConnectivityData, GL_SHADER_STORAGE_BUFFER> chunkConnectivityBuffer;

              // Buffer for storing temperature in FLOAT as Celsius
              GLBuffer<float, GL_SHADER_STORAGE_BUFFER> voxelTemperatureBuffer;
              // Buffer for storing heat capacity in FLOAT
              GLBuffer<float, GL_SHADER_STORAGE_BUFFER> voxelHeatCapacityBuffer;
              // Buffer for storing heat conductivity in FLOAT
              GLBuffer<float, GL_SHADER_STORAGE_BUFFER> voxelConductivityBuffer;
              // Buffer for storing pressure in FLOAT
              GLBuffer<float, GL_SHADER_STORAGE_BUFFER> voxelPressureBuffer;
              // Buffer for storing voxel IDs in UINT
              GLBuffer<uint32_t, GL_SHADER_STORAGE_BUFFER> voxelIdBuffer;
              // Output buffer for any output
              GLuint outputDataBuffer = 0;

              GLBuffer<uint32_t, GL_ATOMIC_COUNTER_BUFFER> atomicCounterBuffer;

              // -------------------


              ComputeShader *heatShader = nullptr;
              ComputeShader *pressureShader = nullptr;
              ComputeShader *reactionShader = nullptr;
              ComputeShader *clearBufferShader = nullptr;
       };
}