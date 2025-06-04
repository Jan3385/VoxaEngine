#include "World/Voxels/Fire.h"
#include "World/ChunkMatrix.h"

using namespace Volume;

const RGBA FireVoxel::fireColors[8] = {
    RGBA(237, 31, 10, 210),
    RGBA(255, 87, 34, 210),
    RGBA(255, 154, 0, 210),
    RGBA(255, 144, 10, 210),
    RGBA(199, 75, 75, 210),
    RGBA(255, 69, 0, 210),
    RGBA(255, 140, 0, 210),
    RGBA(255, 165, 0, 210)
};

FireVoxel::FireVoxel(Vec2i position, Temperature temp, float pressure) : VoxelGas("Fire", position, temp, pressure){ }

// Spread the fire to adjacent voxels, returns true if this fire voxel is near oxygen
bool FireVoxel::Spread(ChunkMatrix *matrix, const VoxelElement *FireVoxel)
{
    //check for oxygen and spread
    bool isAroundOxygen = false;
    for(Vec2i dir : vector::AROUND8){
        VoxelElement* next = matrix->VirtualGetAt_NoLoad(FireVoxel->position + dir);
        if(next && next->id == "Oxygen"){
            isAroundOxygen = true;
            break;
        }
    }

    for(Vec2i dir : vector::AROUND8){
        VoxelElement* next = matrix->VirtualGetAt_NoLoad(FireVoxel->position + dir);
        if(next){
            //ignite based on Flamability
            if((rand()%256) - next->properties->Flamability < 0){

                // only 20% chance to ignite if there is no oxygen around, 0% for solids
                bool randomIgniteChance = next->GetState() != State::Solid && (rand() % 100 < 20);
                if(isAroundOxygen || randomIgniteChance){
                    matrix->SetFireAt(next->position);
                }
            }
        }
    }
    return isAroundOxygen;
}

bool FireVoxel::Step(ChunkMatrix *matrix)
{
    //check for oxygen and spread
    bool isAroundOxygen = FireVoxel::Spread(matrix, this);

    //flame burns faster without oxygen
    float amountChange;
    if(isAroundOxygen){
        forcedLifetimeTime--;
        amountChange = this->amount * 0.15f;
    }
    else{
        forcedLifetimeTime = std::max(forcedLifetimeTime - 3, 0);
        amountChange = this->amount * 0.25f;
    }

    for(Vec2i dir : vector::AROUND8){
        VoxelElement* next = matrix->VirtualGetAt_NoLoad(this->position + dir);
        if(next && next->GetState() == State::Gas){
            this->amount -= amountChange;
            matrix->PlaceVoxelAt(this->position + dir, "Carbon_Dioxide", this->temperature, false, amountChange, false);
            break;
        }
    }

    //set random fire color
    this->color = fireColors[rand() % fireColorCount];
    matrix->GetChunkAtWorldPosition(this->position)->dirtyRender = true;

    //15% chance to dissapear
    if (rand() % 100 < 15 || forcedLifetimeTime <= 0 || this->amount <= 0)
    {
        if(this->amount <= 0)
            this->amount = 0.1f;
        
        if(this->amount > 8)
            this->amount = 8;
        
    	this->DieAndReplace(*matrix, "Carbon_Dioxide");
        return true;
    }
    VoxelGas::Step(matrix);
    return true;
}

FireLiquidVoxel::FireLiquidVoxel(Vec2i position, Temperature temp, float amount)
    : VoxelLiquid("Fire_Liquid", position, temp, amount)
{
}

bool FireLiquidVoxel::Step(ChunkMatrix *matrix)
{
    //check for oxygen and spread
    bool isAroundOxygen = FireVoxel::Spread(matrix, this);

    //flame burns faster with oxygen
    float amountChange;
    if(isAroundOxygen)
        amountChange = (this->amount * 0.0001f) + 0.5;
    else
        amountChange = (this->amount * 0.00005f) + 1;

    amountChange = std::min(amountChange, this->amount); // prevent negative amount

    this->amount -= amountChange;

    float temperatureChangePercent = 0;
    if(this->amount > 0 && amountChange > 0.5f)
        temperatureChangePercent = (amountChange / this->amount);
    
    if(temperatureChangePercent < 0.15f)
        temperatureChangePercent = 0.15f;
    
    this->temperature = Temperature(std::max(this->temperature.GetCelsius(), temperatureChangePercent * 1000));

    //set random fire color
    this->color = FireVoxel::fireColors[rand() % FireVoxel::fireColorCount];
    matrix->GetChunkAtWorldPosition(this->position)->dirtyRender = true;

    if (this->amount <= 5)
    {
        this->amount = std::max(this->amount, 0.1f);

    	if(rand() % 100 < 10) // 10% chance to turn into ash
            this->DieAndReplace(*matrix, "Ash");
        else
            this->DieAndReplace(*matrix, "Carbon_Dioxide");

        return true;
    }
    VoxelLiquid::Step(matrix);
    return true;
}

FireSolidVoxel::FireSolidVoxel(Vec2i position, Temperature temp, float amount, bool isStatic)
    : VoxelSolid("Fire_Solid", position, temp, isStatic, amount)
{
}

bool FireSolidVoxel::Step(ChunkMatrix *matrix)
{
    //check for oxygen and spread
    bool isAroundOxygen = FireVoxel::Spread(matrix, this);

    //flame burns faster with oxygen
    float amountChange;
    if(isAroundOxygen)
        amountChange = (this->amount * 0.00005f) + 0.5f;
    else
        amountChange = (this->amount * 0.000025f) + 0.5f;

    amountChange = std::min(amountChange, this->amount); // prevent negative amount

    this->amount -= amountChange;

    float temperatureChangePercent = 0;
    if(this->amount > 0 && amountChange > 0.5f)
        temperatureChangePercent = (amountChange / this->amount);
    
    if(temperatureChangePercent < 0.15f)
        temperatureChangePercent = 0.15f;
    
    this->temperature = Temperature(std::max(this->temperature.GetCelsius(), temperatureChangePercent * 1000));

    //set random fire color
    this->color = FireVoxel::fireColors[rand() % FireVoxel::fireColorCount];
    matrix->GetChunkAtWorldPosition(this->position)->dirtyRender = true;

    if (this->amount <= 5)  
    {
        this->amount = std::max(this->amount, 0.1f);

        if(rand() % 100 < 10) // 10% chance to turn into ash
            this->DieAndReplace(*matrix, "Ash");
        else
            this->DieAndReplace(*matrix, "Carbon_Dioxide");

        return true;
    }
    VoxelSolid::Step(matrix);
    return true;
}
