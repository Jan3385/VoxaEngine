#pragma once

#include <SDL.h>

#include "Math/Vector.h"
#include "VoxelObject/VoxelObject.h"
#include "VoxelObject/PhysicsObject.h"
#include "World/ChunkMatrix.h"
#include "Physics/Physics.h"
#include "World/Voxel.h"

struct IGame;

namespace Registry
{
    enum class VoxelObjectType{
        VoxelObject,         // Regular voxel object
        PhysicsObject,      // Moving, physical object
        Custom              // Uses custom constructor
    };
    struct VoxelData{
        std::string id;
        RGBA color;
    };
    struct VoxelObjectProperty{
        VoxelObjectType type;
        float densityOverride;  // 0 = no override, KG/m^3

        std::vector<std::vector<VoxelData>> voxelData;

        std::string specialFactoryID; // empty string = no factory override
        uint32_t id = 0; // Unique identifier for the game object

        ~VoxelObjectProperty();
    };

    class VoxelObjectBuilder{
    public:
        VoxelObjectBuilder(VoxelObjectType objectType);
        VoxelObjectBuilder& SetDensityOverride(float density);
        VoxelObjectBuilder& SetVoxelFileName(std::string fileName);
        VoxelObjectBuilder& SpecialFactoryOverride(std::string factoryID);
        VoxelObjectProperty Build();
    private:
        VoxelObjectType type;
        std::string voxelPath;
        float densityOverride = 0.0f;
        std::string specialFactoryID = "";
    };

    void CreateVoxelObject(std::string id, Vec2f position, ChunkMatrix *matrix, GamePhysics *gamePhysics);

    using VoxelObjectFactory = std::function<VoxelObject*(Vec2f position, const std::vector<std::vector<VoxelData>>& voxelData, std::string name)>;

    class VoxelObjectRegistry{
    public:
        static VoxelObjectProperty* GetProperties(std::string id);
        static VoxelObjectProperty* GetProperties(uint32_t id);

        static void RegisterVoxelObject(const std::string &name, VoxelObjectProperty property);
        static void RegisterVoxelObjectFactory(const std::string &name, VoxelObjectFactory factory);
        static void SetVoxelsFromFile(VoxelObjectProperty &property, const std::string &fileName);

        static VoxelObjectFactory *FindFactoryWithID(std::string id);

        static void RegisterObjects(IGame *game);
        static void CloseRegistry();

        static std::string GetVoxelFromColorID(uint32_t colorId);

        static std::unordered_map<std::string, VoxelObjectProperty> registry;
		static std::unordered_map<uint32_t, VoxelObjectProperty*> idRegistry;
    private:
        static uint32_t idCounter;
		static bool registryClosed;

        static std::unordered_map<std::string, VoxelObjectFactory> voxelObjectFactories;
    };
}
