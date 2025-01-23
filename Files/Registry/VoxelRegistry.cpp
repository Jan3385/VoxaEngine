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
			.SetColor(RGB(15, 15, 15))
			.PhaseDown("Liquid_Oxygen", -182.96)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Liquid_Oxygen",
		VoxelBuilder(State::Liquid, 919, 0.026, 1.429)
			.SetName("Liquid Oxygen")
			.SetColor(RGB(50, 50, 50))
			.PhaseUp("Oxygen", -182.96)
			.PhaseDown("Solid_Oxygen", -218.79)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Solid_Oxygen",
		VoxelBuilder(State::Solid, 919, 0.026, 1.429)
			.SetName("Solid Oxygen")
			.SetColor(RGB(70, 70, 70))
			.PhaseUp("Liquid_Oxygen", -218.79)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Water",
		VoxelBuilder(State::Liquid, 4186, 0.6f, 2)
			.SetName("Water")
			.SetColor(RGB(3, 169, 244))
			.PhaseDown("Ice", 0)
			.PhaseUp("Steam", 99.98)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Ice",
		VoxelBuilder(State::Solid, 4186, 0.6f, 2)
			.SetName("Ice")
			.SetColor(RGB(3, 169, 244))
			.PhaseUp("Water", 0)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Steam",
		VoxelBuilder(State::Gas, 4186, 0.6f, 2)
			.SetName("Steam")
			.SetColor(RGB(101, 193, 235))
			.PhaseDown("Water", 99.98)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Stone",
		VoxelBuilder(State::Solid, 800, 2.5, 200)
			.SetName("Stone")
			.SetColor(RGB(128, 128, 128))
			.PhaseUp("Magma", 1200)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Magma",
		VoxelBuilder(State::Liquid, 800, 2.5, 200)
			.SetName("Magma")
			.SetColor(RGB(161, 56, 14))
			.PhaseDown("Stone", 1200)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Grass",
		VoxelBuilder(State::Solid, 1100, 0.06, 200)
			.SetName("Grass")
			.SetColor(RGB(34, 139, 34))
			.PhaseUp("Dirt", 200)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Dirt",
		VoxelBuilder(State::Solid, 1000, 0.3, 200)
			.SetName("Dirt")
			.SetColor(RGB(121, 85, 72))
			.PhaseUp("Magma", 1400)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Sand",
		VoxelBuilder(State::Solid, 830, 0.25, 190)
			.SetName("Sand")
			.SetColor(RGB(255, 193, 7))
			.PhaseUp("Liquid_Glass", 1000)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Liquid_Glass",
		VoxelBuilder(State::Liquid, 830, 0.25, 190)
			.SetName("Glass")
			.SetColor(RGB(255, 219, 176))
			.PhaseDown("Glass", 1000)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Glass",
		VoxelBuilder(State::Solid, 830, 0.25, 190)
			.SetName("Glass")
			.SetColor(RGB(255, 255, 255))
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Fire",
		VoxelBuilder(State::Gas, 100, 0.4, 1)
			.SetName("Fire")
			.SetColor(RGB(255, 87, 34))
			.PhaseDown("Carbon_Dioxide", 200)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Plasma",
		VoxelBuilder(State::Gas, 500, 2.0, 1)
			.SetName("Plasma")
			.SetColor(RGB(156, 39, 176))
			.PhaseDown("Fire", 1000)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Carbon_Dioxide",
		VoxelBuilder(State::Gas, 850, 0.016, 1.98)
			.SetName("Carbon Dioxide")
			.SetColor(RGB(4, 4, 4))
			.PhaseDown("Liquid_Carbon_Dioxide", -56.6)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Liquid_Carbon_Dioxide",
		VoxelBuilder(State::Liquid, 850, 0.016, 1.98)
			.SetName("Liquid Carbon Dioxide")
			.SetColor(RGB(4, 4, 4))
			.PhaseDown("Solid_Carbon_Dioxide", -78.5)
			.PhaseUp("Carbon_Dioxide", -56.6)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Solid_Carbon_Dioxide",
		VoxelBuilder(State::Solid, 850, 0.016, 1.98)
			.SetName("Solid Carbon Dioxide")
			.SetColor(RGB(4, 4, 4))
			.PhaseUp("Liquid_Carbon_Dioxide", -78.5)
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

const bool VoxelRegistry::CanGetMovedByExplosion(Volume::VoxelState state)
{
    return state == VoxelState::Liquid || state == VoxelState::MovableSolid;
}

const bool VoxelRegistry::CanGetDestroyedByExplosion(std::string id, float explosionPower)
{
    return true;
}

const bool VoxelRegistry::CanBeMovedBySolid(VoxelState state)
{
    return state == VoxelState::Gas || state == VoxelState::Liquid;
}

const bool VoxelRegistry::CanBeMovedByLiquid(VoxelState state)
{
    return state == VoxelState::Gas;
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

VoxelBuilder &VoxelBuilder::SetColor(RGB Color)
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
		this->HeatedChange
	};
}
