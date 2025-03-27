#pragma once

#include <unordered_map>
#include <string>
#include <optional>
#include "../Math/Color.h"
#include "../Math/Temperature.h"
#include "../Math/Vector.h"

struct PhaseChange{
		Volume::Temperature TemperatureAt;
		std::string To;
};

namespace Volume{
	class VoxelElement;
	
    static constexpr float TEMP_TRANSITION_THRESHOLD = 1.5f;
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

		uint32_t id = 0;
	};
}

Volume::VoxelElement* CreateVoxelElement(std::string id, Vec2i position, float amount, Volume::Temperature temp, bool placeUnmovableSolids);

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
	static Volume::VoxelProperty* GetProperties(uint32_t id);
	static bool CanGetMovedByExplosion(Volume::State state);
	static bool CanGetDestroyedByExplosion(std::string id, float explosionPower);
	static bool CanBeMovedBySolid(Volume::State state);
	static bool CanBeMovedByLiquid(Volume::State state);

	static void RegisterVoxel(const std::string& name, Volume::VoxelProperty property);
	static void RegisterVoxels();
	static std::unordered_map<std::string, Volume::VoxelProperty> registry;
	static std::unordered_map<uint32_t, Volume::VoxelProperty*> idRegistry;
private:
	static uint32_t idCounter;
	static bool registriesClosed;
};