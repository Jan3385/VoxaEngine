#include "GLGroupStorageBuffer.h"

/// @brief Generates the data storage objects with an array of bools indicating if the segment is used
void Shader::GLGroupStorageBufferBase::GenerateDataStorage()
{
    this->DataStorage = new BufferDataStorage();
    this->DataStorage->SegmentBoolSSBO = GLBuffer<bool, GL_SHADER_STORAGE_BUFFER>(this->name + " Segment Bool SSBO");
    
    uint32_t numberOfSegments = this->totalSize / this->segmentSize;
    this->DataStorage->SegmentBoolSSBO.SetData(std::vector<bool>(numberOfSegments, false), GL_DYNAMIC_DRAW);
    this->DataStorage->NumberOfLinks++;
}

/// @brief Links the data storage of another GLGroupStorageBuffer
void Shader::GLGroupStorageBufferBase::LinkDataStorage(GLGroupStorageBufferBase &other)
{
    if(this->DataStorage || !other.DataStorage)
        throw std::runtime_error("[" + this->name + "] Cannot link data storage! Buffer already set or linking an unset buffer");

    this->DataStorage = other.DataStorage;
    this->DataStorage->NumberOfLinks++;
}

void Shader::GLGroupStorageBufferBase::Bind() const
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
}
void Shader::GLGroupStorageBufferBase::Unbind()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}