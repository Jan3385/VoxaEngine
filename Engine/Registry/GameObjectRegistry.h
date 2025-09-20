#pragma once

#include <SDL.h>

#include "Math/Vector.h"
#include "GameObject/VoxelObject.h"
#include "GameObject/PhysicsObject.h"
#include "World/ChunkMatrix.h"
#include "Physics/Physics.h"
#include "World/Voxel.h"

struct IGame;

namespace Registry
{
    enum class GameObjectType{
        GameObject,         // Regular game object
        PhysicsObject,      // Moving, physical object
        Custom              // Uses custom constructor
    };
    struct VoxelData{
        std::string id;
        RGBA color;
    };
    struct GameObjectProperty{
        GameObjectType type;
        float densityOverride;  // 0 = no override, KG/m^3

        std::vector<std::vector<VoxelData>> voxelData;

        std::string specialFactoryID; // empty string = no factory override
        uint32_t id = 0; // Unique identifier for the game object

        ~GameObjectProperty();
    };

    class GameObjectBuilder{
    public:
        GameObjectBuilder(GameObjectType objectType);
        GameObjectBuilder& SetDensityOverride(float density);
        GameObjectBuilder& SetVoxelFileName(std::string fileName);
        GameObjectBuilder& SpecialFactoryOverride(std::string factoryID);
        GameObjectProperty Build();
    private:
        GameObjectType type;
        std::string voxelPath;
        float densityOverride = 0.0f;
        std::string specialFactoryID = "";
    };

    void CreateGameObject(std::string id, Vec2f position, ChunkMatrix *matrix, GamePhysics *gamePhysics);

    using VoxelObjectFactory = std::function<VoxelObject*(Vec2f position, const std::vector<std::vector<VoxelData>>& voxelData, std::string name)>;

    class GameObjectRegistry{
    public:
        static GameObjectProperty* GetProperties(std::string id);
        static GameObjectProperty* GetProperties(uint32_t id);

        static void RegisterGameObject(const std::string &name, GameObjectProperty property);
        static void RegisterGameObjectFactory(const std::string &name, VoxelObjectFactory factory);
        static void SetVoxelsFromFile(GameObjectProperty &property, const std::string &fileName);

        static VoxelObjectFactory *FindFactoryWithID(std::string id);

        static void RegisterObjects(IGame *game);
        static void CloseRegistry();

        static std::string GetVoxelFromColorID(uint32_t colorId);

        static std::unordered_map<std::string, GameObjectProperty> registry;
		static std::unordered_map<uint32_t, GameObjectProperty*> idRegistry;
    private:
        static uint32_t idCounter;
		static bool registryClosed;

        static std::unordered_map<std::string, VoxelObjectFactory> voxelObjectFactories;
    };
}
