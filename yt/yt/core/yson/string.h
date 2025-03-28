#pragma once

#include "public.h"

#include <yt/yt/core/misc/serialize.h>

#include <library/cpp/yt/yson_string/string.h>

namespace NYT::NYson {

////////////////////////////////////////////////////////////////////////////////

void ValidateYson(const TYsonStringBuf& str, int nestingLevelLimit);

////////////////////////////////////////////////////////////////////////////////

struct TBinaryYsonStringSerializer
{
    static void Save(TStreamSaveContext& context, const TYsonString& str);
    static void Load(TStreamLoadContext& context, TYsonString& str);
};

////////////////////////////////////////////////////////////////////////////////

void Serialize(const TYsonString& yson, IYsonConsumer* consumer);
void Serialize(const TYsonStringBuf& yson, IYsonConsumer* consumer);

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NYson

namespace NYT {

////////////////////////////////////////////////////////////////////////////////

template <class C>
struct TSerializerTraits<NYson::TYsonString, C, void>
{
    using TSerializer = NYson::TBinaryYsonStringSerializer;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT
