#include "Math/Temperature.h"
#include "Temperature.h"

using namespace Volume;

Temperature const Temperature::absoluteZero = Temperature(-273.15f);
std::ostream &Volume::operator<<(std::ostream &os, const Temperature &temp)
{
    os << temp.GetCelsius() << " °C";
    return os;
}