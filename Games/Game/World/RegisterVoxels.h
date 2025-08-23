#pragma once

#include <Registry/VoxelRegistry.h>
#include <Math/Temperature.h>

#include "World/Voxels/Fire.h"

namespace Registry{
void RegisterGameVoxels(){
    using namespace Registry;
    using namespace Volume;

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
		"Stone",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 800, 2.5, 200)
			.SetName("Stone")
			.SetColor(RGBA(128, 128, 128, 255))
			.PhaseUp("Magma", 1200)
			.SetSolidInertiaResistance(0.8)
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
		"Magma",
		VoxelBuilder(DefaultVoxelConstructor::LiquidVoxel, 800, 2.5, 200)
			.SetName("Magma")
			.SetColor(RGBA(161, 56, 14, 230))
			.PhaseDown("Stone", 1200)
			.SetFluidDispursionRate(3)
			.Build()
	);
	VoxelRegistry::RegisterVoxel(
		"Sand",
		VoxelBuilder(DefaultVoxelConstructor::SolidVoxel, 830, 0.25, 190)
			.SetName("Sand")
			.SetColor(RGBA(236, 204, 162, 255))
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
		"Plasma",
		VoxelBuilder(DefaultVoxelConstructor::GasVoxel, 500, 2.0, 1)
			.SetName("Plasma")
			.SetColor(RGBA(156, 39, 176, 160))
			.PhaseDown("Fire", 1000)
			.SetFluidDispursionRate(1)
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
}
}
