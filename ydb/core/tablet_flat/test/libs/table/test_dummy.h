#pragma once

#include <ydb/core/tablet_flat/flat_part_iface.h>
#include <ydb/core/tablet_flat/flat_table_misc.h>
#include <ydb/core/tablet_flat/util_fmt_abort.h>

namespace NKikimr {
namespace NTable {

    class TDummyEnv: public IPages {
    public:
        TResult Locate(const TMemTable *memTable, ui64 ref, ui32 tag) override
        {
            return MemTableRefLookup(memTable, ref, tag);
        }

        TResult Locate(const TPart*, ui64, ELargeObj) override
        {
            Y_TABLET_ERROR("Dummy env cannot deal with storage");
        }

        const TSharedData* TryGetPage(const TPart*, TPageId, TGroupId) override
        {
             Y_TABLET_ERROR("Dummy env cannot deal with storage");
        }
    };

}
}
