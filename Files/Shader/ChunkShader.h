#pragma once

#include "Shader/Computing/ComputeShader.h"

#include <cstdint>
#include <GL/glew.h>

class ChunkMatrix; // Forward declaration

namespace Shader{
       class ChunkShaderManager {
       public:
              ChunkShaderManager();
              ~ChunkShaderManager();

              // collects data for shaders & runs a batch of several chunk shaders
              void BatchRunChunkShaders(ChunkMatrix& chunkMatrix);

              void BindHeatShaderBuffers();
              void BindPressureShaderBuffers();

              void ClearOutputBuffer(GLuint size);
       private:
              // ----- Buffers -----

              // Chunk connectivity buffer
              GLuint chunkConnectivityBuffer = 0;

              // Buffer for storing temperature in FLOAT as Celsius
              GLuint voxelTemperatureBuffer = 0;
              // Buffer for storing heat capacity in FLOAT
              GLuint voxelHeatCapacityBuffer = 0;
              // Buffer for storing heat conductivity in FLOAT
              GLuint voxelConductivityBuffer = 0;
              // Buffer for storing pressure in FLOAT
              GLuint voxelPressureBuffer = 0;
              // Buffer for storing voxel IDs in UINT
              GLuint voxelIdBuffer = 0;
              // Output buffer for any output
              GLuint outputDataBuffer = 0;

              // -------------------


              ComputeShader *heatShader = nullptr;
              ComputeShader *pressureShader = nullptr;
              ComputeShader *clearBufferShader = nullptr;
       };
}