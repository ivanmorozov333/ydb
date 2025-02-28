#include "config.h"

namespace NYT::NChaosClient {

////////////////////////////////////////////////////////////////////////////////

void TReplicationCardCacheConfig::Register(TRegistrar registrar)
{
    registrar.Parameter("enable_watching", &TThis::EnableWatching)
        .Default(true)
        .DontSerializeDefault();
}

////////////////////////////////////////////////////////////////////////////////

void TReplicationCardCacheDynamicConfig::Register(TRegistrar registrar)
{
    registrar.Parameter("enable_watching", &TThis::EnableWatching)
        .Optional();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NChaosClient
