#include "WorldConfig.h"

bool WorldConfig::Validate() const noexcept
{
    if (tickRate == 0 || tickRate > 240) return false;
    if (snapshotRate == 0 || snapshotRate > 240) return false;
    if (boundsMin.x >= boundsMax.x || boundsMin.z >= boundsMax.z) return false;
    if (maxPlayers == 0) return false;

    return true;
}