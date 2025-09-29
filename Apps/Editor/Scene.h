#pragma once

#include <string>
#include <vector>
#include "Math/Vector.h"
#include "World/ChunkMatrix.h"

class EditorScene{
public:
    enum class Type{
        Unset = 0,
        ObjectEditor = 1,
        Sandbox = 2
    };

    EditorScene() = default;
    EditorScene(std::string name, Type type, ChunkMatrix* chunkMatrix, Vec2i chunkSize)
        : name(name), chunkMatrix(chunkMatrix), type(type) {
        this->chunkSize = chunkSize;
    }

    std::string name;
    ChunkMatrix* chunkMatrix = nullptr;

    virtual Type GetType() const { return this->type; }

    void SetNewChunkSize(const Vec2i& size) { this->chunkSize = size; }
    Vec2i GetChunkSize() const { return this->chunkSize; }
private:
    Type type = Type::Unset;
    Vec2i chunkSize = Vec2i(0, 0);
};

class ObjEditor : public EditorScene{
public:
    ObjEditor(std::string name, ChunkMatrix* chunkMatrix, Vec2i chunkSize)
        : EditorScene(name, Type::ObjectEditor, chunkMatrix, chunkSize) {}

    Type GetType() const override { return Type::ObjectEditor; }
private:

};