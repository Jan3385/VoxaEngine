#include "VoxelRegistry.h"
#include <stdexcept>
#include <iostream>

using namespace Volume;
// Initialize the voxel properties

std::unordered_map<std::string, Volume::VoxelProperty> VoxelRegistry::registry = {};

void VoxelRegistry::RegisterVoxel(const std::string &name, const Volume::VoxelProperty property)
{
	registry[name] = property;
}

void VoxelRegistry::RegisterVoxels()
{
	std::cout << "Registering voxels ";
	using namespace Volume;
	VoxelRegistry::RegisterVoxel(
		"Oxygen",
		VoxelBuilder(State::Gas, 919, 0.026, 1.429)
			.SetName("Oxygen")
			//.SetColor(RGBA(15, 15, 15, 50))
			.SetColor(RGBA(15, 15, 15, 50))
			.PhaseDown("Liquid_Oxygen", -182.96)
			.SetFluidDispursionRate(3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Liquid_Oxygen",
		VoxelBuilder(State::Liquid, 919, 0.026, 1.429)
			.SetName("Liquid Oxygen")
			.SetColor(RGBA(50, 50, 50, 122))
			.PhaseUp("Oxygen", -182.96)
			.PhaseDown("Solid_Oxygen", -218.79)
			.SetFluidDispursionRate(10)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Solid_Oxygen",
		VoxelBuilder(State::Solid, 919, 0.026, 1.429)
			.SetName("Solid Oxygen")
			.SetColor(RGBA(70, 70, 70, 200))
			.PhaseUp("Liquid_Oxygen", -218.79)
			.SetSolidInertiaResistance(0.15)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Water",
		VoxelBuilder(State::Liquid, 4186, 0.6f, 2)
			.SetName("Water")
			.SetColor(RGBA(3, 169, 244, 200))
			.PhaseDown("Ice", 0)
			.PhaseUp("Steam", 99.98)
			.SetFluidDispursionRate(10)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Ice",
		VoxelBuilder(State::Solid, 4186, 0.6f, 2)
			.SetName("Ice")
			.SetColor(RGBA(3, 169, 244, 220))
			.PhaseUp("Water", 0)
			.SetSolidInertiaResistance(0.4)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Steam",
		VoxelBuilder(State::Gas, 4186, 0.6f, 1)
			.SetName("Steam")
			.SetColor(RGBA(101, 193, 235, 180))
			.PhaseDown("Water", 99.98)
			.SetFluidDispursionRate(7)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Stone",
		VoxelBuilder(State::Solid, 800, 2.5, 200)
			.SetName("Stone")
			.SetColor(RGBA(128, 128, 128, 255))
			.PhaseUp("Magma", 1200)
			.SetSolidInertiaResistance(0.8)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Magma",
		VoxelBuilder(State::Liquid, 800, 2.5, 200)
			.SetName("Magma")
			.SetColor(RGBA(161, 56, 14, 230))
			.PhaseDown("Stone", 1200)
			.SetFluidDispursionRate(3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Grass",
		VoxelBuilder(State::Solid, 1100, 0.06, 200)
			.SetName("Grass")
			.SetColor(RGBA(34, 139, 34, 255))
			.PhaseUp("Dirt", 200)
			.SetSolidInertiaResistance(0.5)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Dirt",
		VoxelBuilder(State::Solid, 1000, 0.3, 200)
			.SetName("Dirt")
			.SetColor(RGBA(121, 85, 72, 255))
			.PhaseUp("Magma", 1400)
			.SetSolidInertiaResistance(0.7)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Sand",
		VoxelBuilder(State::Solid, 830, 0.25, 190)
			.SetName("Sand")
			.SetColor(RGBA(255, 193, 7, 255))
			.PhaseUp("Liquid_Glass", 1000)
			.SetSolidInertiaResistance(0.1)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Liquid_Glass",
		VoxelBuilder(State::Liquid, 830, 0.25, 190)
			.SetName("Glass")
			.SetColor(RGBA(255, 219, 176, 100))
			.PhaseDown("Glass", 1000)
			.SetFluidDispursionRate(4)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Glass",
		VoxelBuilder(State::Solid, 830, 0.25, 190)
			.SetName("Glass")
			.SetColor(RGBA(255, 255, 255, 80))
			.PhaseUp("Liquid_Glass", 1000)
			.SetSolidInertiaResistance(0.2)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Fire",
		VoxelBuilder(State::Gas, 100, 0.4, 1)
			.SetName("Fire")
			.SetColor(RGBA(255, 87, 34, 210))
			.PhaseDown("Carbon_Dioxide", 200)
			.SetFluidDispursionRate(5)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Plasma",
		VoxelBuilder(State::Gas, 500, 2.0, 1)
			.SetName("Plasma")
			.SetColor(RGBA(156, 39, 176, 160))
			.PhaseDown("Fire", 1000)
			.SetFluidDispursionRate(1)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Carbon_Dioxide",
		VoxelBuilder(State::Gas, 850, 0.016, 1.98)
			.SetName("Carbon Dioxide")
			.SetColor(RGBA(4, 4, 4, 50))
			.PhaseDown("Liquid_Carbon_Dioxide", -56.6)
			.SetFluidDispursionRate(3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Liquid_Carbon_Dioxide",
		VoxelBuilder(State::Liquid, 850, 0.016, 1.98)
			.SetName("Liquid Carbon Dioxide")
			.SetColor(RGBA(4, 4, 4, 150))
			.PhaseDown("Solid_Carbon_Dioxide", -78.5)
			.PhaseUp("Carbon_Dioxide", -56.6)
			.SetFluidDispursionRate(13)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Solid_Carbon_Dioxide",
		VoxelBuilder(State::Solid, 850, 0.016, 1.98)
			.SetName("Solid Carbon Dioxide")
			.SetColor(RGBA(4, 4, 4, 130))
			.PhaseUp("Liquid_Carbon_Dioxide", -78.5)
			.SetSolidInertiaResistance(0.3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Iron",
		VoxelBuilder(State::Solid, 450, 5, 7874)
			.SetName("Iron")
			.SetColor(RGBA(130, 130, 130, 255))
			.PhaseUp("Molten_Iron", 1538)
			.SetSolidInertiaResistance(1)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Molten_Iron",
		VoxelBuilder(State::Liquid, 450, 5, 7874)
			.SetName("Molten Iron")
			.SetColor(RGBA(130, 130, 130, 240))
			.PhaseDown("Iron", 1538)
			.SetFluidDispursionRate(4)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Rust",
		VoxelBuilder(State::Solid, 450, 5, 7874)
			.SetName("Rust")
			.SetColor(RGBA(219, 139, 48, 255))
			.PhaseUp("Molten_Iron", 1538)
			.SetSolidInertiaResistance(0.5)
			.Build()
	);

	std::cout << "[ OK ]" << std::endl;
}

VoxelProperty* VoxelRegistry::GetProperties(std::string id)
{
    auto it = VoxelRegistry::registry.find(id);
	if(it == VoxelRegistry::registry.end()){
		throw std::runtime_error("Voxel property not found for id: " + id);
	}
	return &it->second;
}

bool VoxelRegistry::CanGetMovedByExplosion(Volume::State state)
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

VoxelBuilder::VoxelBuilder(Volume::State State, float tCapacity, float tConductivity, float Density)
{
	this->State = State;
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

VoxelBuilder &VoxelBuilder::SetSolidInertiaResistance(float resistance)
{
	this->SolidInertiaResistance = resistance;
	return *this;
}

VoxelBuilder &VoxelBuilder::SetFluidDispursionRate(uint8_t rate)
{
	this->FluidDispursionRate = rate;
	return *this;
}

Volume::VoxelProperty VoxelBuilder::Build()
{
    return {
		this->Name,
		this->State,
		this->Color,
		this->Density,
		this->HeatCapacity,
		this->HeatConductivity,
		this->CooledChange,
		this->HeatedChange,
		0,
		10
	};
}
