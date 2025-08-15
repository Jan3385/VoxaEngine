#pragma once

#include <map>
#include <vector>
#include <memory>
#include <GL/glew.h>
#include "Math/Vector.h"
#include "Registry/VoxelRegistry.h"

// Forward declaration of ChunkMatrix
class ChunkMatrix;

namespace Volume {
	struct VoxelHeatData{
		float temperature;
		float capacity;
		float conductivity;
	};
	struct VoxelPressureData{
		float pressure;
		uint32_t id; // Voxel ID (last two bit reserved for voxel type (00 - gas, 01 - liquid, 10 - solid))
	};

	static constexpr float VOXEL_SIZE_METERS = 0.125f;
	//Interfaces
	class IGravity {
	public:
		virtual ~IGravity(){ };
		short int GetAcceleration() const { return Acceleration; };
		void SetAcceleration(short int acceleration);
		void IncrementAcceleration(short int amount);
		bool isFalling = false;
	private:
		short int Acceleration = 1;
		static constexpr short int MAX_ACCELERATION = 10;
	};
	//Base class for all voxel elements
	class VoxelElement
	{
	public:
		VoxelElement();
		VoxelElement(std::string id, Vec2i position, Temperature temperature, float amount);
		virtual ~VoxelElement();

		const std::string id;
		const VoxelProperty* properties = nullptr;
		// worlds space position
		Vec2i position;
		RGBA color;
		Temperature temperature;
		bool updatedThisFrame = false;

		bool partOfObject = false;

		float amount;

		// Functions
		//return the state of the element
		virtual State GetState() const { return State::Gas; };
		//returm true if the voxel moved
		virtual bool Step(ChunkMatrix* matrix) { updatedThisFrame = true; return false; };
		//return true if the voxel acted on another voxel
		virtual bool ActOnAnother() { return false; };
		//return the id of the voxel that this voxel should transition to, empty string if no transition
		std::string ShouldTransitionToID();

		// Swap the voxel with another voxel
		void Swap(Vec2i& toSwapPos,ChunkMatrix& matrix);
		void DieAndReplace(ChunkMatrix &matrix, std::string id);

		bool IsMoveableSolid();
		bool IsUnmoveableSolid();

		// Check if the voxel meets all the conditions to update colliders in its chunk
		virtual bool ShouldTriggerDirtyColliders() { return false; };
		virtual bool IsSolidCollider() const { return false; };

		bool IsStateBelowDensity(State state, float density) const;
		bool IsStateAboveDensity(State state, float density) const;

		static const char* computeShaderPressure;
		static GLuint computeShaderPressure_Program;
	};

	//Solid Voxels -> inherit from base voxel class
	class VoxelSolid : public VoxelElement, public IGravity {
	public:
		VoxelSolid() : VoxelElement("Dirt" , vector::ZERO, Temperature(21), 1) {};
		VoxelSolid(std::string id, Vec2i position, Temperature temp, bool isStatic, float amount) : VoxelElement(id, position, temp, amount), isStatic(isStatic) {};
		~VoxelSolid() {};
		
		bool isStatic;
		short unsigned int XVelocity = 0;     // 0 - short unsigned int max

		State GetState() const override { return State::Solid; };
		bool Step(ChunkMatrix* matrix) override;
		bool StepAlongDirection(ChunkMatrix* matrix, Vec2i direction, short int length);
		void TryToMoveVoxelBelow(ChunkMatrix* matrix);

		bool ShouldTriggerDirtyColliders() override;
		bool IsSolidCollider() const override;
	private:
		void StopFalling();
	};
	//liquid voxels -> inherit from base voxel class
	class VoxelLiquid : public VoxelElement, public IGravity {
	public:
		VoxelLiquid() : VoxelElement("Water", vector::ZERO, Temperature(21), 1) {};
		VoxelLiquid(std::string id, Vec2i position, Temperature temp, float amount) : VoxelElement(id, position, temp, amount) {};
		~VoxelLiquid() {};

		State GetState() const override { return State::Liquid; };
		bool Step(ChunkMatrix* matrix) override;
		bool StepAlongDirection(ChunkMatrix* matrix, Vec2i direction, short int length);
		Vec2i GetValidSideSwapPosition(ChunkMatrix& matrix, short int length);
	private:
		static constexpr uint16_t DesiredDensity = 20;
	};
	//Gas voxels -> inherit from base voxel class
	struct VoxelGas : public VoxelElement {
	public:
		static constexpr double MinimumGasAmount = 0.0000001f;

		VoxelGas() : VoxelElement("Oxygen", vector::ZERO, Temperature(21), 1) {};
		VoxelGas(std::string id, Vec2i position, Temperature temp, float amount);
		~VoxelGas() {};

		State GetState() const override { return State::Gas; };
		bool Step(ChunkMatrix* matrix) override;
		bool StepAlongSide(ChunkMatrix *matrix, bool positiveX, short int length);
		bool MoveInDirection(ChunkMatrix* matrix, Vec2i direction);
	};

	int GetLiquidVoxelPercentile(std::vector<VoxelElement *> voxels);
}
