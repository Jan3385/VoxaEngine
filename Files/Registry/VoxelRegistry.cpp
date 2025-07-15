#include "Registry/VoxelRegistry.h"
#include <stdexcept>
#include <iostream>

#include "World/voxelTypes.h"
#include "GameObjectRegistry.h"
#include "VoxelRegistry.h"

using namespace Volume;
using namespace Registry;

// Initialize the voxel properties

std::unordered_map<std::string, VoxelProperty> VoxelRegistry::registry = {};
std::unordered_map<uint32_t, VoxelProperty*> VoxelRegistry::idRegistry = {};
uint32_t VoxelRegistry::idCounter = 1;
bool VoxelRegistry::registriesClosed = false;

void VoxelRegistry::RegisterVoxel(const std::string &name, VoxelProperty property)
{
	if(registriesClosed) 
		throw std::runtime_error("Voxel registered after designated time window: " + name);
	
	property.id = ++idCounter;

	registry[name] = property;

	idRegistry[property.id] = &property;
}

void VoxelRegistry::RegisterVoxels()
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
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 2000, 0.4, 1000)
			.SetName("Organic goo")
			.SetColor(RGBA(90, 80, 19, 255))
			.SetSolidInertiaResistance(0.5)
			.SetFlamability(0)
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
	VoxelRegistry::RegisterVoxel(
		"Fire_Solid",
		VoxelBuilder(DefaultVoxelConstructor::Custom, 100, 0.1, 1)
			.SetName("Fire")
			.SetColor(RGBA(255, 87, 34, 210))
			.PhaseDown("Carbon_Dioxide", 100)
			.SetSolidInertiaResistance(0.1)
			.Build()
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
			//.SetColor(RGBA(4, 4, 4, 50))
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

	registriesClosed = true;
	std::cout << "[ OK ]" << std::endl;
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
		if(id == "Empty")
			voxel = new EmptyVoxel(position);
		else if(id == "Fire")
			voxel = new FireVoxel(position, temp, amount);
		else if(id == "Fire_Liquid")
			voxel = new FireLiquidVoxel(position, temp, amount);
		else if(id == "Fire_Solid")
			voxel = new FireSolidVoxel(position, temp, amount, placeUnmovableSolids);
		else{
			throw std::runtime_error("Called a custom voxel constructor with unset constructor: " + id);
		}

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
