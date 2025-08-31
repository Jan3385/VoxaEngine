#pragma once

#include "Shader/Buffer/GLBuffer.h"
#include <queue>

namespace Shader{
    using StorageBufferTicket = uint32_t;

    template<typename T>
    class GLGroupStorageBuffer : public GLBufferBase{
    public:
        GLGroupStorageBuffer();
        GLGroupStorageBuffer(std::string name, uint32_t segmentSize, uint32_t maxExpectedElements, bool staticSize, GLenum usage);
        ~GLGroupStorageBuffer();

        void GenerateSegmentBoolBuffer();
        void LinkSegmentBoolBuffer(GLGroupStorageBuffer& other);

        // Disable copy
        GLGroupStorageBuffer(const GLGroupStorageBuffer&) = delete;
        GLGroupStorageBuffer& operator=(const GLGroupStorageBuffer&) = delete;
        // Movable
        GLGroupStorageBuffer(GLGroupStorageBuffer&& other) noexcept;
        GLGroupStorageBuffer& operator=(GLGroupStorageBuffer&& other) noexcept;

        StorageBufferTicket GenerateTicket();
        void DiscardTicket(StorageBufferTicket &ticket);
        static constexpr uint32_t TicketToIndex(const StorageBufferTicket ticket) { return ticket; }

        /// @brief Number of elements in a segment
        uint32_t GetSegmentSize() const { return segmentSize; }
        /// @brief Number of total elements
        uint32_t GetTotalSize() const { return totalSize; }

        void SetData(StorageBufferTicket ticket, const T* data);
        void SetData(StorageBufferTicket ticket, const std::vector<T>& data);
        void UpdateData(StorageBufferTicket ticket, GLuint offset, const std::vector<T>& data) const;
        void UpdateData(StorageBufferTicket ticket, GLuint offset, const T* data, GLuint size) const;

        void BindBufferBase(GLuint binding) const;
        void BindSegmentBoolBufferBase(GLuint binding) const;

        void ExpandBufferBySegments(uint32_t newSegments, GLenum usage);

        void ClearBuffer(const StorageBufferTicket ticket);

        void Bind() const;
        static void Unbind();
    private:
        struct SegmentBoolBufferStruct{
            GLBuffer<bool, GL_SHADER_STORAGE_BUFFER> SegmentBoolSSBO;
            uint32_t NumberOfLinks;
        };

        // A buffer of bools indicating availability of each segment/ticket
        SegmentBoolBufferStruct *SegmentBoolBuffer;

        bool staticSize;
        GLenum usage;

        GLuint TicketToOffset(const StorageBufferTicket ticket) const { return ticket * segmentSize * sizeof(T); }

        uint64_t totalSize = 0;
        const uint32_t segmentSize;

        uint32_t nextTicket = 0;
        std::priority_queue<
            uint32_t,
            std::vector<uint32_t>,
            std::greater<uint32_t>
        > freeTickets;
    };
}

#include "GLGroupStorageBuffer.inl"