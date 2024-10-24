#pragma once
#include "events.h"

#include <ydb/core/tx/columnshard/engines/writer/indexed_blob_constructor.h>
#include <ydb/core/tx/data_events/write_data.h>

#include <ydb/library/actors/core/actor_bootstrapped.h>

#include <util/digest/numeric.h>

namespace NKikimr::NColumnShard::NWriting {

class TAggregationId {
private:
    const ui64 PathId;
    const ui32 SchemaVersion;
    const bool IsDeletion;

public:
    TAggregationId(const ui64 pathId, const ui32 schemaVersion, const bool isDeletion)
        : PathId(pathId)
        , SchemaVersion(schemaVersion)
        , IsDeletion(isDeletion) {
    }

    ui64 CalcHash() const {
        return CombineHashes(PathId | (IsDeletion ? ((ui64)1 << 63) : 0), SchemaVersion);
    }
};

class TAggregationUnit {
private:
    NEvWrite::TWriteData WriteData;
    YDB_READONLY_DEF(std::shared_ptr<arrow::RecordBatch>, Batch);
    YDB_READONLY_DEF(std::shared_ptr<NOlap::TWritingContext>, Context);

public:
    NEvWrite::TWriteData&& ExportWriteData() {
        return std::move(WriteData);
    }

    TAggregationUnit(
        NEvWrite::TWriteData&& writeData, const std::shared_ptr<arrow::RecordBatch>& batch, std::shared_ptr<NOlap::TWritingContext>& context)
        : WriteData(std::move(writeData))
        , Batch(batch)
        , Context(context) {
    }
};

class TAggregation {
private:
    std::vector<TAggregationUnit> Units;
    YDB_READONLY(TMonotonic, Start, TMonotonic::Now());
    YDB_READONLY(ui64, SumBytes, 0);
    YDB_READONLY(ui64, SumRecords, 0);
    bool Flushed = false;

public:
    void Add(
        NEvWrite::TWriteData&& writeData, const std::shared_ptr<arrow::RecordBatch>& batch, std::shared_ptr<NOlap::TWritingContext>& context) {
        Units.emplace_back(std::move(writeData), batch, context);
        SumBytes += NArrow::GetBatchMemorySize(batch);
        SumRecords += batch->num_rows();
    }

    bool CanTakeMore() const {
        return Units.size() < 500 && SumBytes < (100 << 20) && SumRecords < 1000000;
    }

    void Flush();
};

class TAggregations {
private:
    THashMap<TAggregationId, TAggregation> Packs;
    std::map<TMonotonic, TAggregationId> Queue;

public:
    [[nodiscard]] void Add(
        NEvWrite::TWriteData&& writeData, const std::shared_ptr<arrow::RecordBatch>& batch, std::shared_ptr<NOlap::TWritingContext>& context) {
        const ui64 pathId = writeData.GetWriteMeta().GetTableId();
        const ui32 schemaVersion = context->GetActualSchema()->GetSchemaVersion();
        TAggregationId id(pathId, schemaVersion, writeData.GetWriteMeta().GetModificationType() == NEvWrite::EModificationType::Delete);
        auto it = Packs.find(id);
        if (it == Packs.end()) {
            it = Packs.emplace(id, TAggregation()).first;
            Queue.emplace(it->second->GetStart(), it->first);
        }
        it->second.Add(std::move(writeData, batch, context));
        if (!it->second.CanTakeMore()) {
            it->second.Flush();
            Packs.erase(it);
        }
    }

    void Flush(const TMonotonic current, const TDuration d) {
        while (Queue.size() && Queue.begin()->first + d < current) {
            auto it = Packs.find(Queue.begin()->second);
            AFL_VERIFY(it != Packs.end());
            it->second.Flush();
            Packs.erase(it);
            Queue.erase(Queue.begin());
        }
    }
};

class TActor: public TActorBootstrapped<TActor> {
private:
    std::vector<std::shared_ptr<NOlap::TWriteAggregation>> Aggregations;
    TAggregations NoTxAggregations;

    const ui64 TabletId;
    NActors::TActorId ParentActorId;
    TDuration FlushDuration = TDuration::Zero();
    TDuration NoTxFlushDuration = TDuration::MilliSeconds(100);
    ui64 SumSize = 0;
    void Flush();

public:
    TActor(ui64 tabletId, const TActorId& parent);
    ~TActor() = default;

    void Handle(TEvFlushNoTxBuffer::TPtr& ev);
    void Handle(TEvAddInsertedDataToBuffer::TPtr& ev);
    void Handle(TEvFlushBuffer::TPtr& ev);
    void Bootstrap();

    STFUNC(StateWait) {
        TLogContextGuard gLogging(
            NActors::TLogContextBuilder::Build(NKikimrServices::TX_COLUMNSHARD)("tablet_id", TabletId)("parent", ParentActorId));
        switch (ev->GetTypeRewrite()) {
            cFunc(NActors::TEvents::TEvPoison::EventType, PassAway);
            hFunc(TEvAddInsertedDataToBuffer, Handle);
            hFunc(TEvFlushNoTxBuffer, Handle);
            hFunc(TEvFlushBuffer, Handle);
            default:
                AFL_VERIFY(false);
        }
    }
};

}   // namespace NKikimr::NColumnShard::NWriting
