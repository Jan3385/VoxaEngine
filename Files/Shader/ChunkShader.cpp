#include "Shader/ChunkShader.h"

#include <iostream>
#include <cstring>

#include "GameEngine.h"

using namespace ChunkShader;

// Buffer for storing temperature in FLOAT as Celsius
GLuint ChunkShader::voxelTemperatureBuffer;
// Buffer for storing heat capacity in FLOAT
GLuint ChunkShader::voxelHeatCapacityBuffer;
// Buffer for storing heat conductivity in FLOAT
GLuint ChunkShader::voxelConductivityBuffer;
// Buffer for storing pressure in FLOAT
GLuint ChunkShader::voxelPressureBuffer;
// Buffer for storing voxel IDs in UINT
GLuint ChunkShader::voxelIdBuffer;
// Output buffer for any output
GLuint ChunkShader::outputDataBuffer;

// Chunk connectivity buffer
GLuint ChunkShader::chunkConnectivityBuffer;

GLuint  ChunkShader::pressureComputeShaderProgram, 
        ChunkShader::heatComputeShaderProgram;

GLuint ChunkShader::CompileComputeShader(const char* shaderSource)
{
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &shaderSource, NULL);
    glCompileShader(computeShader);

    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
        std::cerr << "Error compiling compute shader: " << infoLog << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Error linking compute shader program: " << infoLog << std::endl;
    }

    glDeleteShader(computeShader);

    return program;
}

void ChunkShader::InitializeBuffers()
{
    glGenBuffers(1, &voxelTemperatureBuffer);
    glGenBuffers(1, &voxelHeatCapacityBuffer);
    glGenBuffers(1, &voxelConductivityBuffer);
    glGenBuffers(1, &voxelPressureBuffer);
    glGenBuffers(1, &voxelIdBuffer);
    glGenBuffers(1, &outputDataBuffer);
    glGenBuffers(1, &chunkConnectivityBuffer);
}

void ChunkShader::InitializeComputeShaders()
{
    heatComputeShaderProgram = CompileComputeShader(computeShaderHeat);
    pressureComputeShaderProgram = CompileComputeShader(computeShaderPressure);
}

void ChunkShader::RunChunkShaders(ChunkMatrix &chunkMatrix)
{
    uint16_t chunkCount = static_cast<uint16_t>(chunkMatrix.Grid.size());

    if(chunkCount == 0) return; // No chunks to process

    uint32_t NumberOfVoxels = chunkCount * Volume::Chunk::CHUNK_SIZE_SQUARED;

    std::vector<float> temperatureBuffer(NumberOfVoxels);
    std::vector<float> heatCapacityBuffer(NumberOfVoxels);
    std::vector<float> heatConductivityBuffer(NumberOfVoxels);
    std::vector<float> pressureBuffer(NumberOfVoxels);
    std::vector<uint32_t> idBuffer(NumberOfVoxels);
    std::vector<ChunkConnectivityData> connectivityDataBuffer(chunkCount);

    // Load data for shaders
    uint16_t chunkIndex = 0;
    std::vector<Volume::Chunk*> chunksToUpdate;
    for(uint16_t i = 0; i < static_cast<uint16_t>(chunkMatrix.Grid.size()); ++i){
        chunkMatrix.Grid[i]->GetShadersData(
            temperatureBuffer.data(), 
            heatCapacityBuffer.data(), 
            heatConductivityBuffer.data(), 
            pressureBuffer.data(), 
            idBuffer.data(), 
            chunkIndex++
        );

        ChunkShader::ChunkConnectivityData *data = &connectivityDataBuffer[i];
        data->chunk = i;
        data->chunkUp = -1;
        data->chunkDown = -1;
        data->chunkLeft = -1;
        data->chunkRight = -1;
        Vec2i pos = chunkMatrix.Grid[i]->GetPos();
        Vec2i posUp = pos + vector::UP;
        Vec2i posDown = pos + vector::DOWN;
        Vec2i posLeft = pos + vector::LEFT;
        Vec2i posRight = pos + vector::RIGHT;
        for(uint16_t j = 0; j < static_cast<uint16_t>(chunkMatrix.Grid.size()); ++j){
            if(chunkMatrix.Grid[j]->GetPos() == posUp)
                data->chunkUp = j;
            else if(chunkMatrix.Grid[j]->GetPos() == posDown)
                data->chunkDown = j;
            else if(chunkMatrix.Grid[j]->GetPos() == posLeft)
                data->chunkLeft = j;
            else if(chunkMatrix.Grid[j]->GetPos() == posRight)
                data->chunkRight = j;
        }
    }

    UploadDataToBuffer(voxelTemperatureBuffer, temperatureBuffer.data(), NumberOfVoxels);
    UploadDataToBuffer(voxelHeatCapacityBuffer, heatCapacityBuffer.data(), NumberOfVoxels);
    UploadDataToBuffer(voxelConductivityBuffer, heatConductivityBuffer.data(), NumberOfVoxels);
    UploadDataToBuffer(voxelPressureBuffer, pressureBuffer.data(), NumberOfVoxels);
    UploadDataToBuffer(voxelIdBuffer, idBuffer.data(), NumberOfVoxels);
    UploadDataToBuffer(chunkConnectivityBuffer, connectivityDataBuffer.data(), chunkCount);

    ClearOutputBuffer(NumberOfVoxels);
    RunChunkHeatShader(chunkCount);
    float* heatOutputData = ReadDataFromOutputBuffer<float>(NumberOfVoxels);

    ClearOutputBuffer(NumberOfVoxels);
    RunChunkPressureShader(chunkCount);
    float* pressureOutputData = ReadDataFromOutputBuffer<float>(NumberOfVoxels);

    #pragma omp parallel for
    for(uint32_t i = 0; i < NumberOfVoxels; ++i){
        uint16_t chunkIndex = i / Volume::Chunk::CHUNK_SIZE_SQUARED;
        uint16_t voxelIndex = i % Volume::Chunk::CHUNK_SIZE_SQUARED;	
        uint16_t x = voxelIndex % Volume::Chunk::CHUNK_SIZE;
        uint16_t y = voxelIndex / Volume::Chunk::CHUNK_SIZE;

        auto& chunk = chunkMatrix.Grid[chunkIndex];
        chunk->voxels[y][x]->temperature.SetCelsius(heatOutputData[i]);
        chunk->voxels[y][x]->amount = pressureOutputData[i];

        std::string newId = chunk->voxels[y][x]->ShouldTransitionToID();
        if(!newId.empty()){
            chunk->voxels[y][x]->DieAndReplace(chunkMatrix, newId);
        }
    }

    delete[] heatOutputData;
    delete[] pressureOutputData;
}

// Uploads data to a buffer of type T.
template<typename T>
void ChunkShader::UploadDataToBuffer(GLuint buffer, const T* data, size_t amount){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, amount * sizeof(T), data, GL_STATIC_DRAW); 
}

// Uploads data to a Uniform Buffer Object (UBO) of type T.
template<typename T>
void ChunkShader::UploadDataToUBO(GLuint UBO, const T* data, size_t amount){
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferData(GL_UNIFORM_BUFFER, amount * sizeof(T), data, GL_STATIC_DRAW); 
}

/**
 * Reads data from the output buffer and returns a pointer to an array of type T.
 * @attention The caller is responsible for deleting the returned pointer!
 * @param VoxelAmount The number of elements to read from the output buffer.
 * @return A pointer to an array of type T containing the data read from the output buffer.
 */
template <typename T>
T *ChunkShader::ReadDataFromOutputBuffer(size_t VoxelAmount)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputDataBuffer);
    T* data = static_cast<T*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
    if (!data) {
        throw std::runtime_error("Failed to map output buffer for ChunkShader (NULLPTR)");
    }

    T* result = new T[VoxelAmount];
    std::memcpy(result, data, VoxelAmount * sizeof(T));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    return result;
}

// Needs set voxelIdBuffer, voxelPressureBuffer, chunkConnectivityUniformBuffer.. Sets outputDataBuffer (FLOAT)
void ChunkShader::RunChunkPressureShader(size_t NumberOfChunks){
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, outputDataBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxelIdBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, voxelPressureBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, chunkConnectivityBuffer);
    
    glUseProgram(pressureComputeShaderProgram);

    GLint location = glGetUniformLocation(pressureComputeShaderProgram, "NumberOfVoxels");
    if (location == -1) {
        throw std::runtime_error("Failed to get uniform location for NumberOfVoxels in pressure compute shader");
    }
    glUniform1ui(location, NumberOfChunks * Volume::Chunk::CHUNK_SIZE_SQUARED);

    glDispatchCompute(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, NumberOfChunks);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
// Needs set voxelTemperatureBuffer, voxelHeatCapacityBuffer, voxelConductivityBuffer, chunkConnectivityUniformBuffer.. Sets outputDataBuffer (FLOAT)
void ChunkShader::RunChunkHeatShader(size_t NumberOfChunks) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, outputDataBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxelTemperatureBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, voxelHeatCapacityBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, voxelConductivityBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, chunkConnectivityBuffer);

    glUseProgram(heatComputeShaderProgram);

    GLint location = glGetUniformLocation(heatComputeShaderProgram, "NumberOfVoxels");
    if (location == -1) {
        throw std::runtime_error("Failed to get uniform location for NumberOfVoxels in heat compute shader");
    }
    glUniform1ui(location, NumberOfChunks * Volume::Chunk::CHUNK_SIZE_SQUARED);

    glDispatchCompute(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, NumberOfChunks);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
}

void ChunkShader::ClearOutputBuffer(size_t VoxelAmount)
{
    std::vector<float> zeros(VoxelAmount, 0.0f);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputDataBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, VoxelAmount * sizeof(float), zeros.data(), GL_DYNAMIC_COPY);
}

const char* ChunkShader::computeShaderHeat = R"glsl(#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

#define TEMPERATURE_TRANSITION_SPEED 80

#define CHUNK_SIZE 64
#define CHUNK_SIZE_SQUARED 4096

#define DIRECTION_COUNT 4

const ivec2 directions[DIRECTION_COUNT] = {
    ivec2(0, -1),
    ivec2(-1, 0),
    ivec2(1, 0),
    ivec2(0, 1),
};

struct ChunkConnectivityData{
    int chunk;
    int chunkUp;
    int chunkDown;
    int chunkLeft;
    int chunkRight;
    int _pad[3]; // padding to 32 bytes - 8 * 4 = 32 bytes
};

// flattened arrays (c = chunk, x = x, y = y)
layout(std430, binding = 0) buffer OutputBuffer {
    float voxelTempsOut[];
};
layout(std430, binding = 1) buffer TemperatureBuffer {
    float voxelTemps[];
};
layout(std430, binding = 2) buffer HeatCapacityBuffer {
    float voxelHeatCapacity[];
};
layout(std430, binding = 3) buffer ConductivityBuffer {
    float voxelConductivity[];
};

uniform uint NumberOfVoxels; // total number of voxels in the simulation

layout(std430, binding = 4) buffer ChunkBuffer {
    ChunkConnectivityData chunkData[];
};


void main(){
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint c = gl_GlobalInvocationID.z;

    uint localX = x % CHUNK_SIZE;
    uint localY = y % CHUNK_SIZE;

    uint index = c * CHUNK_SIZE_SQUARED + y * CHUNK_SIZE + x;

    uint numberOfChunks = NumberOfVoxels / CHUNK_SIZE_SQUARED;

    float sum = 0.0;

    ivec2 pos = ivec2(x, y);
    ivec2 localPos = ivec2(localX, localY);

    uint NumOfValidDirections = 0;
    for(int i = 0; i < DIRECTION_COUNT; ++i){
        ivec2 testPos = localPos + directions[i];

        ivec2 nPos = pos + directions[i];
        uint nIndex;

        // forbid diagonal heat transfer
        if((testPos.x < 0 || testPos.x >= CHUNK_SIZE) && (testPos.y < 0 || testPos.y >= CHUNK_SIZE))
            continue;

        // if out of bounds from current chunk
        if(testPos.x < 0 || testPos.x >= CHUNK_SIZE || testPos.y < 0 || testPos.y >= CHUNK_SIZE) {
            if(testPos.x < 0){ // right
                int nC = -1;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkRight == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + CHUNK_SIZE + (nPos.x - CHUNK_SIZE);
            }else if (testPos.x >= CHUNK_SIZE){ // left
                int nC = -1;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkLeft == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + (CHUNK_SIZE - nPos.x);
            }
            if(testPos.y >= CHUNK_SIZE){ // up
                int nC = -1;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkUp == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + (CHUNK_SIZE - nPos.y) * CHUNK_SIZE + nPos.x;
            }else if(testPos.y < 0){ // down
                int nC = -1;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkDown == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + (CHUNK_SIZE + nPos.y) * CHUNK_SIZE + nPos.x;
            }
        }else{ // if in bounds
            nIndex = c * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + nPos.x;
        }

        if (nIndex < 0 || nIndex >= NumberOfVoxels)
            continue;

        ++NumOfValidDirections;
        
        float heatCapacity = voxelHeatCapacity[index]/TEMPERATURE_TRANSITION_SPEED;
        float heatDiff = voxelTemps[nIndex] - voxelTemps[index];
        float heatTrans = heatDiff * voxelConductivity[nIndex] / heatCapacity;

        if(heatCapacity <= 0.0f)
            heatTrans = 0.0f;

        sum += heatTrans;
    }

    if(NumOfValidDirections == 0) NumOfValidDirections = 1;
    
    voxelTempsOut[index] = voxelTemps[index] + (sum / NumOfValidDirections);
}
)glsl";

const char* ChunkShader::computeShaderPressure = R"glsl(#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

// bigger number = slower
#define PRESSURE_TRANSITION_SPEED 1.1

#define CHUNK_SIZE 64
#define CHUNK_SIZE_SQUARED 4096

#define DIRECTION_COUNT 4

const ivec2 directions[DIRECTION_COUNT] = {
    ivec2(0, -1),
    ivec2(-1, 0),
    ivec2(1, 0),
    ivec2(0, 1),
};
struct ChunkConnectivityData{
    int chunk;
    int chunkUp;
    int chunkDown;
    int chunkLeft;
    int chunkRight;
    int _pad[3]; // padding to 32 bytes - 8 * 4 = 32 bytes
};

layout(std430, binding = 0) buffer OutputChunkBuffer {
    float voxelPressureOut[];
};
layout(std430, binding = 1) buffer IdBuffer {
    uint voxelIds[];
};
layout(std430, binding = 2) buffer PressureBuffer {
    float voxelPressures[];
};

uniform uint NumberOfVoxels; // total number of voxels in the simulation

layout(std430, binding = 3) buffer ChunkBuffer {
    ChunkConnectivityData chunkData[];
};

void main(){
	uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint c = gl_GlobalInvocationID.z;

    uint localX = x % CHUNK_SIZE;
    uint localY = y % CHUNK_SIZE;

	uint index = c * CHUNK_SIZE_SQUARED + y * CHUNK_SIZE + x;

	// if not gas, return
	if((voxelIds[index] & (0x1 << 31)) != 0){
		voxelPressureOut[index] = voxelPressures[index];
		return;
	}

	uint numberOfChunks = NumberOfVoxels / CHUNK_SIZE_SQUARED;

	float sum = 0.0;

	ivec2 pos = ivec2(x, y);
    ivec2 localPos = ivec2(localX, localY);

	uint NumOfValidDirections = 0;
	for(int i = 0; i < DIRECTION_COUNT; ++i){
		ivec2 testPos = localPos + directions[i];
        ivec2 nPos = pos + directions[i];
        uint nIndex;

        // forbid diagonal heat transfer
        if((testPos.x < 0 || testPos.x >= CHUNK_SIZE) && (testPos.y < 0 || testPos.y >= CHUNK_SIZE))
            continue;

		// if out of bounds from current chunk
        if(testPos.x < 0 || testPos.x >= CHUNK_SIZE || testPos.y < 0 || testPos.y >= CHUNK_SIZE) {
            if(testPos.x < 0){ // right
                int nC = -1;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkRight == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + CHUNK_SIZE + (nPos.x - CHUNK_SIZE);
                ++NumOfValidDirections;
            }else if (testPos.x >= CHUNK_SIZE){ // left
                int nC = -1;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkLeft == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + (CHUNK_SIZE - nPos.x);
                ++NumOfValidDirections;
            }
            if(testPos.y >= CHUNK_SIZE){ // up
                int nC = -1;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkUp == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + (CHUNK_SIZE - nPos.y) * CHUNK_SIZE + nPos.x;
                ++NumOfValidDirections;
            }else if(testPos.y < 0){ // down
                int nC = -1;
                for(int i = 0; i < numberOfChunks; ++i){
                    if(chunkData[i].chunkDown == c){
                        nC = chunkData[i].chunk;
                        break;
                    }
                }
                if(nC == -1) continue;
                nIndex = nC * CHUNK_SIZE_SQUARED + (CHUNK_SIZE + nPos.y) * CHUNK_SIZE + nPos.x;
                ++NumOfValidDirections;
            }
        }else{ // if in bounds
            nIndex = c * CHUNK_SIZE_SQUARED + nPos.y * CHUNK_SIZE + nPos.x;
            ++NumOfValidDirections;
        }

		if(voxelIds[index] == voxelIds[nIndex]){
			float pressureDiff = voxelPressures[index] - voxelPressures[nIndex];
			float pressureTransfer = pressureDiff / PRESSURE_TRANSITION_SPEED;
			sum += pressureTransfer;
		}else
			--NumOfValidDirections;
	}

	if(NumOfValidDirections == 0) NumOfValidDirections = 1;
    
    voxelPressureOut[index] = voxelPressures[index] - (sum / NumOfValidDirections);
}
)glsl";