#include "Shader/ChunkShader.h"

#include <iostream>
#include <cstring>

#include "GameEngine.h"
#include "ChunkShader.h"
#include "Math/Temperature.h"


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
    this->chunkConnectivityBuffer = GLBuffer<ChunkConnectivityData, GL_SHADER_STORAGE_BUFFER>("Chunk Connectivity Buffer");

    this->voxelTemperatureBuffer = GLBuffer<float, GL_SHADER_STORAGE_BUFFER>("Voxel Temperature Buffer");
    this->voxelHeatCapacityBuffer = GLBuffer<float, GL_SHADER_STORAGE_BUFFER>("Voxel Heat Capacity Buffer");
    this->voxelConductivityBuffer = GLBuffer<float, GL_SHADER_STORAGE_BUFFER>("Voxel Conductivity Buffer");
    this->voxelPressureBuffer = GLBuffer<float, GL_SHADER_STORAGE_BUFFER>("Voxel Pressure Buffer");
    this->voxelIdBuffer = GLBuffer<uint32_t, GL_SHADER_STORAGE_BUFFER>("Voxel ID Buffer");

    this->floatOutputDataBuffer = GLBuffer<float, GL_SHADER_STORAGE_BUFFER>("Float Output Data Buffer");
    this->chemicalOutputDataBuffer = GLBuffer<ChemicalVoxelChanges, GL_SHADER_STORAGE_BUFFER>("Chemical Output Data Buffer");

    this->atomicCounterBuffer = GLBuffer<uint32_t, GL_ATOMIC_COUNTER_BUFFER>("Atomic Counter Buffer");

    std::cout << "Compiling chunk compute shaders... ";

    this->heatShader = new ComputeShader("ChunkHeat");
    this->pressureShader = new ComputeShader("ChunkPressure");
    this->reactionShader = new ComputeShader("ChunkReactions");
    this->clearBufferShader = new ComputeShader("ClearOutputBuffer.comp", "Output buffer clearing");

    std::cout << "[Done]" << std::endl;
}

Shader::ChunkShaderManager::~ChunkShaderManager()
{
    delete this->heatShader;
    delete this->pressureShader;
    delete this->clearBufferShader;
}

void Shader::ChunkShaderManager::BatchRunChunkShaders(ChunkMatrix &chunkMatrix)
{
    std::vector<Volume::Chunk*> chunksToUpdate;
    for(Volume::Chunk* chunk : chunkMatrix.Grid){
        if(chunk->IsInitialized()){
            chunksToUpdate.push_back(chunk);
        }
    }

    uint16_t chunkCount = static_cast<uint16_t>(chunksToUpdate.size());
    if(chunkCount == 0) return; // No chunks to process

    uint32_t numberOfVoxels = chunkCount * Volume::Chunk::CHUNK_SIZE_SQUARED;

    std::vector<ChunkConnectivityData> connectivityDataBuffer(chunkCount);

    // Load data for shaders
    std::unordered_map<Volume::Chunk*, int> chunkToPositionIndexMap;
    
    for(uint16_t i = 0; i < static_cast<uint16_t>(chunksToUpdate.size()); ++i){
        chunkToPositionIndexMap[chunksToUpdate[i]] = i;
    }

    voxelTemperatureBuffer.ClearBuffer(numberOfVoxels, GL_DYNAMIC_DRAW);
    voxelHeatCapacityBuffer.ClearBuffer(numberOfVoxels, GL_DYNAMIC_DRAW);
    voxelConductivityBuffer.ClearBuffer(numberOfVoxels, GL_DYNAMIC_DRAW);
    voxelPressureBuffer.ClearBuffer(numberOfVoxels, GL_DYNAMIC_DRAW);
    voxelIdBuffer.ClearBuffer(numberOfVoxels, GL_DYNAMIC_DRAW);

    for(uint16_t i = 0; i < static_cast<uint16_t>(chunksToUpdate.size()); ++i){
        Volume::Chunk *c = chunksToUpdate[i];
        c->UpdateComputeGPUBuffers();

        unsigned int offset = i * Volume::Chunk::CHUNK_SIZE_SQUARED;

        voxelTemperatureBuffer.UploadBufferIn(0, offset, c->temperatureVBO, 0);
        voxelHeatCapacityBuffer.UploadBufferIn(0, offset, c->heatCapacityVBO, 0);
        voxelConductivityBuffer.UploadBufferIn(0, offset, c->heatConductivityVBO, 0);
        voxelPressureBuffer.UploadBufferIn(0, offset, c->pressureVBO, 0);
        voxelIdBuffer.UploadBufferIn(0, offset, c->idVBO, 0);
    }

    #pragma omp parallel for
    for(uint16_t i = 0; i < static_cast<uint16_t>(chunksToUpdate.size()); ++i){
        // Set up chunk connectivity data
        ChunkConnectivityData *data = &connectivityDataBuffer[i];
        data->chunk = i;
        data->chunkUp = -1;
        data->chunkDown = -1;
        data->chunkLeft = -1;
        data->chunkRight = -1;
        Vec2i pos = chunksToUpdate[i]->GetPos();
        Vec2i posUp = pos + vector::UP;
        Vec2i posDown = pos + vector::DOWN;
        Vec2i posLeft = pos + vector::LEFT;
        Vec2i posRight = pos + vector::RIGHT;
        for(uint16_t j = 0; j < static_cast<uint16_t>(chunksToUpdate.size()); ++j){
            Vec2i otherPos = chunksToUpdate[j]->GetPos();
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

    chunkConnectivityBuffer.SetData(connectivityDataBuffer, GL_DYNAMIC_DRAW);

    // Run heat simulation
    float *heatOutput = nullptr;
    if(GameEngine::instance->runHeatSimulation)
    {
        floatOutputDataBuffer.ClearBuffer(numberOfVoxels, GL_DYNAMIC_DRAW);
        this->BindHeatShaderBuffers();
        this->heatShader->Use();

        this->heatShader->SetUnsignedInt("NumberOfVoxels", numberOfVoxels);

        this->heatShader->Run(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, chunkCount);

        heatOutput = floatOutputDataBuffer.ReadBuffer(numberOfVoxels);

        // Update the GPU buffer of the chunk
        for(uint16_t i = 0; i < chunksToUpdate.size(); ++i){
            unsigned int offset = i * Volume::Chunk::CHUNK_SIZE_SQUARED;
            Volume::Chunk *c = chunksToUpdate[i];

            c->temperatureVBO.UploadBufferIn(offset, 0, floatOutputDataBuffer, Volume::Chunk::CHUNK_SIZE_SQUARED);
            c->renderTemperatureVBO.UploadBufferIn(offset, 0, floatOutputDataBuffer, Volume::Chunk::CHUNK_SIZE_SQUARED);
        }

        floatOutputDataBuffer.ClearBuffer();
    }

    // Run pressure simulation
    float *pressureOutput = nullptr;
    if(GameEngine::instance->runPressureSimulation)
    {
        this->BindPressureShaderBuffers();
        this->pressureShader->Use();
        
        this->pressureShader->SetUnsignedInt("NumberOfVoxels", numberOfVoxels);

        this->pressureShader->Run(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, chunkCount);
        pressureOutput = floatOutputDataBuffer.ReadBuffer(numberOfVoxels);

        // Update the GPU buffer of the chunk
        for(uint16_t i = 0; i < chunksToUpdate.size(); ++i){
            unsigned int offset = i * Volume::Chunk::CHUNK_SIZE_SQUARED;
            Volume::Chunk *c = chunksToUpdate[i];

            c->pressureVBO.UploadBufferIn(offset, 0, floatOutputDataBuffer, Volume::Chunk::CHUNK_SIZE_SQUARED);
        }

        floatOutputDataBuffer.ClearBuffer();
    }

    // Run chemical simulation
    ChemicalVoxelChanges *reactionOutput = nullptr;
    uint32_t reactionsSize = 0;
    if(GameEngine::instance->runChemicalReactions)
    {
        chemicalOutputDataBuffer.ClearBuffer(numberOfVoxels, GL_DYNAMIC_DRAW);
        this->BindReactionShaderBuffers();
        this->reactionShader->Use();

        this->reactionShader->SetUnsignedInt("NumberOfVoxels", numberOfVoxels);
        uint32_t randomNumber = static_cast<uint32_t>(rand() % 1000);
        this->reactionShader->SetUnsignedInt("randomNumber", randomNumber);

        this->reactionShader->Run(Volume::Chunk::CHUNK_SIZE/8, Volume::Chunk::CHUNK_SIZE/4, chunkCount);

        // Read the output data
        uint32_t *reactionsSizePointer = atomicCounterBuffer.ReadBuffer();
        reactionsSize = *reactionsSizePointer;
        delete reactionsSizePointer;

        reactionOutput = chemicalOutputDataBuffer.ReadBuffer(reactionsSize);
    }

    // Apply the heat and pressure updates
    #pragma omp parallel for
    for (uint32_t i = 0; i < numberOfVoxels; i++) {
        uint16_t chunkIndex = i / Volume::Chunk::CHUNK_SIZE_SQUARED;
        uint16_t voxelIndex = i % Volume::Chunk::CHUNK_SIZE_SQUARED;	
        uint16_t x = voxelIndex % Volume::Chunk::CHUNK_SIZE;
        uint16_t y = voxelIndex / Volume::Chunk::CHUNK_SIZE;

        auto& chunk = chunksToUpdate[chunkIndex];
        if(heatOutput) chunk->voxels[y][x]->temperature = Volume::Temperature(heatOutput[i]);
        if(pressureOutput) chunk->voxels[y][x]->amount = pressureOutput[i];

        std::string newId = chunk->voxels[y][x]->ShouldTransitionToID();
        if(!newId.empty()){
            chunk->voxels[y][x]->DieAndReplace(chunkMatrix, newId);
        }
    }

    // faster lookup for offsets
    std::unordered_map<uint16_t, Vec2i> chunkIndexToPosOffset;
    for(uint16_t i = 0; i < static_cast<uint16_t>(chunksToUpdate.size()); ++i){
        chunkIndexToPosOffset[i] = chunksToUpdate[i]->GetPos() * Volume::Chunk::CHUNK_SIZE;
    }

    //Place voxels that underwent chemical reactions
    //#pragma omp parallel for ( crashes :( )
    for(uint32_t i = 0; i < reactionsSize; i++) {
        ChemicalVoxelChanges& change = reactionOutput[i];
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
    floatOutputDataBuffer.BindBufferBase(0);
    voxelTemperatureBuffer.BindBufferBase(1);
    voxelHeatCapacityBuffer.BindBufferBase(2);
    voxelConductivityBuffer.BindBufferBase(3);
    chunkConnectivityBuffer.BindBufferBase(4);
}

void Shader::ChunkShaderManager::BindPressureShaderBuffers()
{
    floatOutputDataBuffer.BindBufferBase(0);
    voxelIdBuffer.BindBufferBase(1);
    voxelPressureBuffer.BindBufferBase(2);
    chunkConnectivityBuffer.BindBufferBase(3);
}

void Shader::ChunkShaderManager::BindReactionShaderBuffers()
{
    atomicCounterBuffer.SetData(0, GL_DYNAMIC_COPY);

    chemicalOutputDataBuffer.BindBufferBase(0);
    atomicCounterBuffer.BindBufferBase(1);
    voxelTemperatureBuffer.BindBufferBase(2);
    voxelIdBuffer.BindBufferBase(3);
    Registry::VoxelRegistry::chemicalReactionsGLBuffer->BindBufferBase(4);
    chunkConnectivityBuffer.BindBufferBase(5);
}
