#pragma once

#include <cstdint>
#include <GL/glew.h>

class ChunkMatrix; // Forward declaration

namespace ChunkShader{
       GLuint CompileComputeShader(const char* shaderSource);

       struct ChunkConnectivityData{
              int32_t chunk;
              int32_t chunkUp;
              int32_t chunkDown;
              int32_t chunkLeft;
              int32_t chunkRight;
              int32_t _pad[3]; // Padding to ensure alignment
       };

       // Buffer for storing temperature in FLOAT as Celsius
       extern GLuint voxelTemperatureBuffer;
       // Buffer for storing heat capacity in FLOAT
       extern GLuint voxelHeatCapacityBuffer;
       // Buffer for storing heat conductivity in FLOAT
       extern GLuint voxelConductivityBuffer;
       // Buffer for storing pressure in FLOAT
       extern GLuint voxelPressureBuffer;
       // Buffer for storing voxel IDs in UINT
       extern GLuint voxelIdBuffer;
       // Output buffer for any output
       extern GLuint outputDataBuffer;

       // Chunk connectivity buffer
       extern GLuint chunkConnectivityBuffer;

       extern GLuint pressureComputeShaderProgram, 
              heatComputeShaderProgram;

       extern const char* computeShaderHeat;
       extern const char* computeShaderPressure;

       void InitializeBuffers();
       void InitializeComputeShaders();
       
       void RunChunkShaders(ChunkMatrix& chunkMatrix);

       template<typename T>
       void UploadDataToBuffer(GLuint buffer, const T* data, size_t amount);

       template<typename T>
       void UploadDataToUBO(GLuint UBO, const T* data, size_t amount);

       void RunChunkPressureShader(size_t NumberOfChunks);
       void RunChunkHeatShader(size_t NumberOfChunks);

       template <typename T>
       T *ReadDataFromOutputBuffer(size_t VoxelAmount);
       void ClearOutputBuffer(size_t VoxelAmount);
}