#include "Registry/VoxelObjectRegistry.h"
#include "VoxelObjectRegistry.h"
#include "Physics/Physics.h"
#include "GameEngine.h"

#include <iostream>

using namespace Registry;

std::unordered_map<std::string, VoxelObjectProperty> VoxelObjectRegistry::registry = {};
std::unordered_map<uint32_t, VoxelObjectProperty*> VoxelObjectRegistry::idRegistry = {};
std::unordered_map<std::string, VoxelObjectFactory> VoxelObjectRegistry::voxelObjectFactories = {};
uint32_t VoxelObjectRegistry::idCounter = 1;
bool VoxelObjectRegistry::registryClosed = false;

void VoxelObjectRegistry::RegisterVoxelObject(const std::string &name, VoxelObjectProperty property)
{
    if (registryClosed) 
        throw std::runtime_error("VoxelObject registered after designated time window: " + name);
    
    property.id = ++idCounter;

    // Generate mesh and voxel 2D vectors for VoxelObject

    registry[name] = property;

    idRegistry[property.id] = &property;
}
void Registry::VoxelObjectRegistry::RegisterVoxelObjectFactory(const std::string &name, VoxelObjectFactory factory)
{
    if(registryClosed)
        throw std::runtime_error("VoxelObject factory registered after designated time window: " + name);
    
    voxelObjectFactories[name] = factory;
}
void Registry::VoxelObjectRegistry::SetVoxelsFromFile(VoxelObjectProperty &property, const std::string &fileName)
{
    const std::string voxelImage = "Textures/Objects/" + fileName + ".bmp";
    const std::string materialImage = "Textures/Objects/" + fileName + ".mat.bmp";

    SDL_Surface *colorSurface = SDL_LoadBMP(voxelImage.c_str());
    SDL_Surface *materialSurface = SDL_LoadBMP(materialImage.c_str());
    
    if (!colorSurface || !materialSurface) {
        throw std::runtime_error("[VoxelObjectRegistry] Failed to load color or material image for file named: " + fileName);
    }
    else if (colorSurface->w != materialSurface->w || colorSurface->h != materialSurface->h) {
        throw std::runtime_error("[VoxelObjectRegistry] Color and material images must have the same dimensions for file named: " + fileName);
    }
    else if (colorSurface->format->format != SDL_PIXELFORMAT_ARGB8888 || materialSurface->format->format != SDL_PIXELFORMAT_ARGB8888) {
        throw std::runtime_error("[VoxelObjectRegistry] Color and material images must be in ARGB8888 format for file named: " + fileName);
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
VoxelObjectFactory *Registry::VoxelObjectRegistry::FindFactoryWithID(std::string id)
{
    auto it = voxelObjectFactories.find(id);
    if (it != voxelObjectFactories.end()) {
        return &it->second;
    }
    return nullptr;
}
void VoxelObjectRegistry::RegisterObjects(IGame *game)
{
    std::cout << "Registering game objects ";

    game->RegisterVoxelObjects();

    std::cout << "[ \033[32mOK\033[0m ]" << std::endl;
}

void Registry::VoxelObjectRegistry::CloseRegistry()
{
    if (registryClosed) return;

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

std::string Registry::VoxelObjectRegistry::GetVoxelFromColorID(uint32_t colorId)
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

void Registry::CreateVoxelObject(std::string id, Vec2f position, ChunkMatrix *matrix, GamePhysics *gamePhysics)
{
    VoxelObjectProperty *property = VoxelObjectRegistry::GetProperties(id);
    if (!property)
        throw std::runtime_error("VoxelObject with id " + id + " not found in registry.");

    if (property->type == VoxelObjectType::VoxelObject) {
        VoxelObject *voxelObj = new VoxelObject(position, property->voxelData, id);
        matrix->voxelObjects.push_back(voxelObj);

    } else if (property->type == VoxelObjectType::PhysicsObject) {
        PhysicsObject *physicsObj = new PhysicsObject(position, property->voxelData, property->densityOverride, id);
        matrix->voxelObjects.push_back(physicsObj);
        gamePhysics->physicsObjects.push_back(physicsObj);
    } else{
        VoxelObject *voxelObj = nullptr;

        std::string factoryID = property->specialFactoryID.empty() ? id : property->specialFactoryID;

        auto factory = VoxelObjectRegistry::FindFactoryWithID(factoryID);

        if(!factory)
            throw std::runtime_error("No factory found for custom game object with id: " + factoryID);

        voxelObj = (*factory)(position, property->voxelData, id);
    }
}

VoxelObjectBuilder::VoxelObjectBuilder(VoxelObjectType objectType)
    : type(objectType) { }

// density in KG/m^3
VoxelObjectBuilder &VoxelObjectBuilder::SetDensityOverride(float density)
{
    this->densityOverride = density;
    return *this;
}

VoxelObjectBuilder &VoxelObjectBuilder::SetVoxelFileName(std::string fileName)
{
    this->voxelPath = fileName;
    return *this;
}
VoxelObjectBuilder &Registry::VoxelObjectBuilder::SpecialFactoryOverride(std::string factoryID)
{
    this->specialFactoryID = factoryID;
    return *this;
}
VoxelObjectProperty VoxelObjectBuilder::Build()
{
    VoxelObjectProperty property;
    property.type = this->type;
    property.densityOverride = this->densityOverride;
    property.specialFactoryID = this->specialFactoryID;

    VoxelObjectRegistry::SetVoxelsFromFile(property, this->voxelPath);

    return property;
}

Registry::VoxelObjectProperty::~VoxelObjectProperty()
{
    
}

VoxelObjectProperty* VoxelObjectRegistry::GetProperties(std::string id){
    auto it = registry.find(id);
    if (it != registry.end()) {
        return &it->second;
    }
    return nullptr;
}
VoxelObjectProperty* VoxelObjectRegistry::GetProperties(uint32_t id){
    auto it = idRegistry.find(id);
    if (it != idRegistry.end()) {
        return it->second;
    }
    return nullptr;
}