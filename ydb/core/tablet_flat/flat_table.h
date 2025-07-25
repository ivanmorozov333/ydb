#pragma once
#include "defs.h"
#include "flat_update_op.h"
#include "flat_dbase_scheme.h"
#include "flat_mem_warm.h"
#include "flat_iterator.h"
#include "flat_row_scheme.h"
#include "flat_row_versions.h"
#include "flat_part_laid.h"
#include "flat_part_slice.h"
#include "flat_table_committed.h"
#include "flat_table_part.h"
#include "flat_table_stats.h"
#include "flat_table_subset.h"
#include "flat_table_misc.h"
#include "flat_table_observer.h"
#include "flat_sausage_solid.h"
#include "util_basics.h"

#include <ydb/core/scheme/scheme_tablecell.h>
#include <library/cpp/containers/absl_flat_hash/flat_hash_map.h>
#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/generic/deque.h>
#include <util/generic/set.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>

namespace NKikimr {
namespace NTable {

class TTableEpochs;
class TKeyRangeCache;
class TKeyRangeCacheNeedGCList;

class TTable: public TAtomicRefCount<TTable> {
public:
    using TOpsRef = TArrayRef<const TUpdateOp>;
    using TMemGlob = NPageCollection::TMemGlob;

    struct TStat {
        /*_ In memory (~memtable) data statistics   */

        ui64 FrozenWaste = 0;
        ui64 FrozenSize = 0;
        ui64 FrozenOps  = 0;
        ui64 FrozenRows = 0;

        /*_ Already flatten data statistics (parts) */

        TPartStats Parts;
        THashMap<ui64, TPartStats> PartsPerTablet;
    };

    struct TReady {
        EReady Ready = EReady::Page;

        /* Per part operation statictics on Charge(...) or Select(...)
            for ByKey bloom filter usage. The filter misses >= NoKey */

        ui64 Weeded = 0;
        ui64 Sieved = 0;
        ui64 NoKey = 0;         /* Examined TPart without the key */

        TIteratorStats Stats;
    };

    explicit TTable(TEpoch, const TIntrusivePtr<TKeyRangeCacheNeedGCList>& gcList = nullptr);
    ~TTable();

    void PrepareRollback();
    void PrepareTruncate();
    void RollbackChanges();
    void CommitChanges(TArrayRef<const TMemGlob> blobs);
    void CommitNewTable(TArrayRef<const TMemGlob> blobs);

    void SetScheme(const TScheme::TTableInfo& tableScheme);

    TIntrusiveConstPtr<TRowScheme> GetScheme() const noexcept;

    TEpoch Snapshot();

    TEpoch Head() const noexcept
    {
        return Epoch;
    }

    TAutoPtr<TSubset> CompactionSubset(TEpoch edge, TArrayRef<const TLogoBlobID> bundle);
    TAutoPtr<TSubset> PartSwitchSubset(TEpoch edge, TArrayRef<const TLogoBlobID> bundle, TArrayRef<const TLogoBlobID> txStatus);
    TAutoPtr<TSubset> Subset(TEpoch edge) const;
    TAutoPtr<TSubset> ScanSnapshot(TRowVersion snapshot = TRowVersion::Max());
    TAutoPtr<TSubset> Unwrap(); /* full Subset(..) + final Replace(..) */

    bool HasBorrowed(ui64 selfTabletId) const;

    /**
     * Returns current slices for bundles
     *
     * Map will only contain bundles that currently exist in the table
     */
    TBundleSlicesMap LookupSlices(TArrayRef<const TLogoBlobID> bundles) const;

    /**
     * Replaces slices for bundles in the slices map
     */
    void ReplaceSlices(TBundleSlicesMap slices);

    /* Interface for redistributing data layout within the table. Take some
        subset with Subset(...) call, do some work and then return result
        with Replace(...) method. The result should hold the same set of rows
        as original subset. Replace(...) may produce some garbage that have to
        be displaced from table with Clean() method eventually.
    */

    void Replace(const TSubset&, TArrayRef<const TPartView>, TArrayRef<const TIntrusiveConstPtr<TTxStatusPart>>);

    /*_ Special interface for clonig flatten part of table for outer usage.
        Cook some TPartView with Subset(...) method and/or TShrink tool first and
        then merge produced TPartView to outer table.
    */

    void Merge(TPartView partView);
    void Merge(TIntrusiveConstPtr<TColdPart> part);
    void Merge(TIntrusiveConstPtr<TTxStatusPart> txStatus);
    void MergeDone();

    /**
     * Returns constructed levels for slices
     */
    const TLevels& GetLevels() const;

    /**
     * Returns search height if there are no cold parts, 0 otherwise
     */
    ui64 GetSearchHeight() const;

    /* Hack for filling external blobs in TMemTable tables with data */

    TVector<TIntrusiveConstPtr<TMemTable>> GetMemTables() const;

    TAutoPtr<TTableIter> Iterate(TRawVals key, TTagsRef tags, IPages* env, ESeek,
            TRowVersion snapshot,
            const ITransactionMapPtr& visible = nullptr,
            const ITransactionObserverPtr& observer = nullptr) const;
    TAutoPtr<TTableReverseIter> IterateReverse(TRawVals key, TTagsRef tags, IPages* env, ESeek,
            TRowVersion snapshot,
            const ITransactionMapPtr& visible = nullptr,
            const ITransactionObserverPtr& observer = nullptr) const;
    EReady Select(TRawVals key, TTagsRef tags, IPages* env, TRowState& row,
                  ui64 flg, TRowVersion snapshot, TDeque<TPartIter>& tempIterators,
                  TSelectStats& stats,
                  const ITransactionMapPtr& visible = nullptr,
                  const ITransactionObserverPtr& observer = nullptr) const;
    TSelectRowVersionResult SelectRowVersion(
            TRawVals key, IPages* env, ui64 readFlags,
            const ITransactionMapPtr& visible = nullptr,
            const ITransactionObserverPtr& observer = nullptr) const;
    TSelectRowVersionResult SelectRowVersion(
            TArrayRef<const TCell> key, IPages* env, ui64 readFlags,
            const ITransactionMapPtr& visible = nullptr,
            const ITransactionObserverPtr& observer = nullptr) const;
    TSelectRowVersionResult SelectRowVersion(
            const TCelled& key, IPages* env, ui64 readFlags,
            const ITransactionMapPtr& visible = nullptr,
            const ITransactionObserverPtr& observer = nullptr) const;

    EReady Precharge(TRawVals minKey, TRawVals maxKey, TTagsRef tags,
                     IPages* env, ui64 flg,
                     ui64 itemsLimit, ui64 bytesLimit,
                     EDirection direction, TRowVersion snapshot, TSelectStats& stats) const;

    void Update(ERowOp, TRawVals key, TOpsRef, TArrayRef<const TMemGlob> apart, TRowVersion rowVersion);

    void UpdateTx(ERowOp, TRawVals key, TOpsRef, TArrayRef<const TMemGlob> apart, ui64 txId);
    void CommitTx(ui64 txId, TRowVersion rowVersion);
    void RemoveTx(ui64 txId);

    /**
     * Returns true when table has an open transaction that is not committed or removed yet
     */
    bool HasOpenTx(ui64 txId) const;
    bool HasTxData(ui64 txId) const;
    bool HasCommittedTx(ui64 txId) const;
    bool HasRemovedTx(ui64 txId) const;

    const absl::flat_hash_set<ui64>& GetOpenTxs() const;
    size_t GetOpenTxCount() const;
    size_t GetTxsWithDataCount() const;
    size_t GetTxsWithStatusCount() const;
    size_t GetCommittedTxCount() const;
    size_t GetRemovedTxCount() const;

    TPartView GetPartView(const TLogoBlobID &bundle) const
    {
        auto *partView = Flatten.FindPtr(bundle);

        return partView ? *partView : TPartView{ };
    }

    TVector<TPartView> GetAllParts() const
    {
        TVector<TPartView> parts(Reserve(Flatten.size()));

        for (auto& x : Flatten) {
            parts.emplace_back(x.second);
        }

        return parts;
    }

    TVector<TIntrusiveConstPtr<TColdPart>> GetColdParts() const
    {
        TVector<TIntrusiveConstPtr<TColdPart>> parts(Reserve(ColdParts.size()));

        for (auto& x : ColdParts) {
            parts.emplace_back(x.second);
        }

        return parts;
    }

    void EnumerateParts(const std::function<void(const TPartView&)>& callback) const
    {
        for (auto& x : Flatten) {
            callback(x.second);
        }
    }

    void EnumerateColdParts(const std::function<void(const TIntrusiveConstPtr<TColdPart>&)>& callback) const
    {
        for (auto& x : ColdParts) {
            callback(x.second);
        }
    }

    void EnumerateTxStatusParts(const std::function<void(const TIntrusiveConstPtr<TTxStatusPart>&)>& callback) const
    {
        for (auto& x : TxStatus) {
            callback(x.second);
        }
    }

    const TStat& Stat() const noexcept
    {
        return Stat_;
    }

    TTableRuntimeStats RuntimeStats() const noexcept;

    ui64 GetMemSize(TEpoch epoch = TEpoch::Max()) const noexcept
    {
        if (Y_LIKELY(epoch == TEpoch::Max())) {
            return Stat_.FrozenSize
                + (Mutable ? Mutable->GetUsedMem() : 0)
                + (MutableBackup ? MutableBackup->GetUsedMem() : 0);
        }

        ui64 size = 0;

        for (const auto& x : Frozen) {
            if (x->Epoch < epoch) {
                size += x->GetUsedMem();
            }
        }

        if (MutableBackup && MutableBackup->Epoch < epoch) {
            size += MutableBackup->GetUsedMem();
        }

        if (Mutable && Mutable->Epoch < epoch) {
            size += Mutable->GetUsedMem();
        }

        return size;
    }

    ui64 GetMemWaste() const noexcept
    {
        return Stat_.FrozenWaste
            + (Mutable ? Mutable->GetWastedMem() : 0)
            + (MutableBackup ? MutableBackup->GetWastedMem() : 0);
    }

    ui64 GetMemRowCount() const noexcept
    {
        return Stat_.FrozenRows
            + (Mutable ? Mutable->GetRowCount() : 0)
            + (MutableBackup ? MutableBackup->GetRowCount() : 0);
    }

    ui64 GetOpsCount() const noexcept
    {
        return Stat_.FrozenOps
            + (Mutable ? Mutable->GetOpsCount() : 0)
            + (MutableBackup ? MutableBackup->GetOpsCount() : 0);
    }

    ui64 GetPartsCount() const noexcept
    {
        return Flatten.size();
    }

    ui64 EstimateRowSize() const noexcept
    {
        ui64 size = Stat_.FrozenSize
            + (Mutable ? Mutable->GetUsedMem() : 0)
            + (MutableBackup ? MutableBackup->GetUsedMem() : 0);
        ui64 rows = Stat_.FrozenRows
            + (Mutable ? Mutable->GetRowCount() : 0)
            + (MutableBackup ? MutableBackup->GetRowCount() : 0);

        for (const auto& flat : Flatten) {
            if (const TPartView &partView = flat.second) {
                size += partView->DataSize();
                rows += partView.Part->Stat.Rows;
            }
        }

        return rows ? (size / rows) : 0;
    }

    void DebugDump(IOutputStream& str, IPages *env, const NScheme::TTypeRegistry& typeRegistry) const;

    TKeyRangeCache* GetErasedKeysCache() const;

    bool RemoveRowVersions(const TRowVersion& lower, const TRowVersion& upper);

    const TRowVersionRanges& GetRemovedRowVersions() const {
        return RemovedRowVersions;
    }

    TCompactionStats GetCompactionStats() const;

    void SetTableObserver(TIntrusivePtr<ITableObserver> ptr);

private:
    TMemTable& MemTable();
    void AddSafe(TPartView partView);

    void AddStat(const TPartView& partView);
    void RemoveStat(const TPartView& partView);

private:
    void AddTxDataRef(ui64 txId);
    void RemoveTxDataRef(ui64 txId);
    void AddTxStatusRef(ui64 txId);
    void RemoveTxStatusRef(ui64 txId);

private:
    TEpoch Epoch; /* Monotonic table change number, with holes */
    ui64 Annexed = 0; /* Monotonic serial of attached external blobs */
    TIntrusiveConstPtr<TRowScheme> Scheme;
    TIntrusivePtr<TMemTable> Mutable;
    TSet<TIntrusiveConstPtr<TMemTable>, TOrderByEpoch<TMemTable>> Frozen;
    THashMap<TLogoBlobID, TPartView> Flatten;
    THashMap<TLogoBlobID, TIntrusiveConstPtr<TColdPart>> ColdParts;
    THashMap<TLogoBlobID, TIntrusiveConstPtr<TTxStatusPart>> TxStatus;
    TEpoch FlattenEpoch = TEpoch::Min(); /* Current maximum flatten epoch */
    TStat Stat_;
    mutable THolder<TLevels> Levels;
    mutable TIntrusivePtr<TKeyRangeCache> ErasedKeysCache;

    bool EraseCacheEnabled = false;
    TKeyRangeCacheConfig EraseCacheConfig;
    const TIntrusivePtr<TKeyRangeCacheNeedGCList> EraseCacheGCList;

    TRowVersionRanges RemovedRowVersions;

    // The number of entities (memtable/sst) that have rows with a TxId. As
    // long as there is at least one row with a TxId its commit/remove status
    // must be preserved.
    absl::flat_hash_map<ui64, size_t> TxDataRefs;

    // The number of entities (memtable/txstatus) that have a commit/remove
    // status for a TxId. As long as there is at least one such entity the
    // transaction cannot be used again without artifacts, and must stay
    // in committed/removed set.
    absl::flat_hash_map<ui64, size_t> TxStatusRefs;

    // A set of open transactions, i.e. transactions that have rows with the
    // specified TxId and that have not been committed or removed yet.
    absl::flat_hash_set<ui64> OpenTxs;

    TTransactionMap CommittedTransactions;
    TTransactionSet RemovedTransactions;
    TTransactionSet DecidedTransactions;
    TTransactionSet GarbageTransactions;
    TIntrusivePtr<ITableObserver> TableObserver;

    ui64 RemovedCommittedTxs = 0;

private:
    struct TRollbackRemoveTxDataRef {
        ui64 TxId;
    };

    struct TRollbackRemoveTxStatusRef {
        ui64 TxId;
    };

    struct TRollbackAddCommittedTx {
        ui64 TxId;
        TRowVersion RowVersion;
    };

    struct TRollbackRemoveCommittedTx {
        ui64 TxId;
    };

    struct TRollbackAddRemovedTx {
        ui64 TxId;
    };

    struct TRollbackRemoveRemovedTx {
        ui64 TxId;
    };

    using TRollbackOp = std::variant<
        TRollbackRemoveTxDataRef,
        TRollbackRemoveTxStatusRef,
        TRollbackAddCommittedTx,
        TRollbackRemoveCommittedTx,
        TRollbackAddRemovedTx,
        TRollbackRemoveRemovedTx>;

    struct TCommitAddDecidedTx {
        ui64 TxId;
    };

    using TCommitOp = std::variant<
        TCommitAddDecidedTx>;

    struct TRollbackState {
        TEpoch Epoch;
        TIntrusiveConstPtr<TRowScheme> Scheme;
        ui64 Annexed;
        TKeyRangeCacheConfig EraseCacheConfig;
        bool EraseCacheEnabled;
        bool MutableExisted;
        bool MutableUpdated;
        bool DisableEraseCache;
        bool Truncated;

        TRollbackState(TEpoch epoch)
            : Epoch(epoch)
        { }
    };

    std::optional<TRollbackState> RollbackState;
    std::vector<TCommitOp> CommitOps;
    std::vector<TRollbackOp> RollbackOps;
    TIntrusivePtr<TMemTable> MutableBackup;
};

}
}
