#include "Registry/VoxelRegistry.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>

#include "World/Voxels/Empty.h"
#include "VoxelObjectRegistry.h"
#include "VoxelRegistry.h"
#include "GameEngine.h"

using namespace Volume;
using namespace Registry;

// Initialize the voxel properties

std::unordered_map<std::string, VoxelProperty> VoxelRegistry::registry = {};
std::unordered_map<uint32_t, VoxelProperty*> VoxelRegistry::idRegistry = {};
std::unordered_map<std::string, VoxelFactory> VoxelRegistry::voxelFactories = {};
std::unordered_map<std::string, VoxelTextureMap*> VoxelRegistry::textureMaps = {};
std::vector<Registry::ChemicalReaction> VoxelRegistry::reactionRegistry = {};
Shader::GLBuffer<Registry::VoxelRegistry::ChemicalReactionGL, GL_SHADER_STORAGE_BUFFER>* 
	Registry::VoxelRegistry::chemicalReactionsGLBuffer = nullptr;

uint32_t VoxelRegistry::idCounter = 1;
bool VoxelRegistry::registryClosed = false;

void VoxelRegistry::RegisterVoxel(const std::string &id, VoxelProperty property)
{
	if(registryClosed) 
		throw std::runtime_error("Voxel registered after designated time window: " + id);
	
	property.id = ++idCounter;

	registry[id] = property;

	idRegistry[property.id] = &registry[id];
}

void Registry::VoxelRegistry::RegisterVoxelFactory(const std::string &name, VoxelFactory factory)
{
	if(registryClosed) 
		throw std::runtime_error("Voxel factory registered after designated time window: " + name);

	voxelFactories[name] = factory;
}
void Registry::VoxelRegistry::RegisterTextureMap(const std::string &name, const std::string &texturePath, TextureRotation possibleRotations)
{
	if(registryClosed) 
		throw std::runtime_error("Texture map registered after designated time window: " + name);

	VoxelTextureMap* map = new VoxelTextureMap(texturePath, possibleRotations);
	VoxelRegistry::textureMaps[name] = map;
}

void Registry::VoxelRegistry::RegisterReaction(Registry::ChemicalReaction reaction)
{
	if (registryClosed)
		throw std::runtime_error("Reaction registered after designated time window");

	Registry::VoxelRegistry::reactionRegistry.push_back(reaction);
}

void VoxelRegistry::RegisterVoxels(IGame *game)
{
	std::cout << "Registering voxels ";
	using namespace Volume;
	VoxelRegistry::RegisterVoxel(
		"Empty",
		VoxelBuilder(DefaultVoxelConstructor::Custom, 0, 0, 0)
			.SetName("Vacuum")
			.SetColor(RGBA(0, 0, 0, 0))
			.SetFluidDispersionRate(0)
			.Build()
	);
	VoxelRegistry::RegisterVoxelFactory(
		"Empty",
		[](Vec2i position, Volume::Temperature temp, float amount, bool placeUnmovableSolids) {
			return new EmptyVoxel(position);
		}
	);

	game->RegisterVoxels();

	std::cout << "[ \033[32mOK\033[0m ]" << std::endl;
}

void Registry::VoxelRegistry::CloseRegistry()
{
	if(registryClosed) return;

	registryClosed = true;

	// get IDs for chemical reactions
	std::vector<ChemicalReactionGL> reactions;
	for(ChemicalReaction reaction : VoxelRegistry::reactionRegistry){
		uint32_t from = VoxelRegistry::GetProperties(reaction.from)->id;
		uint32_t catalyst = VoxelRegistry::GetProperties(reaction.catalyst)->id;
		uint32_t to = VoxelRegistry::GetProperties(reaction.to)->id;

		reactions.push_back({
			from,
			catalyst,
			to,
			reaction.reactionSpeed,
			reaction.preserveCatalyst,
			reaction.minTemperatureC
		});

		VoxelProperty *prop = VoxelRegistry::GetProperties(reaction.from);
		if(!prop) continue;

		prop->Reactions.push_back({
			catalyst,
			to,
			reaction.reactionSpeed,
			reaction.preserveCatalyst,
			Temperature(reaction.minTemperatureC)
		});
	}

	// sort reactions by "from" ID to be able to quickly search them thru later on the GPU
	std::sort(reactions.begin(), reactions.end(), [](const ChemicalReactionGL& a, const ChemicalReactionGL& b) {
		return a.fromID < b.fromID;
	});

	// Upload chemical reactions to a GPU buffer
	VoxelRegistry::chemicalReactionsGLBuffer = new Shader::GLBuffer<ChemicalReactionGL, GL_SHADER_STORAGE_BUFFER>("Chemical Reactions Buffer");
	VoxelRegistry::chemicalReactionsGLBuffer->SetData(reactions, GL_STATIC_DRAW);

	// Clear reaction registry to free up memory
	VoxelRegistry::reactionRegistry.clear();
	std::vector<ChemicalReactionGL>().swap(reactions); // free memory
}

VoxelFactory *Registry::VoxelRegistry::FindFactoryWithID(std::string id)
{
    auto it = VoxelRegistry::voxelFactories.find(id);
    if (it != VoxelRegistry::voxelFactories.end()) {
        return &it->second;
    }
    return nullptr;
}

/// @brief Prevents memory leaks at the end of the application
void Registry::VoxelRegistry::CleanupRegistry()
{
	for(auto& pair : VoxelRegistry::textureMaps){
		delete pair.second;
	}
	VoxelRegistry::textureMaps.clear();

	delete VoxelRegistry::chemicalReactionsGLBuffer;
}

VoxelProperty* VoxelRegistry::GetProperties(std::string id)
{
    auto it = VoxelRegistry::registry.find(id);
	if(it == VoxelRegistry::registry.end()){
		throw std::runtime_error("Voxel property not found for character id: " + id);
	}
	return &it->second;
}

VoxelProperty *VoxelRegistry::GetProperties(uint32_t id)
{
    auto it = VoxelRegistry::idRegistry.find(id);

	if(it == VoxelRegistry::idRegistry.end()){
		throw std::runtime_error("Voxel property not found for numeric id: " + id);
	}

	return it->second;
}

std::string Registry::VoxelRegistry::GetStringID(uint32_t numericId)
{
	VoxelProperty* it = VoxelRegistry::idRegistry.find(numericId)->second;
	if(it == nullptr){
		throw std::runtime_error("Voxel property not found for numeric id: " + std::to_string(numericId));
	}

	for(const auto& pair : VoxelRegistry::registry){
		if(pair.second.id == numericId){
			return pair.first;
		}
	}

	throw std::runtime_error("Voxel string ID not found for numeric id: " + std::to_string(numericId));
}

bool VoxelRegistry::CanGetMovedByExplosion(State state)
{
    return state == State::Liquid || state == State::Solid;
}

bool VoxelRegistry::CanGetDestroyedByExplosion(std::string id, float explosionPower)
{
    return true;
}

bool VoxelRegistry::CanBeMovedBySolid(State state)
{
    return state == State::Gas || state == State::Liquid;
}

bool VoxelRegistry::CanBeMovedByLiquid(State state)
{
    return state == State::Gas;
}

VoxelBuilder::VoxelBuilder(DefaultVoxelConstructor State, float tCapacity, float tConductivity, float Density)
{
	this->Constructor = State;
	this->HeatCapacity = tCapacity;
	this->HeatConductivity = tConductivity;
	this->Density = Density;
}

VoxelBuilder &VoxelBuilder::SetName(std::string Name)
{
    this->Name = Name;
	return *this;
}

VoxelBuilder &VoxelBuilder::SetColor(RGBA Color)
{
    this->Color = Color;
	return *this;
}

VoxelBuilder &VoxelBuilder::PhaseUp(std::string To, Volume::Temperature Temperature)
{
	this->HeatedChange = { Temperature, To };
	return *this;
}

VoxelBuilder &VoxelBuilder::PhaseDown(std::string To, Volume::Temperature Temperature)
{
	this->CooledChange = { Temperature, To };
	return *this;
}

VoxelBuilder &Registry::VoxelBuilder::Reaction(std::string To, std::string Catalyst, float ReactionSpeed, bool PreserveCatalyst, Volume::Temperature MinTemperature)
{
	VoxelRegistry::reactionRegistry.push_back({ 
		this->Name, 
		Catalyst, 
		To, 
		ReactionSpeed, 
		PreserveCatalyst, 
		MinTemperature.GetCelsius() 
	});

	return *this;
}

VoxelBuilder &Registry::VoxelBuilder::ReactionOxidation(std::string To, float OxygenReactionSpeed)
{
	// rusting is minimal at temperatures below freezing point
	Temperature minOxidationTemp(0);


	VoxelRegistry::reactionRegistry.push_back({ 
		this->Name, 
		"Oxygen", 
		To, 
		OxygenReactionSpeed, 
		true,
		minOxidationTemp.GetCelsius()
	});
	VoxelRegistry::reactionRegistry.push_back({ 
		this->Name, 
		"Water", 
		To, 
		OxygenReactionSpeed * 100, 
		true,
		minOxidationTemp.GetCelsius()
	});
	VoxelRegistry::reactionRegistry.push_back({ 
		this->Name, 
		"Steam", 
		To, 
		OxygenReactionSpeed * 300, 
		true,
		minOxidationTemp.GetCelsius()
	});

	// if "hot" enough, achieves extremely fast oxidation
	VoxelRegistry::reactionRegistry.push_back({ 
		this->Name, 
		"Liquid_Oxygen", 
		To, 
		OxygenReactionSpeed * 1000, 
		true,
		minOxidationTemp.GetCelsius()
	});
	
    return *this;
}

VoxelBuilder &VoxelBuilder::VoxelTextureMap(const std::string &textureName, bool keepRandomTints)
{
	this->TextureMapName = textureName;
	this->RandomColorTints = keepRandomTints;
	return *this;
}

// Between 0 and 1, 0 being no resistance and 1 being full resistance
VoxelBuilder &VoxelBuilder::SetSolidInertiaResistance(float resistance)
{
	if(resistance < 0 || resistance > 1)
		throw std::runtime_error("Solid Inertia Resistance must be between 0 and 1\nMaterial name: " + this->Name);

	this->SolidInertiaResistance = resistance;
	return *this;
}

VoxelBuilder &VoxelBuilder::SetFluidDispersionRate(uint8_t rate)
{
	this->FluidDispursionRate = rate;
	return *this;
}

VoxelBuilder &VoxelBuilder::SetFlamability(uint8_t flamability)
{
	this->Flamability = flamability;
	return *this;
}

VoxelBuilder &Registry::VoxelBuilder::SpecialFactoryOverride(std::string factoryID)
{
    this->specialFactoryID = factoryID;
	return *this;
}

VoxelProperty VoxelBuilder::Build()
{
	Registry::VoxelTextureMap *map = nullptr;
	if(!this->TextureMapName.empty()){
		auto it = VoxelRegistry::textureMaps.find(this->TextureMapName);
		if(it == VoxelRegistry::textureMaps.end())
			throw std::runtime_error("Texture map not found: " + this->TextureMapName);

		map = it->second;
	}

    return VoxelProperty{
		.name = this->Name,
		.Constructor = this->Constructor,
		.pColor = this->Color,
		.Density = this->Density,
		.heatCapacity = this->HeatCapacity,
		.heatConductivity = this->HeatConductivity,
		.CooledChange = this->CooledChange,
		.HeatedChange = this->HeatedChange,
		.SolidInertiaResistance = this->SolidInertiaResistance,
		.FluidDispursionRate = this->FluidDispursionRate,
		.Flamability = this->Flamability,
		.TextureMap = map,
		.RandomColorTints = this->RandomColorTints,
		.Reactions = {},
		.specialFactoryID = this->specialFactoryID
	};
}

/// @brief Allocates a new instance of a voxel element from string id
/// @return pointer to the newly created voxel element
VoxelElement *CreateVoxelElement(std::string id, Vec2i position, float amount, Temperature temp, bool placeUnmovableSolids)
{
	VoxelProperty* prop = VoxelRegistry::GetProperties(id);   

	return CreateVoxelElement(prop, id, position, amount, temp, placeUnmovableSolids);
}

/// @brief Allocates a new instance of a voxel element from numeric id
/// @return pointer to the newly created voxel element
Volume::VoxelElement *CreateVoxelElement(uint32_t id, Vec2i position, float amount, Volume::Temperature temp, bool placeUnmovableSolids)
{
	VoxelProperty* prop = VoxelRegistry::GetProperties(id);

	std::string stringId = VoxelRegistry::GetStringID(id);

	return CreateVoxelElement(prop, stringId, position, amount, temp, placeUnmovableSolids);
}
/// @brief Allocates a new instance of a voxel element from VoxelProperty pointer
/// @return pointer to the newly created voxel element
Volume::VoxelElement *CreateVoxelElement(Volume::VoxelProperty *property, std::string id, Vec2i position, float amount, Volume::Temperature temp, bool placeUnmovableSolids)
{
	VoxelElement *voxel;

    if(property->Constructor == DefaultVoxelConstructor::Custom){
		std::string factoryID = property->specialFactoryID.empty() ? id : property->specialFactoryID;
		auto it = VoxelRegistry::FindFactoryWithID(factoryID);

		if(it == nullptr){
			throw std::runtime_error("Voxel factory not found for id: " + factoryID + ". Did you forget use RegisterVoxelFactory?");
		}

		voxel = (*it)(position, temp, amount, placeUnmovableSolids);
		return voxel;
	}
	
	if(property->Constructor == DefaultVoxelConstructor::GasVoxel)
		voxel = new VoxelGas(id, position, temp, amount);
	else if(property->Constructor == DefaultVoxelConstructor::LiquidVoxel)
		voxel = new VoxelLiquid(id, position, temp, amount);
	else{
		voxel = new VoxelSolid(id, position, temp, placeUnmovableSolids, amount);
	}
	
	return voxel;
}

Registry::VoxelTextureMap::VoxelTextureMap()
	: name("Unnamed"), data(nullptr), width(0), height(0), possibleRotations(Registry::TextureRotation::None)
{
}

Registry::VoxelTextureMap::VoxelTextureMap(const std::string &textureName, Registry::TextureRotation possibleRotations)
    : name(textureName), data(nullptr), width(0), height(0), possibleRotations(possibleRotations)
{
	const std::string path = "Textures/VoxelMaps/" + textureName + ".bmp";
	SDL_Surface *surface = SDL_LoadBMP(path.c_str());

	if (!surface)
		throw std::runtime_error("Failed to load texture map for " + textureName + ": " + path + " SDL_Error: " + SDL_GetError());

	if(surface->format->format != SDL_PIXELFORMAT_ARGB8888)
		throw std::runtime_error("Texture map must be in ARGB8888 format for " + textureName + ": " + path);

	width = surface->w;
	height = surface->h;
	data = new RGBA[width * height];

	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			Uint32 pixel = ((Uint32*)surface->pixels)[y * width + x];
			RGBA color = RGBA(pixel);

			if(color.a == 0) color = RGBA(255, 255, 255, 255);
			
			data[y * width + x] = color;
		}
	}

	SDL_FreeSurface(surface);
}

Registry::VoxelTextureMap::~VoxelTextureMap()
{
	if(data) delete[] data;
	data = nullptr;
}

RGBA &Registry::VoxelTextureMap::operator()(unsigned int x, unsigned int y)
{
	if(!data)
		throw std::runtime_error("Texture map data not loaded for " + name);

	unsigned int overreachX = x / width;
	unsigned int overreachY = y / height;

	unsigned int localX = x % width;
	unsigned int localY = y % height;
	
	// to not waste resources where not needed
	if(this->possibleRotations == Registry::TextureRotation::None)
		return data[localY * width + localX];

	// seemingly random rotations with predictable outcome
	std::string key = std::to_string(overreachX) + "," + std::to_string(overreachY);
	size_t hashed = std::hash<std::string>{}(key);

	// only lets us rotate within the bounds of possibleRotations
	hashed = hashed & static_cast<size_t>(this->possibleRotations);

	// flip horizontally
	if((hashed & 0b01) != 0){
		localX = width - localX - 1;
	}
	// flip vertically
	if((hashed & 0b10) != 0){
		localY = height - localY - 1;
	}

	return data[localY * width + localX];
}
