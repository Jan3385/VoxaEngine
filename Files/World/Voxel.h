#pragma once

#include <map>
#include <memory>
#include "../Math/Vector.h"
#include "../Registry/VoxelRegistry.h"

// Forward declaration of ChunkMatrix
class ChunkMatrix;

namespace Volume {
	//Interfaces
	class IGravity {
	public:
		virtual ~IGravity(){ };
		short int Acceleration = 1;
		bool IsFalling = false;
	};
	//Base class for all voxel elements
	class VoxelElement : public std::enable_shared_from_this<VoxelElement>
	{
	public:
		VoxelElement();
		VoxelElement(VoxelType type, Vec2i position, Temperature temperature);
		virtual ~VoxelElement();

		const VoxelType type;
		const VoxelProperty* properties = nullptr;
		Vec2i position;
		RGB color;
		Temperature temperature;
		bool updatedThisFrame = false; //TODO: maybe set to true

		// Functions
		//return the state of the element
		virtual VoxelState GetState() { return VoxelState::ImmovableSolid; };
		//returm true if the voxel moved
		virtual bool Step(ChunkMatrix* matrix) { updatedThisFrame = true; return false; };
		//return true if the voxel acted on another voxel
		virtual bool ActOnAnother() { return false; };
		//return true if the voxel transitioned to another state
		virtual bool CheckTransitionTemps(ChunkMatrix& matrix);

		// Swap the voxel with another voxel
		void Swap(Vec2i& toSwapPos,ChunkMatrix& matrix);
		void DieAndReplace(ChunkMatrix& matrix, std::shared_ptr<VoxelElement> replacement);
	private:
		Volume::VoxelState state = VoxelState::ImmovableSolid;
	};

	class VoxelParticle : public VoxelElement {
	public:
		VoxelParticle();
		VoxelParticle(VoxelType type,const Vec2i& position, Temperature temp, float angle, float speed);

		Vec2f fPosition;

		//Angle in radians
		float angle = 0;
		float speed = 0;
		short int particleIterations = 50;
		
		bool Step(ChunkMatrix* matrix) override;
	private: 
		//Particle variables
		Vec2f m_dPosition;
	};

	//Solid Voxels -> inherit from base voxel class
	class VoxelSolid : public VoxelElement {
	public:
		VoxelSolid() : VoxelElement(VoxelType::Dirt, Vec2i(0, 0), Temperature(21)) {};
		VoxelSolid(VoxelType type, Vec2i position, Temperature temp) : VoxelElement(type, position, temp) {};
		virtual ~VoxelSolid() {};

		virtual VoxelState GetState() override { return VoxelState::ImmovableSolid; };
		bool CheckTransitionTemps(ChunkMatrix& matrix) override;
	};
	//Solid immovable voxels -> inherit from solid voxels
	class VoxelImmovableSolid : public VoxelSolid {
	public:
		VoxelImmovableSolid() : VoxelSolid(VoxelType::Dirt, Vec2i(0, 0), Temperature(21)) {};
		VoxelImmovableSolid(VoxelType type, Vec2i position, Temperature temp) : VoxelSolid(type, position, temp) {};
		~VoxelImmovableSolid() {};

		VoxelState GetState() override { return VoxelState::ImmovableSolid; };
		bool Step(ChunkMatrix* matrix) override { updatedThisFrame = true; return false; };
	};
	//solid movable voxels -> inherit from solid voxels
	class VoxelMovableSolid : public VoxelSolid, public IGravity {
	public:
		VoxelMovableSolid() : VoxelSolid(VoxelType::Sand, Vec2i(0, 0), Temperature(21)) {};
		VoxelMovableSolid(VoxelType type, Vec2i position, Temperature temp) : VoxelSolid(type, position, temp) {};
		~VoxelMovableSolid() {};

		short unsigned int XVelocity = 0;     // 0 - short unsigned int max
		float InertiaResistance = 0; //0 - 1

		VoxelState GetState() override { return VoxelState::MovableSolid; };
		bool Step(ChunkMatrix* matrix) override;
		bool StepAlongDirection(ChunkMatrix* matrix, Vec2i direction, short int length);
	private:
		void StopFalling();
	};
	//liquid voxels -> inherit from base voxel class
	class VoxelLiquid : public VoxelElement, public IGravity {
	public:
		VoxelLiquid() : VoxelElement(VoxelType::Water, Vec2i(0, 0), Temperature(21)) {};
		VoxelLiquid(VoxelType type, Vec2i position, Temperature temp) : VoxelElement(type, position, temp) {};
		~VoxelLiquid() {};

		VoxelState GetState() override { return VoxelState::Liquid; };
		bool Step(ChunkMatrix* matrix) override;
		bool StepAlongDirection(ChunkMatrix* matrix, Vec2i direction, short int length);
		Vec2i GetValidSideSwapPosition(ChunkMatrix& matrix, short int length);
		bool CheckTransitionTemps(ChunkMatrix& matrix) override;

		short int dispursionRate = 10;
	};
	//Gas voxels -> inherit from base voxel class
	struct VoxelGas : public VoxelElement {
	public:
		VoxelGas() : VoxelElement(VoxelType::Oxygen, Vec2i(0, 0), Temperature(21)) {};
		VoxelGas(VoxelType type, Vec2i position, Temperature temp) : VoxelElement(type, position, temp) {};
		~VoxelGas() {};

		VoxelState GetState() override { return VoxelState::Gas; };
		bool Step(ChunkMatrix* matrix) override;
		bool CheckTransitionTemps(ChunkMatrix& matrix) override;
	};
}
