#pragma once

#include <SDL.h>

#include "Math/Vector.h"
#include "GameObject/VoxelObject.h"
#include "GameObject/PhysicsObject.h"
#include "World/ChunkMatrix.h"
#include "Physics/Physics.h"
#include "World/Voxel.h"

namespace Registry
{
    enum class GameObjectType{
        GameObject,         // Regular game object
        PhysicsObject,      // Moving, physical object
    };
    struct VoxelData{
        std::string id;
        RGBA color;
    };
    struct GameObjectProperty{
        GameObjectType type;
        int mass;

        std::vector<std::vector<VoxelData>> voxelData;

        uint32_t id = 0; // Unique identifier for the game object

        ~GameObjectProperty();
    };

    class GameObjectBuilder{
    public:
        GameObjectBuilder(GameObjectType objectType);
        GameObjectBuilder& SetMass(int mass);
        GameObjectBuilder& SetVoxelFileName(std::string fileName);
        GameObjectProperty Build();
    private:
        GameObjectType type;
        std::string voxelPath;
        int mass = 1;
    };

    void CreateGameObject(std::string id, Vec2f position, ChunkMatrix *matrix, GamePhysics *gamePhysics);

    class GameObjectRegistry{
    public:
        static GameObjectProperty* GetProperties(std::string id);
        static GameObjectProperty* GetProperties(uint32_t id);

        static void RegisterGameObject(const std::string &name, GameObjectProperty property);
        static void SetVoxelsFromFile(GameObjectProperty &property, const std::string &fileName);
        static void RegisterObjects();

        static std::string GetVoxelFromColorID(uint32_t colorId);

        static std::unordered_map<std::string, GameObjectProperty> registry;
		static std::unordered_map<uint32_t, GameObjectProperty*> idRegistry;
    private:
        static uint32_t idCounter;
		static bool registriesClosed;
    };
}
