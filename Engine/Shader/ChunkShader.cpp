#include "Shader/ChunkShader.h"

#include <iostream>
#include <cstring>

#include "GameEngine.h"
#include "ChunkShader.h"


struct ChunkConnectivityData{
    int32_t chunk;
    int32_t chunkUp;
    int32_t chunkDown;
    int32_t chunkLeft;
    int32_t chunkRight;
    int32_t _pad[3]; // Padding to ensure alignment
};

Shader::ChunkShaderManager::ChunkShaderManager()
{
    glGenBuffers(1, &this->chunkConnectivityBuffer);

    glGenBuffers(1, &this->voxelTemperatureBuffer);
    glGenBuffers(1, &this->voxelHeatCapacityBuffer);
    glGenBuffers(1, &this->voxelConductivityBuffer);
    glGenBuffers(1, &this->voxelPressureBuffer);
    glGenBuffers(1, &this->voxelIdBuffer);

    glGenBuffers(1, &this->outputDataBuffer);

    glGenBuffers(1, &this->atomicCounterBuffer);

    std::cout << "Compiling chunk compute shaders... ";

    this->heatShader = new ComputeShader("ChunkHeat");
    this->pressureShader = new ComputeShader("ChunkPressure");
    this->reactionShader = new ComputeShader("ChunkReactions");
    this->clearBufferShader = new ComputeShader("ClearOutputBuffer.comp", "Output buffer clearing");

    std::cout << "[Done]" << std::endl;
}

Shader::ChunkShaderManager::~ChunkShaderManager()
{
    glDeleteBuffers(1, &this->chunkConnectivityBuffer);
    glDeleteBuffers(1, &this->voxelTemperatureBuffer);
    glDeleteBuffers(1, &this->voxelHeatCapacityBuffer);
    glDeleteBuffers(1, &this->voxelConductivityBuffer);
    glDeleteBuffers(1, &this->voxelPressureBuffer);
    glDeleteBuffers(1, &this->voxelIdBuffer);
    glDeleteBuffers(1, &this->outputDataBuffer);

    delete this->heatShader;
    delete this->pressureShader;
    delete this->clearBufferShader;
}

void Shader::ChunkShaderManager::BatchRunChunkShaders(ChunkMatrix &chunkMatrix)
{
    uint16_t chunkCount = static_cast<uint16_t>(chunkMatrix.Grid.size());
    if(chunkCount == 0) return; // No chunks to process

    uint32_t numberOfVoxels = chunkCount * Volume::Chunk::CHUNK_SIZE_SQUARED;

    std::vector<float>      temperatureBuffer(numberOfVoxels);
    std::vector<float>      heatCapacityBuffer(numberOfVoxels);
    std::vector<float>      heatConductivityBuffer(numberOfVoxels);
    std::vector<float>      pressureBuffer(numberOfVoxels);
    std::vector<uint32_t>   idBuffer(numberOfVoxels);
    std::vector<ChunkConnectivityData> connectivityDataBuffer(chunkCount);

    // Load data for shaders
    std::vector<Volume::Chunk*> chunksToUpdate;
    std::unordered_map<Volume::Chunk*, int> chunkToPositionIndexMap;
    
    for(uint16_t i = 0; i < static_cast<uint16_t>(chunkMatrix.Grid.size()); ++i){
        chunkToPositionIndexMap[chunkMatrix.Grid[i]] = i;
        chunkMatrix.Grid[i]->UpdateInnerTemperatureBuffer();
    }

    #pragma omp parallel for
    for(uint16_t i = 0; i < static_cast<uint16_t>(chunkMatrix.Grid.size()); ++i){
        //TODO: huge performance cost: somehow rework one day
        chunkMatrix.Grid[i]->GetShadersData(
            temperatureBuffer.data(), 
            heatCapacityBuffer.data(), 
            heatConductivityBuffer.data(), 
            pressureBuffer.data(), 
            idBuffer.data(), 
            i
        );

        // Set up chunk connectivity data
        ChunkConnectivityData *data = &connectivityDataBuffer[i];
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
            Vec2i otherPos = chunkMatrix.Grid[j]->GetPos();
            if(otherPos == posUp)
                data->chunkUp = j;
            else if(otherPos == posDown)
                data->chunkDown = j;
            else if(otherPos == posLeft)
                data->chunkLeft = j;
            else if(otherPos == posRight)
                data->chunkRight = j;
        }
    }
    
    // add physicsObjects to the buffers, entering GPU simulations
    for(PhysicsObject *obj : GameEngine::physics->physicsObjects) {
        #pragma omp parallel for collapse(2)
        for(unsigned int y = 0; y < obj->rotatedVoxelBuffer.size(); y++) {
            for(unsigned int x = 0; x < obj->rotatedVoxelBuffer[0].size(); x++) {
                Volume::VoxelElement *voxel = obj->rotatedVoxelBuffer[y][x];
                if(voxel == nullptr) continue; // skip empty voxels

                Vec2i worldPos = obj->GetWorldPositionFromLocalRotatedIndex(x, y);

                Volume::Chunk *c = chunkMatrix.GetChunkAtWorldPosition(worldPos);

                if(!c) continue;

                Vec2i localPos = Vec2i(worldPos.x % Volume::Chunk::CHUNK_SIZE, worldPos.y % Volume::Chunk::CHUNK_SIZE);

                unsigned int index =
                    chunkToPositionIndexMap[c] * Volume::Chunk::CHUNK_SIZE_SQUARED +
                    localPos.y * Volume::Chunk::CHUNK_SIZE +
                    localPos.x;
                
                temperatureBuffer[index] = voxel->temperature.GetCelsius();
                heatCapacityBuffer[index] = voxel->properties->HeatCapacity;
                heatConductivityBuffer[index] = voxel->properties->HeatConductivity;
                pressureBuffer[index] = voxel->amount;
                
                idBuffer[index] = voxel->properties->id |
                    (static_cast<uint32_t>(voxel->GetState() != Volume::State::Gas) << 31) |
                    (static_cast<uint32_t>(voxel->GetState() == Volume::State::Liquid) << 30);

            }
        }
    }

    ComputeShader::UploadDataToBuffer(this->voxelTemperatureBuffer, temperatureBuffer.data(), numberOfVoxels);
    ComputeShader::UploadDataToBuffer(this->voxelHeatCapacityBuffer, heatCapacityBuffer.data(), numberOfVoxels);
    ComputeShader::UploadDataToBuffer(this->voxelConductivityBuffer, heatConductivityBuffer.data(), numberOfVoxels);
    ComputeShader::UploadDataToBuffer(this->voxelPressureBuffer, pressureBuffer.data(), numberOfVoxels);
    ComputeShader::UploadDataToBuffer(this->voxelIdBuffer, idBuffer.data(), numberOfVoxels);

    ComputeShader::UploadDataToBuffer(this->chunkConnectivityBuffer, connectivityDataBuffer.data(), chunkCount);

    float *heatOutput = nullptr;
    if(GameEngine::instance->runHeatSimulation)
    {;
        this->BindHeatShaderBuffers();
        this->heatShader->Use();

        this->heatShader->SetUnsignedInt("NumberOfVoxels", numberOfVoxels);

        this->heatShader->Run(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, chunkCount);
        heatOutput = ComputeShader::ReadDataFromBuffer<float>(this->outputDataBuffer, numberOfVoxels);
        this->ClearOutputBuffer<float>(numberOfVoxels);
    }

    float *pressureOutput = nullptr;
    if(GameEngine::instance->runPressureSimulation)
    {
        this->BindPressureShaderBuffers();
        this->pressureShader->Use();
        
        this->pressureShader->SetUnsignedInt("NumberOfVoxels", numberOfVoxels);

        this->pressureShader->Run(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, chunkCount);
        pressureOutput = ComputeShader::ReadDataFromBuffer<float>(this->outputDataBuffer, numberOfVoxels);
        this->ClearOutputBuffer<float>(numberOfVoxels);
    }

    struct VoxelChanges {
        uint32_t voxelID;
        uint32_t localPosX;
        uint32_t localPosY;
        uint32_t chunk;
    };

    VoxelChanges *reactionOutput = nullptr;
    uint32_t reactionsSize = 0;
    if(GameEngine::instance->runChemicalReactions)
    {
        this->BindReactionShaderBuffers();
        this->reactionShader->Use();

        this->reactionShader->SetUnsignedInt("NumberOfVoxels", numberOfVoxels);
        uint32_t randomNumber = static_cast<uint32_t>(rand() % 1000);
        this->reactionShader->SetUnsignedInt("randomNumber", randomNumber);

        this->reactionShader->Run(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, chunkCount);

        // Read the output data
        reactionsSize = ComputeShader::ReadDataFromAtomicCounter<uint32_t>(this->atomicCounterBuffer);
        reactionOutput = ComputeShader::ReadDataFromBuffer<VoxelChanges>(this->outputDataBuffer, reactionsSize);
        this->ClearOutputBuffer<VoxelChanges>(numberOfVoxels);
    }

    #pragma omp parallel for
    for (uint32_t i = 0; i < numberOfVoxels; i++) {
        uint16_t chunkIndex = i / Volume::Chunk::CHUNK_SIZE_SQUARED;
        uint16_t voxelIndex = i % Volume::Chunk::CHUNK_SIZE_SQUARED;	
        uint16_t x = voxelIndex % Volume::Chunk::CHUNK_SIZE;
        uint16_t y = voxelIndex / Volume::Chunk::CHUNK_SIZE;

        auto& chunk = chunkMatrix.Grid[chunkIndex];
        if(heatOutput) chunk->voxels[y][x]->temperature.SetCelsius(heatOutput[i]);
        if(pressureOutput) chunk->voxels[y][x]->amount = pressureOutput[i];

        std::string newId = chunk->voxels[y][x]->ShouldTransitionToID();
        if(!newId.empty()){
            chunk->voxels[y][x]->DieAndReplace(chunkMatrix, newId);
        }
    }
    std::map<uint16_t, Vec2i> chunkIndexToPosOffset;
    for(uint16_t i = 0; i < static_cast<uint16_t>(chunkMatrix.Grid.size()); ++i){
        chunkIndexToPosOffset[i] = chunkMatrix.Grid[i]->GetPos() * Volume::Chunk::CHUNK_SIZE;
    }

    //#pragma omp parallel for ( crashes :( )
    for(uint32_t i = 0; i < reactionsSize; i++) {
        VoxelChanges& change = reactionOutput[i];
        std::string stringID = Registry::VoxelRegistry::GetStringID(change.voxelID);
        Vec2i voxelPos =  Vec2i(change.localPosX, change.localPosY) + chunkIndexToPosOffset[change.chunk];

        Volume::VoxelElement* oldVoxel = chunkMatrix.VirtualGetAt(voxelPos, false);
        Volume::VoxelElement* voxel = CreateVoxelElement(
            stringID, 
            voxelPos, 
            oldVoxel->amount, 
            oldVoxel->temperature,
            oldVoxel->IsUnmoveableSolid()
        );
        chunkMatrix.PlaceVoxelAt(voxel, true, true);
    }

    delete[] heatOutput;
    delete[] pressureOutput;
    delete[] reactionOutput;
}

void Shader::ChunkShaderManager::BindHeatShaderBuffers()
{
    ComputeShader::BindBufferAt(0, this->outputDataBuffer);
    ComputeShader::BindBufferAt(1, this->voxelTemperatureBuffer);
    ComputeShader::BindBufferAt(2, this->voxelHeatCapacityBuffer);
    ComputeShader::BindBufferAt(3, this->voxelConductivityBuffer);
    ComputeShader::BindBufferAt(4, this->chunkConnectivityBuffer);
}

void Shader::ChunkShaderManager::BindPressureShaderBuffers()
{
    ComputeShader::BindBufferAt(0, this->outputDataBuffer);
    ComputeShader::BindBufferAt(1, this->voxelIdBuffer);
    ComputeShader::BindBufferAt(2, this->voxelPressureBuffer);
    ComputeShader::BindBufferAt(3, this->chunkConnectivityBuffer);
}

void Shader::ChunkShaderManager::BindReactionShaderBuffers()
{
    GLuint zero = 0;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, this->atomicCounterBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_COPY);

    ComputeShader::BindBufferAt(0, this->outputDataBuffer);
    ComputeShader::BindAtomicCounterAt(1, this->atomicCounterBuffer);
    ComputeShader::BindBufferAt(2, this->voxelTemperatureBuffer);
    ComputeShader::BindBufferAt(3, this->voxelIdBuffer);
    ComputeShader::BindBufferAt(4, Registry::VoxelRegistry::chemicalReactionsBuffer);
    ComputeShader::BindBufferAt(5, this->chunkConnectivityBuffer);
}

template<typename T>
void Shader::ChunkShaderManager::ClearOutputBuffer(GLuint size)
{
    static GLuint lastSize = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputDataBuffer);

    // Only reallocate if size changed
    if (lastSize != size) {
        glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(T), nullptr, GL_DYNAMIC_COPY);
        lastSize = size;
    }

    // Clear buffer data
    #ifdef GL_VERSION_4_3
    if constexpr (std::is_same_v<T, float>) {
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, nullptr);
    }else{
        void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        if (ptr) {
            memset(ptr, 0, size * sizeof(T));
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
    }
    #else
    // Fallback - map and memset to zero
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    if (ptr) {
        memset(ptr, 0, size * sizeof(T));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    #endif
}
