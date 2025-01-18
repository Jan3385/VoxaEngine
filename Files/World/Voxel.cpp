#include "Voxel.h"
#include "Chunk.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace Volume;

VoxelElement::VoxelElement()
	:type(VoxelType::Oxygen)
{
	this->properties = &VoxelRegistry::GetProperties(VoxelType::Oxygen);
	this->position = Vec2i(0, 0);
}

VoxelElement::VoxelElement(VoxelType type, Vec2i position)
	:position(position), type(type)
{
	this->properties = &VoxelRegistry::GetProperties(type);

	//tint the color factor 1 to 1.2
	float factor = 1 + ((rand() % 21) / 100.0f);
	this->color = RGB(
		std::clamp(static_cast<int>(this->properties->pColor.r * factor), 0, 255),
		std::clamp(static_cast<int>(this->properties->pColor.g * factor), 0, 255),
		std::clamp(static_cast<int>(this->properties->pColor.b * factor), 0, 255)
	);
}

VoxelElement::~VoxelElement()
{
}

bool VoxelElement::CheckTransitionTemps(ChunkMatrix& matrix) {	return false;	}

void VoxelElement::Swap(Vec2i &toSwapPos, ChunkMatrix &matrix)
{
    // Get the pointer to the voxel at the swap position
    std::shared_ptr<VoxelElement> swapVoxel = matrix.VirtualGetAt(toSwapPos);

    if (!swapVoxel) return;

    // Get the position of the current voxel
    Vec2i tempPos = this->position;

    //flip positions
    this->position = toSwapPos;
    swapVoxel->position = tempPos;

    // Swap the current voxel with the one at the swap position
    matrix.VirtualSetAt(shared_from_this());
    matrix.VirtualSetAt(swapVoxel);
}

void VoxelElement::DieAndReplace(ChunkMatrix &matrix, std::shared_ptr<VoxelElement> replacement)
{
    matrix.VirtualSetAt(replacement);
}

VoxelParticle::VoxelParticle()
	: VoxelElement(VoxelType::Oxygen, Vec2i(0, 0)), fPosition(Vec2f(0,0)), angle(0), speed(0),
	m_dPosition(0,0) { }

VoxelParticle::VoxelParticle(VoxelType type, const Vec2i &position, float angle, float speed)
	: VoxelElement(type, position), fPosition(Vec2f(position)), angle(angle), speed(speed),
	m_dPosition(speed* cos(angle), speed* sin(angle)) { }

bool VoxelParticle::Step(ChunkMatrix *matrix)
{
    updatedThisFrame = true;

    //new position variables
    this->fPosition += m_dPosition;
    Vec2i newPos = Vec2i(fPosition);

    //Adjust position according to gravity
    m_dPosition = m_dPosition + Vec2f(0, 0.4f);

    Vec2i futurePos = Vec2i(
    	static_cast<int>(fPosition.getX() + m_dPosition.getX()), 
    	static_cast<int>(fPosition.getY() + m_dPosition.getY())
    );

    std::shared_ptr<VoxelElement> futureVoxel = matrix->VirtualGetAt(futurePos);
    if (!futureVoxel ||
    	futureVoxel->GetState() == VoxelState::ImmovableSolid || futureVoxel->GetState() == VoxelState::MovableSolid || 
    	particleIterations <= 0)
    {
    	this->position = newPos;

    	//check if the position for the particle exists
    	if (!matrix->IsValidWorldPosition(this->position))
    	{
    		return true;
    	}

		matrix->PlaceVoxelAt(this->position, this->type);
    	return true;
    }

    this->particleIterations--;
    this->position = newPos;

    return false;
}

bool VoxelSolid::CheckTransitionTemps(ChunkMatrix &matrix)
{
    if (this->temperature.GetCelsius() > this->properties->lowerTemp.GetCelsius())
    {
    	this->DieAndReplace(matrix, std::make_shared<VoxelLiquid>(this->type, this->position));
    	return true;
    }
    return false;
}

bool VoxelMovableSolid::Step(ChunkMatrix *matrix)
{
    //lazy hack to make the chunk its in update in the next cycle
    if (updatedThisFrame) {
    	return true;
    }
    updatedThisFrame = true;
    if(this->CheckTransitionTemps(*matrix)){
        return true;
    }

    //Fall below and if not falling try to move to the sides

    //falling down + acceleration + setting isFalling to near voxel handling
    if (StepAlongDirection(matrix, Vec2i(0, 1), Acceleration)) { //if is able to fall down
    	IsFalling = true;
    	Acceleration += 1;

    	//try to set isFalling to true on adjasent voxels - simulates inertia
    	std::shared_ptr<VoxelElement> left = matrix->VirtualGetAt(this->position + Vec2i(-1, 1));
    	std::shared_ptr<VoxelElement> right = matrix->VirtualGetAt(this->position + Vec2i(1, 1));
    	if (left && left->GetState() == VoxelState::MovableSolid)
    	{
    		std::shared_ptr<VoxelMovableSolid> leftMovable = std::dynamic_pointer_cast<VoxelMovableSolid>(left);
    		if ((1 - leftMovable->InertiaResistance) * 1000 > rand() % 1000) {
    			leftMovable->IsFalling = true;
    			leftMovable->XVelocity = 1;
    		}
    	}
    	if (right && right->GetState() == VoxelState::MovableSolid)
    	{
    		std::shared_ptr<VoxelMovableSolid> rightMovable = std::dynamic_pointer_cast<VoxelMovableSolid>(right);
    		if ((1 - rightMovable->InertiaResistance) * 1000 > rand() % 1000) {
    			rightMovable->IsFalling = true;
    			rightMovable->XVelocity = 1;
    		}
    	}

    	return true;
    }
    else if (IsFalling) { //On the frame of the impact
    	StopFalling();
    	//If the voxel below is a solid, try to move to the sides
    
    	//try to set isFalling to true on voxel below - simulates inertia
    	std::shared_ptr<VoxelElement> below = matrix->VirtualGetAt(this->position + Vec2i(0, 1)); //TODO: figure out why did the voxel pos change to y=-1 when there was Vec2i(-1,1)
    	if (below && below->GetState() == VoxelState::MovableSolid)
    	{
    		std::shared_ptr<VoxelMovableSolid> belowMovable = std::dynamic_pointer_cast<VoxelMovableSolid>(below);
    		if ((1 - belowMovable->InertiaResistance) * 1000 > rand() % 1000) {
    			belowMovable->IsFalling = true;
    			belowMovable->XVelocity = 1;
    		}
    	}

    	if (StepAlongDirection(matrix, Vec2i(-1, 1), 1))
    		return true;
    	else if (StepAlongDirection(matrix, Vec2i(1, 1), 1))
    		return true;
    	else if (StepAlongDirection(matrix, Vec2i(-1, 0), XVelocity))
    		return true;
    	else if (StepAlongDirection(matrix, Vec2i(1, 0), XVelocity))
    		return true;

    	below = matrix->VirtualGetAt(this->position + Vec2i(0, 1));
    	if (below && below->GetState() == VoxelState::MovableSolid)
    	{
    		std::shared_ptr<VoxelMovableSolid> belowMovable = std::dynamic_pointer_cast<VoxelMovableSolid>(below);
    		if ((1 - belowMovable->InertiaResistance) * 1000 > rand() % 1000) {
    			belowMovable->IsFalling = true;
    			belowMovable->XVelocity = 1;
    		}
    	}

    	XVelocity = 0;
    }

    //if pixel have not found a place to move to return false
    return false;
}

bool VoxelMovableSolid::StepAlongDirection(ChunkMatrix *matrix, Vec2i direction, short int length)
{
    std::shared_ptr<VoxelElement> side = matrix->VirtualGetAt(this->position + direction);
    if (side && VoxelRegistry::CanBeMovedBySolid(side->GetState()))
    {
    	Vec2i sidePos = side->position;
    	for (short int i = 0; i < length; ++i)
    	{
    		sidePos += direction;
    		side = matrix->VirtualGetAt(sidePos);
    		if (!side || (side->GetState() != VoxelState::Gas))
    		{
    			sidePos -= direction;
    			this->Swap(sidePos, *matrix);
    			return true;
    		}
    	}
    	this->Swap(sidePos, *matrix);
    	return true;
    }
    return false;
}

void VoxelMovableSolid::StopFalling()
{
    XVelocity = ((Acceleration / 6) / (rand()%2 + 1)) + 1;
    IsFalling = false;
    Acceleration = 1;
}

bool VoxelLiquid::Step(ChunkMatrix *matrix)
{
	//TODO: make water only move when theres water above or below it
    //lazy hack to make the chunk its in update in the next cycle
    if (updatedThisFrame) {
    	return true;
    }
    updatedThisFrame = true;
    //falling down + acceleration

    if(this->CheckTransitionTemps(*matrix)){
        return true;
    }

    if (StepAlongDirection(matrix, Vec2i(0, 1), Acceleration)) {
    	IsFalling = true;
    	Acceleration += 1;
    	return true;
    }
    else if (IsFalling) {
    	IsFalling = false;
    	Acceleration = 1;
    }

    //If the voxel below is a solid, try to move to the bottom left and bottom right
    std::shared_ptr<VoxelElement> left = matrix->VirtualGetAt(this->position + Vec2i(-1, 1));
    std::shared_ptr<VoxelElement> right = matrix->VirtualGetAt(this->position + Vec2i(1, 1));
    if (left && VoxelRegistry::CanBeMovedByLiquid(left->GetState()))
    {
    	this->Swap(left->position, *matrix);
    	return true;
    }
    else if (right && VoxelRegistry::CanBeMovedByLiquid(right->GetState()))
    {
    	this->Swap(right->position, *matrix);
    	return true;
    }

    //if there is a liquid voxel above, skip
    std::shared_ptr<VoxelElement> above = matrix->VirtualGetAt(this->position + Vec2i(0, -1));
    std::shared_ptr<VoxelElement> moreAbove = matrix->VirtualGetAt(this->position + Vec2i(0, -2));
    if (above && above->GetState() == VoxelState::Liquid && moreAbove && moreAbove->GetState())
    	return false;

    /*
    //If the voxel below is a solid, try to move to the sides
    short int direction = 1;
    if (StepAlongDirection(matrix, sf::Vector2i(direction, 0), this->dispursionRate - rand()%20))
    	return true;

    direction *= -1;
    if (StepAlongDirection(matrix, sf::Vector2i(direction, 0), this->dispursionRate - rand() % 20))
    	return true;
    */
    Vec2i MovePosition = GetValidSideSwapPosition(*matrix, this->dispursionRate);
    if (MovePosition != this->position) {
    	this->Swap(MovePosition, *matrix);
    	return true;
    }
    //if pixel have not found a place to move to return false
    return false;
}

bool VoxelLiquid::StepAlongDirection(ChunkMatrix *matrix, Vec2i direction, short int length)
{
    std::shared_ptr<VoxelElement> side = matrix->VirtualGetAt(this->position + direction);
    if (side && (side->GetState() == VoxelState::Gas))
    {
    	Vec2i sidePos = side->position;
    	for (short int i = 0; i < length; ++i)
    	{
    		sidePos += direction;
    		side = matrix->VirtualGetAt(sidePos);
    		if (!side || !VoxelRegistry::CanBeMovedByLiquid(side->GetState()))
    		{
    			sidePos -= direction;
    			this->Swap(sidePos, *matrix);
    			return true;
    		}
    	}
    	this->Swap(sidePos, *matrix);
    	return true;
    }
    return false;
}

Vec2i VoxelLiquid::GetValidSideSwapPosition(ChunkMatrix &matrix, short int length)
{
    Vec2i LastValidPosition = this->position;
    Vec2i StartPosition = this->position;
    Vec2i CurrentPosition = this->position;

    //try to move to the sides
    for (short int i = 0; i < length; ++i)
    {
    	CurrentPosition += Vec2i(-1, 0);
    	std::shared_ptr<VoxelElement> side = matrix.VirtualGetAt(CurrentPosition);
    	if (!side) break;
		
		if (side->GetState() == VoxelState::ImmovableSolid || side->GetState() == VoxelState::MovableSolid) break;
    	if (side->GetState() == VoxelState::Gas)
    	{
    		LastValidPosition = CurrentPosition;
    	}
    }
    if (LastValidPosition != StartPosition) return LastValidPosition;
    CurrentPosition = StartPosition;
    for (short int i = 0; i < length; ++i)
    {
    	CurrentPosition += Vec2i(1, 0);
    	std::shared_ptr<VoxelElement> side = matrix.VirtualGetAt(CurrentPosition);
    	if (!side) break;
		
    	if (side->GetState() == VoxelState::ImmovableSolid || side->GetState() == VoxelState::MovableSolid) break;
    	if (side->GetState() == VoxelState::Gas)
    	{
    		LastValidPosition = CurrentPosition;
    	}
    }
    return LastValidPosition;
}

bool VoxelLiquid::CheckTransitionTemps(ChunkMatrix &matrix)
{
    if (this->temperature.GetCelsius() < this->properties->lowerTemp.GetCelsius())
    {
    	this->DieAndReplace(matrix, std::make_shared<VoxelMovableSolid>(this->type, this->position));
    	return true;
    }
    else if (this->temperature.GetCelsius() > this->properties->upperTemp.GetCelsius())
    {
    	this->DieAndReplace(matrix, std::make_shared<VoxelGas>(this->type, this->position));
    	return true;
    }
    return false;
}

bool VoxelGas::Step(ChunkMatrix *matrix)
{
    //lazy hack to make the chunk its in update in the next cycle
	//TODO: fix this making the neighboring chunk active
    if (updatedThisFrame) {
    	return true;
    }
    updatedThisFrame = true;

    if(this->CheckTransitionTemps(*matrix)){
        return true;
    }

    //swapping with a random gas voxel nearby
    //TODO: make this function use density
    //TODO: avoid making useless gas swaps
    //Vec2i toSwapPos = this->position + Vec2i(rand() % 3 - 1, rand() % 3 - 1);
    //std::shared_ptr<VoxelElement> toSwap = matrix->VirtualGetAt(toSwapPos);
    //if (toSwap && toSwap->GetState() == VoxelState::Gas && !toSwap->updatedThisFrame)
    //{
    //	if(toSwap->properties->name != this->properties->name) this->Swap(toSwapPos, *matrix);
    //	return false; //TODO: temp return false;
    //}
    return false;
}

bool VoxelGas::CheckTransitionTemps(ChunkMatrix &matrix)
{
    if (this->temperature.GetCelsius() < this->properties->upperTemp.GetCelsius())
    {
    	this->DieAndReplace(matrix, std::make_shared<VoxelLiquid>(this->type, this->position));
    	return true;
    }
    return false;
}

// Initialize the voxel properties
//TODO: phase changes - for example grass to dirt at 60C and dirt to lava at 1200C or Fire to Carbon dioxide at 200C. probably using a custom boil, freeze and solidify function
const std::map<Volume::VoxelType, VoxelProperty> VoxelRegistry::voxelProperties = {
	{ VoxelType::Oxygen, {"Oxygen", RGB(15, 15, 15), Temperature(-218.79f), Temperature(-182.96f), 1.429f}},
	{ VoxelType::Water, {"Water", RGB(3, 169, 244), Temperature(0.f), Temperature(99.98f), 2}},
	{ VoxelType::Stone, {"Stone", RGB(128, 128, 128), Temperature(1200.f), Temperature(3000.f), 200.f} },
	{ VoxelType::Grass, {"Grass", RGB(34, 139, 34), Temperature(1200.f), Temperature(3000.f), 200.f} },
	{ VoxelType::Sand, {"Sand", RGB(255, 193, 7), Temperature(1200.f), Temperature(3000.f), 190.f}},
	{ VoxelType::Dirt, {"Dirt", RGB(121, 85, 72), Temperature(1700.f), Temperature(5000.f), 200.f}},
	{ VoxelType::Fire, {"Fire", RGB(255, 87, 34), Temperature(-600.f), Temperature(30000.f), 1.f}},
	{ VoxelType::Plasma, {"Plasma", RGB(156, 39, 176), Temperature(-600.f), Temperature(30000.f), 1.f}},
	{ VoxelType::CarbonDioxide, {"Carbon Dioxide", RGB(4, 4, 4), Temperature(-56.6f), Temperature(-78.5f), 1.98f}},
};

const VoxelProperty &VoxelRegistry::GetProperties(VoxelType type)
{
    return voxelProperties.at(type);
}

const bool VoxelRegistry::CanGetMovedByExplosion(VoxelState state)
{
    return state == VoxelState::Liquid || state == VoxelState::MovableSolid;
}

const bool VoxelRegistry::CanGetDestroyedByExplosion(VoxelElement &element, float explosionPower)
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
