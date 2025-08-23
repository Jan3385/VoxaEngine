#include "Registry/VoxelRegistry.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>

#include "World/voxelTypes.h"
#include "GameObjectRegistry.h"
#include "VoxelRegistry.h"
#include "GameEngine.h"

using namespace Volume;
using namespace Registry;

// Initialize the voxel properties

std::unordered_map<std::string, VoxelProperty> VoxelRegistry::registry = {};
std::unordered_map<uint32_t, VoxelProperty*> VoxelRegistry::idRegistry = {};
std::unordered_map<std::string, VoxelFactory> VoxelRegistry::voxelFactories = {};
std::vector<Registry::ChemicalReaction> VoxelRegistry::reactionRegistry = {};
GLuint Registry::VoxelRegistry::chemicalReactionsBuffer = 0;
uint32_t VoxelRegistry::idCounter = 1;
bool VoxelRegistry::registryClosed = false;

void VoxelRegistry::RegisterVoxel(const std::string &name, VoxelProperty property)
{
	if(registryClosed) 
		throw std::runtime_error("Voxel registered after designated time window: " + name);
	
	property.id = ++idCounter;

	registry[name] = property;

	idRegistry[property.id] = &registry[name];
}

void Registry::VoxelRegistry::RegisterVoxelFactory(const std::string &name, VoxelFactory factory)
{
	if(registryClosed) 
		throw std::runtime_error("Voxel factory registered after designated time window: " + name);

	voxelFactories[name] = factory;
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
			.SetFluidDispursionRate(0)
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
	registryClosed = true;
	
	struct ChemicalReactionID{
		uint32_t fromID;
		uint32_t catalystID;
		uint32_t toID;
		float reactionSpeed;
		uint32_t preserveCatalyst;
		float minTemperatureC;
	};

	// get IDs for chemical reactions
	std::vector<ChemicalReactionID> reactions;
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
	}

	// sort reactions by "from" ID to be able to quickly search them thru later on the GPU
	std::sort(reactions.begin(), reactions.end(), [](const ChemicalReactionID& a, const ChemicalReactionID& b) {
		return a.fromID < b.fromID;
	});

	// Upload chemical reactions to a GPU buffer
	glGenBuffers(1, &VoxelRegistry::chemicalReactionsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, VoxelRegistry::chemicalReactionsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, reactions.size() * sizeof(ChemicalReactionID), reactions.data(), GL_STATIC_DRAW);

	// Clear reaction registry to free up memory
	VoxelRegistry::reactionRegistry.clear();
	std::vector<ChemicalReactionID>().swap(reactions); // free memory
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

VoxelBuilder &VoxelBuilder::PhaseUp(std::string To, float Temperature)
{
	this->HeatedChange = { Temperature, To };
	return *this;
}

VoxelBuilder &VoxelBuilder::PhaseDown(std::string To, float Temperature)
{
	this->CooledChange = { Temperature, To };
	return *this;
}

VoxelBuilder &Registry::VoxelBuilder::Reaction(std::string To, std::string Catalyst, float ReactionSpeed, bool PreserveCatalyst, float MinTemperatureC)
{
	VoxelRegistry::reactionRegistry.push_back({ 
		this->Name, 
		Catalyst, 
		To, 
		ReactionSpeed, 
		PreserveCatalyst, 
		MinTemperatureC 
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

// Between 0 and 1, 0 being no resistance and 1 being full resistance
VoxelBuilder &VoxelBuilder::SetSolidInertiaResistance(float resistance)
{
	if(resistance < 0 || resistance > 1)
		throw std::runtime_error("Solid Inertia Resistance must be between 0 and 1\nMaterial name: " + this->Name);

	this->SolidInertiaResistance = resistance;
	return *this;
}

VoxelBuilder &VoxelBuilder::SetFluidDispursionRate(uint8_t rate)
{
	this->FluidDispursionRate = rate;
	return *this;
}

VoxelBuilder &VoxelBuilder::SetFlamability(uint8_t flamability)
{
	this->Flamability = flamability;
	return *this;
}

VoxelProperty VoxelBuilder::Build()
{
    return VoxelProperty{
		.name = this->Name,
		.Constructor = this->Constructor,
		.pColor = this->Color,
		.Density = this->Density,
		.HeatCapacity = this->HeatCapacity,
		.HeatConductivity = this->HeatConductivity,
		.CooledChange = this->CooledChange,
		.HeatedChange = this->HeatedChange,
		.SolidInertiaResistance = this->SolidInertiaResistance,
		.FluidDispursionRate = this->FluidDispursionRate,
		.Flamability = this->Flamability
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
		auto it = VoxelRegistry::voxelFactories.find(id);

		if(it == VoxelRegistry::voxelFactories.end()){
			throw std::runtime_error("Voxel factory not found for id: " + id + ". Did you forget use RegisterVoxelFactory?");
		}

		voxel = it->second(position, temp, amount, placeUnmovableSolids);
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
