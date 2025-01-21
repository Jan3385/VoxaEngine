#include "VoxelRegistry.h"

using namespace Volume;
// Initialize the voxel properties
//TODO: phase changes - for example grass to dirt at 60C and dirt to lava at 1200C or Fire to Carbon dioxide at 200C. probably using a custom boil, freeze and solidify function
//std::unordered_map<Volume::VoxelType, VoxelProperty> VoxelRegistry::voxelProperties = {
//	{ VoxelType::Oxygen, {"Oxygen", RGB(15, 15, 15), Temperature(-218.79), Temperature(-182.96), 1.429, 919, 0.026}},
//	{ VoxelType::Water, {"Water", RGB(3, 169, 244), Temperature(0.), Temperature(99.98), 2, 4186, 0.6f}},
//	{ VoxelType::Stone, {"Stone", RGB(128, 128, 128), Temperature(1200.), Temperature(3000.), 200., 800, 2.5} },
//	{ VoxelType::Grass, {"Grass", RGB(34, 139, 34), Temperature(1200.), Temperature(3000.), 200., 1100, 0.06} },
//	{ VoxelType::Sand, {"Sand", RGB(255, 193, 7), Temperature(1200.), Temperature(3000.), 190., 830, 0.25}},
//	{ VoxelType::Dirt, {"Dirt", RGB(121, 85, 72), Temperature(1700.), Temperature(5000.), 200., 1000, 0.3}},
//	{ VoxelType::Fire, {"Fire", RGB(255, 87, 34), Temperature(-600.), Temperature(300.), 1., 1000, 0.4}},
//	{ VoxelType::Plasma, {"Plasma", RGB(156, 39, 176), Temperature(-600.), Temperature(30000.), 1., 3000, 2.0}},
//	{ VoxelType::CarbonDioxide, {"Carbon Dioxide", RGB(4, 4, 4), Temperature(-56.6), Temperature(-78.5), 1.98, 850, 0.016}},
//};

void VoxelRegistry::RegisterVoxels()
{
	using namespace Volume;

    VoxelRegistry::RegisterVoxelType(
		VoxelBuilder(VoxelType::Oxygen, Volume::VoxelState::Gas, 919, 0.026)
			.SetName("Oxygen")
			.SetColor(RGB(15, 15, 15))
			.SetDensity(1.429)
	);
	VoxelRegistry::RegisterVoxelType(
		VoxelBuilder(Volume::VoxelType::Water, Volume::VoxelState::Liquid, 4186, 0.6f)
			.SetName("Water")
			.SetColor(RGB(3, 169, 244))
			.SetDensity(2)
	);
	VoxelRegistry::RegisterVoxelType(
		VoxelBuilder(Volume::VoxelType::Stone, Volume::VoxelState::MovableSolid, 800, 2.5)
			.SetName("Stone")
			.SetColor(RGB(128, 128, 128))
			.SetDensity(200)
	);
	VoxelRegistry::RegisterVoxelType(
		VoxelBuilder(Volume::VoxelType::Grass, Volume::VoxelState::MovableSolid, 1100, 0.06)
			.SetName("Grass")
			.SetColor(RGB(34, 139, 34))
			.SetDensity(200)
	);
	VoxelRegistry::RegisterVoxelType(
		VoxelBuilder(Volume::VoxelType::Sand, Volume::VoxelState::MovableSolid, 830, 0.25)
			.SetName("Sand")
			.SetColor(RGB(255, 193, 7))
			.SetDensity(190)
	);
	VoxelRegistry::RegisterVoxelType(
		VoxelBuilder(Volume::VoxelType::Dirt, Volume::VoxelState::MovableSolid, 1000, 0.3)
			.SetName("Dirt")
			.SetColor(RGB(121, 85, 72))
			.SetDensity(200)
	);
	VoxelRegistry::RegisterVoxelType(
		VoxelBuilder(Volume::VoxelType::Fire, Volume::VoxelState::Gas, 1000, 0.4)
			.SetName("Fire")
			.SetColor(RGB(255, 87, 34))
			.SetDensity(1)
	);
	VoxelRegistry::RegisterVoxelType(
		VoxelBuilder(Volume::VoxelType::Plasma, Volume::VoxelState::Gas, 3000, 2.0)
			.SetName("Plasma")
			.SetColor(RGB(156, 39, 176))
			.SetDensity(1)
	);
	VoxelRegistry::RegisterVoxelType(
		VoxelBuilder(Volume::VoxelType::CarbonDioxide, Volume::VoxelState::Gas, 850, 0.016)
			.SetName("Carbon Dioxide")
			.SetColor(RGB(4, 4, 4))
			.SetDensity(1.98)
	);
}

const VoxelProperty &VoxelRegistry::GetProperties(VoxelType type)
{
    return voxelProperties.at(type);
}

const bool VoxelRegistry::CanGetMovedByExplosion(VoxelState state)
{
    return state == VoxelState::Liquid || state == VoxelState::MovableSolid;
}

const bool VoxelRegistry::CanGetDestroyedByExplosion(Volume::VoxelType type, float explosionPower)
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

void VoxelRegistry::RegisterVoxelType(VoxelBuilder build)
{
    VoxelRegistry::voxelProperties.insert({build.Type, build.Build()});
}

VoxelBuilder::VoxelBuilder(Volume::VoxelType Type, Volume::VoxelState DefaultState, float tCapacity, float tConductivity)
{
	this->Type = Type;
	this->DefaultState = DefaultState;
	this->HeatCapacity = tCapacity;
	this->HeatConductivity = tConductivity;
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

VoxelBuilder &VoxelBuilder::SetDensity(float Density)
{
    this->Density = Density;
	return *this;
}

Volume::VoxelProperty VoxelBuilder::Build()
{
    return {
		this->Name,
		this->Color,
		this->Density,
		this->HeatCapacity,
		this->HeatConductivity
	};
}
