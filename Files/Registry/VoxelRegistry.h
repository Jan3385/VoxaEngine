#pragma once

#include <unordered_map>
#include <string>
#include <optional>
#include "../Math/Color.h"
#include "../Math/Temperature.h"

struct PhaseChange{
		Volume::Temperature TemperatureAt;
		std::string To;
};

namespace Volume{
    static constexpr float TEMP_TRANSITION_THRESHOLD = 1.5f;
    enum class VoxelState {
		Gas,
		Liquid,
		MovableSolid,
		ImmovableSolid,
	};
	enum class State {
		Gas,
		Liquid,
		Solid,
	};
	struct VoxelProperty {
		std::string name;
		State state;
		RGBA pColor;
		float Density; // g/L or kg/m^3
		float HeatCapacity; // J/kg*K
		float HeatConductivity; // W/m*K
		std::optional<PhaseChange> CooledChange;
		std::optional<PhaseChange> HeatedChange;
		float SolidInertiaResistance;
		uint8_t FluidDispursionRate;
	};
}

class VoxelBuilder{
public:
	VoxelBuilder(Volume::State State, float tCapacity, float tConductivity, float Density);
	VoxelBuilder& SetName(std::string Name);
	VoxelBuilder& SetColor(RGBA Color);
	VoxelBuilder& PhaseUp(std::string To, float Temperature);
	VoxelBuilder& PhaseDown(std::string To, float Temperature);
	VoxelBuilder& SetSolidInertiaResistance(float resistance);
	VoxelBuilder& SetFluidDispursionRate(uint8_t rate);
	Volume::VoxelProperty Build();
private:
	Volume::State State;
	std::string Name = "NONAME";
	RGBA Color;
	float Density;
	float HeatCapacity;
	float HeatConductivity;
	std::optional<PhaseChange> CooledChange;
	std::optional<PhaseChange> HeatedChange;
	float SolidInertiaResistance; // 0 - 1
	uint8_t FluidDispursionRate; // 0 - 255
};

class VoxelRegistry {
public:
	static Volume::VoxelProperty* GetProperties(std::string id);
	static bool CanGetMovedByExplosion(Volume::VoxelState state);
	static bool CanGetDestroyedByExplosion(std::string id, float explosionPower);
	static bool CanBeMovedBySolid(Volume::VoxelState state);
	static bool CanBeMovedByLiquid(Volume::VoxelState state);

	static void RegisterVoxel(const std::string& name, const Volume::VoxelProperty property);
	static void RegisterVoxels();
	static std::unordered_map<std::string, Volume::VoxelProperty> registry;
private:
	//static std::unordered_map<Volume::VoxelType, Volume::VoxelProperty> voxelProperties;
};