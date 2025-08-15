#include "Registry/GameObjectRegistry.h"
#include "GameObjectRegistry.h"
#include "Physics/Physics.h"

#include <iostream>

using namespace Registry;

std::unordered_map<std::string, GameObjectProperty> GameObjectRegistry::registry = {};
std::unordered_map<uint32_t, GameObjectProperty*> GameObjectRegistry::idRegistry = {};
uint32_t GameObjectRegistry::idCounter = 1;
bool GameObjectRegistry::registryClosed = false;

void GameObjectRegistry::RegisterGameObject(const std::string &name, GameObjectProperty property)
{
    if (registryClosed) 
        throw std::runtime_error("GameObject registered after designated time window: " + name);
    
    property.id = ++idCounter;

    // Generate mesh and voxel 2D vectors for gameobject

    registry[name] = property;

    idRegistry[property.id] = &property;
}
void Registry::GameObjectRegistry::SetVoxelsFromFile(GameObjectProperty &property, const std::string &fileName)
{
    const std::string voxelImage = "Textures/Objects/" + fileName + ".bmp";
    const std::string materialImage = "Textures/Objects/" + fileName + ".mat.bmp";

    SDL_Surface *colorSurface = SDL_LoadBMP(voxelImage.c_str());
    SDL_Surface *materialSurface = SDL_LoadBMP(materialImage.c_str());
    
    if (!colorSurface || !materialSurface) {
        throw std::runtime_error("[GameObjectRegistry] Failed to load color or material image for file named: " + fileName);
    }
    else if (colorSurface->w != materialSurface->w || colorSurface->h != materialSurface->h) {
        throw std::runtime_error("[GameObjectRegistry] Color and material images must have the same dimensions for file named: " + fileName);
    }
    else if (colorSurface->format->format != SDL_PIXELFORMAT_ARGB8888 || materialSurface->format->format != SDL_PIXELFORMAT_ARGB8888) {
        throw std::runtime_error("[GameObjectRegistry] Color and material images must be in ARGB8888 format for file named: " + fileName);
    }

    property.voxelData.resize(colorSurface->h, std::vector<VoxelData>(colorSurface->w));
    for (int y = 0; y < colorSurface->h; ++y) {
        for (int x = 0; x < colorSurface->w; ++x) {
            Uint32 colorPixel = ((Uint32*)colorSurface->pixels)[y * colorSurface->w + x];
            Uint32 materialPixel = ((Uint32*)materialSurface->pixels)[y * materialSurface->w + x];

            RGBA color = RGBA(colorPixel);

            if(color.a == 0) {
                property.voxelData[y][x] = { "", color }; // Transparent pixel
                continue; // Skip transparent pixels
            }

            uint32_t materialId = materialPixel;

            VoxelData data = {
                .id = GetVoxelFromColorID(materialId),
                .color = color
            };

            property.voxelData[y][x] = data;
        }
    }

    SDL_FreeSurface(colorSurface);
    SDL_FreeSurface(materialSurface);
}
void GameObjectRegistry::RegisterObjects()
{
    std::cout << "Registering game objects ";
    GameObjectRegistry::RegisterGameObject(
        "Player",
        GameObjectBuilder(GameObjectType::PhysicsObject)
            .SetDensityOverride(985.0f)
            .SetVoxelFileName("Player")
            .Build()
    );
    GameObjectRegistry::RegisterGameObject(
        "Barrel",
        GameObjectBuilder(GameObjectType::PhysicsObject)
            .SetDensityOverride(400.0f)
            .SetVoxelFileName("Barrel")
            .Build()
    );
    GameObjectRegistry::RegisterGameObject(
        "Ball",
        GameObjectBuilder(GameObjectType::PhysicsObject)
            .SetVoxelFileName("Ball")
            .Build()
    );
    GameObjectRegistry::RegisterGameObject(
        "Crate",
        GameObjectBuilder(GameObjectType::GameObject)
            .SetVoxelFileName("Crate")
            .Build()
    );

    std::cout << "[ OK ]" << std::endl;
}

void Registry::GameObjectRegistry::CloseRegistry()
{
    if (registryClosed)
        throw std::runtime_error("Registry already closed");

    registryClosed = true;
}

// Color Pallete:
// 255, 0, 0        -> metal
// 255, 255, 0      -> gold
// 150, 75, 0       -> wood
// 0, 255, 0        -> rust
// 0, 0, 255        -> stone
// 255, 255, 255    -> glass
// 0, 0, 0          -> charcoal
// 255, 165, 0      -> organics

std::string Registry::GameObjectRegistry::GetVoxelFromColorID(uint32_t colorId)
{
    switch (colorId) {
        // Format: 0xAARRGGBB
        case 0xFFFF0000: return "Iron";         // Solid red    -> metal
        case 0xFFFFFF00: return "Gold";          // Solid yellow -> gold
        case 0xFF964B00: return "Wood";         // Brown        -> wood
        case 0xFF00FF00: return "Rust";         // Solid green  -> rust
        case 0xFF0000FF: return "Stone";        // Solid blue   -> stone
        case 0xFFFFFFFF: return "Glass";        // Solid white  -> glass
        case 0xFF000000: return "Charcoal";     // Solid black  -> charcoal
        case 0xFFFFA500: return "Organics";     // Orange       -> organic
        default: throw std::runtime_error("Unknown color ID: " + std::to_string(colorId));
    }
}

void Registry::CreateGameObject(std::string id, Vec2f position, ChunkMatrix *matrix, GamePhysics *gamePhysics)
{
    GameObjectProperty *property = GameObjectRegistry::GetProperties(id);
    if (!property) {
        throw std::runtime_error("GameObject with id " + id + " not found in registry.");
    }

    if (property->type == GameObjectType::GameObject) {
        VoxelObject *gameObject = new VoxelObject(position, property->voxelData, id);
        matrix->voxelObjects.push_back(gameObject);

    } else if (property->type == GameObjectType::PhysicsObject) {
        PhysicsObject *physicsObject = new PhysicsObject(position, property->voxelData, property->densityOverride, id);
        matrix->voxelObjects.push_back(physicsObject);
        gamePhysics->physicsObjects.push_back(physicsObject);
    }
}

GameObjectBuilder::GameObjectBuilder(GameObjectType objectType)
    : type(objectType) { }

// density in KG/m^3
GameObjectBuilder &GameObjectBuilder::SetDensityOverride(float density)
{
    this->densityOverride = density;
    return *this;
}

GameObjectBuilder &GameObjectBuilder::SetVoxelFileName(std::string fileName)
{
    this->voxelPath = fileName;
    return *this;
}
GameObjectProperty GameObjectBuilder::Build()
{
    GameObjectProperty property;
    property.type = this->type;
    property.densityOverride = this->densityOverride;

    GameObjectRegistry::SetVoxelsFromFile(property, this->voxelPath);

    return property;
}

Registry::GameObjectProperty::~GameObjectProperty()
{
    
}

GameObjectProperty* GameObjectRegistry::GetProperties(std::string id){
    auto it = registry.find(id);
    if (it != registry.end()) {
        return &it->second;
    }
    return nullptr;
}
GameObjectProperty* GameObjectRegistry::GetProperties(uint32_t id){
    auto it = idRegistry.find(id);
    if (it != idRegistry.end()) {
        return it->second;
    }
    return nullptr;
}