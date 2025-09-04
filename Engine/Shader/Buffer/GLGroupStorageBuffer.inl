#include "GLGroupStorageBuffer.h"

#include <iostream>
#include <stack>

template <typename T>
Shader::GLGroupStorageBuffer<T>::GLGroupStorageBuffer()
    : staticSize(false), usage(GL_DYNAMIC_DRAW)
{
    this->ID = 0;
}
/// @brief Constructs a GLGroupStorageBuffer
/// @tparam T The type of data to store
/// @param name The name of the buffer
/// @param segmentSize The size of each segment (in number of elements)
/// @param maxExpectedSegments The maximum number of segments expected (if lower than actual max number of segments expect performance impacts)
/// @param staticSize determines if the buffer can reallocate to a new size. If true, throws an error during attempted reallocation
/// @param usage GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW,..
/// @example Storing 20 floats per segment and expecting 8 active segments at most at once:
/// T -> float; segmentSize = 20; maxExpectedSegments = 8;
template <typename T>
inline Shader::GLGroupStorageBuffer<T>::GLGroupStorageBuffer(std::string name, uint32_t segmentSize, uint32_t maxExpectedSegments, bool staticSize, GLenum usage)
    : staticSize(staticSize), usage(usage)
{
    this->name = name;
    this->segmentSize = segmentSize;
    this->totalSize = segmentSize * maxExpectedSegments;
    glGenBuffers(1, &ID);
    this->Bind();
    glObjectLabel(GL_BUFFER, ID, -1, this->name.c_str());
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize * sizeof(T), nullptr, usage);
    this->Unbind();
}
template <typename T>
inline Shader::GLGroupStorageBuffer<T>::~GLGroupStorageBuffer()
{
    if(ID != 0)
        glDeleteBuffers(1, &ID);
    ID = 0;

    if(!this->DataStorage) return;

    this->DataStorage->NumberOfLinks--;
    if(this->DataStorage->NumberOfLinks == 0)
    {
        delete this->DataStorage;
        this->DataStorage = nullptr;
    }
}

template <typename T>
inline Shader::GLGroupStorageBuffer<T>::GLGroupStorageBuffer(GLGroupStorageBuffer &&other) noexcept
{
    this->ID = other.ID;
    this->name = std::move(other.name);
    this->segmentSize = other.segmentSize;
    this->totalSize = other.totalSize;
    this->staticSize = other.staticSize;
    this->usage = other.usage;
    this->DataStorage = other.DataStorage;
    other.ID = 0;
    other.segmentSize = 0;
    other.totalSize = 0;
    other.staticSize = false;
    other.usage = GL_DYNAMIC_DRAW;
    other.DataStorage = nullptr;
}
template <typename T>
inline Shader::GLGroupStorageBuffer<T> &Shader::GLGroupStorageBuffer<T>::operator=(GLGroupStorageBuffer &&other) noexcept
{
    if (this != &other) {
        glDeleteBuffers(1, &ID);
        this->ID = other.ID;
        other.ID = 0;
        this->name = std::move(other.name);
        this->segmentSize = other.segmentSize;
        other.segmentSize = 0;
        this->totalSize = other.totalSize;
        other.totalSize = 0;
        this->staticSize = other.staticSize;
        this->usage = other.usage;
        this->DataStorage = other.DataStorage;
        other.DataStorage = nullptr;
    }
    return *this;
}

/// @brief Generates a ticket for using the storage buffer
/// @return the Ticket
template <typename T>
inline Shader::StorageBufferTicket Shader::GLGroupStorageBuffer<T>::GenerateTicket()
{
    this->AssertDataStorageGenerated();

    if(!this->DataStorage->freeTickets.empty()){
        uint32_t ticket = this->DataStorage->freeTickets.top();
        this->DataStorage->freeTickets.pop();

        bool isNowUsed = true;
        this->DataStorage->SegmentBoolSSBO.UpdateData(ticket, &isNowUsed, 1);
        return ticket;
    }

    uint32_t newTicket = this->DataStorage->nextTicket++;

    if(newTicket >= this->GetNumberOfSegments())
        this->ExpandBufferBySegments(1, this->usage);
    
    bool isNowUsed = true;
    this->DataStorage->SegmentBoolSSBO.UpdateData(newTicket, &isNowUsed, 1);

    return newTicket;
}

/// @brief Discards a previously generated ticket
/// @param ticket The ticket to discard
template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::DiscardTicket(StorageBufferTicket &ticket)
{
    this->AssertDataStorageGenerated();

    if(ticket == InvalidTicket) return;
    if(ticket >= this->DataStorage->nextTicket) return; // Invalid ticket

    this->DataStorage->freeTickets.push(ticket);

    bool isNowUsed = false;
    this->DataStorage->SegmentBoolSSBO.UpdateData(ticket, &isNowUsed, 1);

    ticket = InvalidTicket;
}

template <typename T>
inline uint32_t Shader::GLGroupStorageBuffer<T>::GetNumOfUsedTickets() const
{
    this->AssertDataStorageGenerated();
    uint32_t start = this->DataStorage->nextTicket;

    // Go down for each unused ticket to find the first used one
    std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> freeTicketsCopy = this->DataStorage->freeTickets;
    while (!freeTicketsCopy.empty() && freeTicketsCopy.top() == start - 1) {
        start--;
        freeTicketsCopy.pop();
    }
    return start;
}

/// @brief Sets the data for a specific segment. Data must have the same size as segment size
/// @param ticket The ticket for the segment
/// @param data The array of data to set
/// @warning No automated checks for array size to exactly match segment size
template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::SetData(StorageBufferTicket ticket, const T *data)
{
    uint32_t offset = TicketToOffset(ticket);
    this->Bind();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, segmentSize * sizeof(T), data);
    this->Unbind();
}

/// @brief Sets the data for a specific segment. Data must have the same size as segment size
/// @param ticket The ticket for the segment
/// @param data The vector of data to set
/// @throw std::invalid_argument if data size does not match segment size
template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::SetData(StorageBufferTicket ticket, const std::vector<T> &data)
{
    if (data.size() != segmentSize) {
        throw std::invalid_argument("[" + this->name + "] Data size must match segment size");
    }

    uint32_t offset = TicketToOffset(ticket);
    this->Bind();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, segmentSize * sizeof(T), data.data());
    this->Unbind();
}

/// @brief Updates a portion of a specific segment. Data must not go over segment size
/// @param ticket The ticket for the segment
/// @param offset The offset within the segment
/// @param data The vector of data to update
/// @throw std::invalid_argument if data size + offset exceeds segment size
template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::UpdateData(StorageBufferTicket ticket, GLuint offset, const std::vector<T> &data) const
{
    if (data.size() + offset > segmentSize) {
        throw std::invalid_argument("[" + this->name + "] Data size + offset must not exceed segment size (VEC)");
    }

    uint32_t baseOffset = TicketToOffset(ticket);
    this->Bind();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + offset, data.size() * sizeof(T), data.data());
    this->Unbind();
}

/// @brief Updates a portion of a specific segment. Data must not go over segment size
/// @param ticket The ticket for the segment
/// @param offset The offset within the segment
/// @param data The array of data to update
/// @param size The size of the array
/// @throw std::invalid_argument if data size + offset exceeds segment size
template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::UpdateData(StorageBufferTicket ticket, GLuint offset, const T *data, GLuint size) const
{
    if (size + offset > segmentSize) {
        throw std::invalid_argument("[" + this->name + "] Data size + offset must not exceed segment size (ARR)");
    }

    uint32_t baseOffset = TicketToOffset(ticket);
    this->Bind();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, baseOffset + offset, size * sizeof(T), data);
    this->Unbind();
}

template <typename T>
template <GLenum target>
inline void Shader::GLGroupStorageBuffer<T>::UploadBufferIn(
    GLuint copyOffset, GLuint writeOffset,
    GLBuffer<T, target> &buffer, GLuint size) const
{
    if(size == 0){
        size = buffer.bufferSize;
    }

    if (copyOffset + size > static_cast<GLuint>(buffer.bufferSize)) {
        std::cerr << "[" << this->name << "] GLBuffer::UploadBufferIn - Error: Attempt to upload buffer data out of bounds!(source)" << std::endl;
        copyOffset = buffer.bufferSize - size;
    }
    if (writeOffset + size > static_cast<GLuint>(this->totalSize)) {
        std::cerr << "[" << this->name << "] GLBuffer::UploadBufferIn - Error: Attempt to upload buffer data out of bounds!(destination)" << std::endl;
    }

    glBindBuffer(GL_COPY_READ_BUFFER, buffer.ID);
    glBindBuffer(GL_COPY_WRITE_BUFFER, this->ID);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, copyOffset * sizeof(T), writeOffset * sizeof(T), size * sizeof(T));
    glBindBuffer(GL_COPY_READ_BUFFER, 0);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}

template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::BindBufferBase(GLuint binding) const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ID);
}

/// @brief Bind an availability buffer to a binding point (a buffer of bools that indicates if the segment is active)
/// @param binding The binding point
template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::BindSegmentBoolBufferBase(GLuint binding) const
{
    this->AssertDataStorageGenerated();

    this->DataStorage->SegmentBoolSSBO.BindBufferBase(binding);
}

/// @brief Creates a new buffer with a bigger size, copying all data from the old one
/// @param newSegments The number of segments to add
/// @warning This will unbind the previous buffer since new one is created and old one deleted
/// @note This operation may be expensive and should be avoided if possible by increasing `maxExpectedSegments`
/// @throws std::length_error if the buffer is static size
template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::ExpandBufferBySegments(uint32_t newSegments, GLenum usage)
{
    if(this->staticSize)
        throw std::length_error("[" + this->name + "] Buffer is static size and cannot be expanded by " + std::to_string(newSegments) + " segments.");
    
    this->totalSize += newSegments * segmentSize;

    GLuint newSSBO;
    glGenBuffers(1, &newSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, newSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize * sizeof(T), nullptr, usage);

    glBindBuffer(GL_COPY_READ_BUFFER, ID);
    glBindBuffer(GL_COPY_WRITE_BUFFER, newSSBO);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, totalSize * sizeof(T));
    glBindBuffer(GL_COPY_READ_BUFFER, 0);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

    glDeleteBuffers(1, &ID);
    ID = newSSBO;

    if(!this->DataStorage) return;
    uint32_t numberOfSegments = this->totalSize / segmentSize;

    if(this->DataStorage->SegmentBoolSSBO.GetSize() == numberOfSegments) return; // Already the right size

    GLBuffer<bool, GL_SHADER_STORAGE_BUFFER> newSegmentBoolSSBO(this->name + " Segment Bool SSBO");

    newSegmentBoolSSBO.SetData(std::vector<bool>(numberOfSegments, false), GL_DYNAMIC_DRAW);
    newSegmentBoolSSBO.UploadBufferIn(0, 0, this->DataStorage->SegmentBoolSSBO, 0);

    this->DataStorage->SegmentBoolSSBO = std::move(newSegmentBoolSSBO);
}

template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::ClearBuffer(const StorageBufferTicket ticket)
{
    uint32_t offset = TicketToOffset(ticket);
    std::vector<T> zeros(segmentSize, T{});

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, segmentSize * sizeof(T), zeros.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

template <typename T>
inline void Shader::GLGroupStorageBuffer<T>::AssertDataStorageGenerated() const
{
    if (!this->DataStorage)
        throw std::runtime_error("[" + this->name + "] Data storage not generated! Have you forgot to use GenerateDataStorage or LinkDataStorage?");
}
