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

	VoxelRegistry::RegisterVoxel(
		"Oxygen",
		VoxelBuilder(DefaultVoxelConstructor::GasVoxel, 919, 0.026, 1.429)
			.SetName("Oxygen")
			.SetColor(RGBA(15, 15, 15, 50))
			.PhaseDown("Liquid_Oxygen", -182.96)
			.SetFluidDispursionRate(3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Liquid_Oxygen",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 919, 0.026, 1.429)
			.SetName("Liquid Oxygen")
			.SetColor(RGBA(50, 50, 50, 122))
			.PhaseUp("Oxygen", -182.96)
			.PhaseDown("Solid_Oxygen", -218.79)
			.SetFluidDispursionRate(10)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Solid_Oxygen",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 919, 0.026, 1.429)
			.SetName("Solid Oxygen")
			.SetColor(RGBA(70, 70, 70, 200))
			.PhaseUp("Liquid_Oxygen", -218.79)
			.SetSolidInertiaResistance(0.15)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Organics",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 2000, 0.4, 700)
			.SetName("Organic goo")
			.SetColor(RGBA(90, 80, 19, 255))
			.SetSolidInertiaResistance(0.5)
			.SetFlamability(0)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Uncarium",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 100, 0.01f, 1000)
			.SetName("Uncarium")
			.SetColor(RGBA(25, 25, 25, 255))
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Water",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 4186, 0.6f, 2)
			.SetName("Water")
			.SetColor(RGBA(3, 169, 244, 200))
			.PhaseDown("Ice", 0)
			.PhaseUp("Steam", 99.98)
			.SetFluidDispursionRate(10)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Ice",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 4186, 0.6f, 2)
			.SetName("Ice")
			.SetColor(RGBA(3, 169, 244, 220))
			.PhaseUp("Water", 0)
			.SetSolidInertiaResistance(0.4)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Steam",
		VoxelBuilder(DefaultVoxelConstructor::GasVoxel, 4186, 0.6f, 1)
			.SetName("Steam")
			.SetColor(RGBA(101, 193, 235, 180))
			.PhaseDown("Water", 99.98)
			.SetFluidDispursionRate(7)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Stone",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 800, 2.5, 200)
			.SetName("Stone")
			.SetColor(RGBA(128, 128, 128, 255))
			.PhaseUp("Magma", 1200)
			.SetSolidInertiaResistance(0.8)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Magma",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 800, 2.5, 200)
			.SetName("Magma")
			.SetColor(RGBA(161, 56, 14, 230))
			.PhaseDown("Stone", 1200)
			.SetFluidDispursionRate(3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Grass",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 1100, 0.06, 200)
			.SetName("Grass")
			.SetColor(RGBA(34, 139, 34, 255))
			.PhaseUp("Dirt", 200)
			.SetSolidInertiaResistance(0.5)
			.SetFlamability(70)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Dirt",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 1000, 0.3, 200)
			.SetName("Dirt")
			.SetColor(RGBA(121, 85, 72, 255))
			.PhaseUp("Magma", 1400)
			.SetSolidInertiaResistance(0.7)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Sand",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 830, 0.25, 190)
			.SetName("Sand")
			.SetColor(RGBA(255, 193, 7, 255))
			.PhaseUp("Liquid_Glass", 1000)
			.SetSolidInertiaResistance(0.1)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Liquid_Glass",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 830, 0.25, 190)
			.SetName("Glass")
			.SetColor(RGBA(255, 219, 176, 100))
			.PhaseDown("Glass", 1000)
			.SetFluidDispursionRate(4)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Glass",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 830, 0.25, 190)
			.SetName("Glass")
			.SetColor(RGBA(255, 255, 255, 80))
			.PhaseUp("Liquid_Glass", 1000)
			.SetSolidInertiaResistance(0.2)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Fire",
		VoxelBuilder(DefaultVoxelConstructor::Custom, 100, 0.4, 1)
			.SetName("Fire")
			.SetColor(RGBA(255, 87, 34, 210))
			.PhaseDown("Carbon_Dioxide", 200)
			.SetFluidDispursionRate(5)
			.Build()
	);
	VoxelRegistry::RegisterVoxelFactory(
		"Fire",
		[](Vec2i position, Volume::Temperature temp, float amount, bool placeUnmovableSolids) {
			return new FireVoxel(position, temp, amount);
		}
	);
	VoxelRegistry::RegisterVoxel(
		"Fire_Solid",
		VoxelBuilder(DefaultVoxelConstructor::Custom, 100, 0.1, 1)
			.SetName("Fire")
			.SetColor(RGBA(255, 87, 34, 210))
			.PhaseDown("Carbon_Dioxide", 100)
			.SetSolidInertiaResistance(0.1)
			.Build()
	);
	VoxelRegistry::RegisterVoxelFactory(
		"Fire_Solid",
		[](Vec2i position, Volume::Temperature temp, float amount, bool placeUnmovableSolids) {
			return new FireSolidVoxel(position, temp, amount, placeUnmovableSolids);
		}
	);
	VoxelRegistry::RegisterVoxel(
		"Fire_Liquid",
		VoxelBuilder(DefaultVoxelConstructor::Custom, 100, 0.1, 1)
			.SetName("Fire")
			.SetColor(RGBA(255, 87, 34, 210))
			.PhaseDown("Carbon_Dioxide", 100)
			.SetFluidDispursionRate(2)
			.Build()
	);
	VoxelRegistry::RegisterVoxelFactory(
		"Fire_Liquid",
		[](Vec2i position, Volume::Temperature temp, float amount, bool placeUnmovableSolids) {
			return new FireLiquidVoxel(position, temp, amount);
		}
	);
	VoxelRegistry::RegisterVoxel(
		"Ash",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 100, 0.4, 540)
			.SetName("Ash")
			.SetColor(RGBA(159, 159, 159, 255))
			.PhaseUp("Charcoal", 600)
			.SetSolidInertiaResistance(0.1)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Plasma",
		VoxelBuilder(DefaultVoxelConstructor::GasVoxel, 500, 2.0, 1)
			.SetName("Plasma")
			.SetColor(RGBA(156, 39, 176, 160))
			.PhaseDown("Fire", 1000)
			.SetFluidDispursionRate(1)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Carbon_Dioxide",
		VoxelBuilder(DefaultVoxelConstructor::GasVoxel, 850, 0.016, 1.98)
			.SetName("Carbon Dioxide")
			.SetColor(RGBA(4, 4, 4, 70))
			.PhaseDown("Liquid_Carbon_Dioxide", -56.6)
			.SetFluidDispursionRate(3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Liquid_Carbon_Dioxide",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 850, 0.016, 1.98)
			.SetName("Liquid Carbon Dioxide")
			.SetColor(RGBA(4, 4, 4, 150))
			.PhaseDown("Solid_Carbon_Dioxide", -78.5)
			.PhaseUp("Carbon_Dioxide", -56.6)
			.SetFluidDispursionRate(13)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Solid_Carbon_Dioxide",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 850, 0.016, 1.98)
			.SetName("Solid Carbon Dioxide")
			.SetColor(RGBA(4, 4, 4, 130))
			.PhaseUp("Liquid_Carbon_Dioxide", -78.5)
			.SetSolidInertiaResistance(0.3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Iron",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 450, 5, 7874)
			.SetName("Iron")
			.SetColor(RGBA(130, 130, 130, 255))
			.ReactionOxidation("Rust", 0.0001f)
			.PhaseUp("Molten_Iron", 1538)
			.SetSolidInertiaResistance(1)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Molten_Iron",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 450, 5, 7874)
			.SetName("Molten Iron")
			.SetColor(RGBA(130, 130, 130, 240))
			.PhaseDown("Iron", 1538)
			.SetFluidDispursionRate(4)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Rust",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 450, 5, 7874)
			.SetName("Rust")
			.SetColor(RGBA(219, 139, 48, 255))
			.PhaseUp("Molten_Iron", 1538)
			.SetSolidInertiaResistance(0.5)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Copper",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 300, 12, 8960)
			.SetName("Copper")
			.SetColor(RGBA(184, 115, 51, 255))
			.ReactionOxidation("Copper_Oxide", 0.00017f)
			.Reaction("Copper_Oxide", "Copper_Oxide", 0.0003f, true, Temperature(0).GetCelsius())
			.PhaseUp("Molten_Copper", 1085)
			.SetSolidInertiaResistance(0.8)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Molten_Copper",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 300, 12, 8960)
			.SetName("Molten Copper")
			.SetColor(RGBA(184, 115, 51, 240))
			.PhaseDown("Copper", 1085)
			.SetFluidDispursionRate(4)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Copper_Oxide",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 410, 0.7, 6315)
			.SetName("Copper Oxide")
			.SetColor(RGBA(50, 184, 115, 255))
			.PhaseUp("Molten_Copper", 1085)
			.SetSolidInertiaResistance(0.3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Gold",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 130, 70, 19300)
			.SetName("Gold")
			.SetColor(RGBA(255, 215, 0, 255))
			.PhaseUp("Molten_Gold", 1064)
			.SetSolidInertiaResistance(0.6)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Molten_Gold",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 130, 70, 19300)
			.SetName("Molten Gold")
			.SetColor(RGBA(255, 215, 0, 240))
			.PhaseDown("Gold", 1064)
			.SetFluidDispursionRate(4)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Wood",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 2000, 0.7, 700)
			.SetName("Wood")
			.SetColor(RGBA(139, 69, 19, 255))
			.PhaseUp("Charcoal", 350)
			.SetSolidInertiaResistance(0.5)
			.SetFlamability(170)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Charcoal",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 1100, 0.4, 1000)
			.SetName("Charcoal")
			.SetColor(RGBA(54, 69, 79, 255))
			.SetSolidInertiaResistance(0.5)
			.SetFlamability(200)
			.Build()
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
