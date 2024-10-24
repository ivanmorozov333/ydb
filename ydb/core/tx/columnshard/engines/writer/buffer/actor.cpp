#include "actor.h"

#include <ydb/core/tx/columnshard/columnshard_impl.h>
#include <ydb/core/tx/columnshard/columnshard_private_events.h>
#include <ydb/core/tx/columnshard/operations/slice_builder/builder.h>
#include <ydb/core/tx/conveyor/usage/service.h>

namespace NKikimr::NColumnShard::NWriting {

TActor::TActor(ui64 tabletId, const TActorId& parent)
    : TabletId(tabletId)
    , ParentActorId(parent) {
}

void TActor::Bootstrap() {
    Become(&TThis::StateWait);
    Schedule(FlushDuration, new TEvFlushBuffer);
    Schedule(NoTxFlushDuration, new TEvFlushNoTxBuffer);
    FlushDuration = TDuration::MilliSeconds(AppDataVerified().ColumnShardConfig.GetWritingBufferDurationMs());
}

void TActor::Flush() {
    if (Aggregations.size()) {
        AFL_DEBUG(NKikimrServices::TX_COLUMNSHARD)("event", "flush_writing")("size", SumSize)("count", Aggregations.size());
        auto action = Aggregations.front()->GetBlobsAction();
        auto writeController = std::make_shared<NOlap::TIndexedWriteController>(ParentActorId, action, std::move(Aggregations));
        if (action->NeedDraftTransaction()) {
            TActorContext::AsActorContext().Send(ParentActorId, std::make_unique<NColumnShard::TEvPrivate::TEvWriteDraft>(writeController));
        } else {
            TActorContext::AsActorContext().Register(NColumnShard::CreateWriteActor(TabletId, writeController, TInstant::Max()));
        }
        Aggregations.clear();
        SumSize = 0;
    } else {
        AFL_DEBUG(NKikimrServices::TX_COLUMNSHARD)("event", "skip_flush_writing");
    }
}

void TActor::Handle(TEvFlushNoTxBuffer::TPtr& /*ev*/) {
    NoTxAggregations.Flush(TMonotonic::Now(), NoTxFlushDuration);
    Schedule(NoTxFlushDuration, new TEvFlushNoTxBuffer);
}

void TActor::Handle(TEvFlushBuffer::TPtr& /*ev*/) {
    FlushDuration = TDuration::MilliSeconds(AppDataVerified().ColumnShardConfig.GetWritingBufferDurationMs());
    Flush();
    if (!FlushDuration) {
        Schedule(TDuration::MilliSeconds(500), new TEvFlushBuffer);
    } else {
        Schedule(FlushDuration, new TEvFlushBuffer);
    }
}

void TActor::Handle(TEvAddInsertedDataToBuffer::TPtr& ev) {
    auto* evBase = ev->Get();
    if (evBase->GetContext()) {
        NoTxAggregations.Add(evBase->ExtractWriteData(), evBase->GetRecordBatch(), evBase->GetContext());
    } else {
        AFL_VERIFY(evBase->GetWriteData()->GetBlobsAction()->GetStorageId() == NOlap::IStoragesManager::DefaultStorageId);
        SumSize += evBase->GetWriteData()->GetSize();
        Aggregations.emplace_back(std::make_shared<NOlap::TWriteAggregation>(
            *evBase->GetWriteData(), std::move(evBase->MutableBlobsToWrite()), evBase->GetRecordBatch()));
        if (SumSize > 4 * 1024 * 1024 || Aggregations.size() > 750 || !FlushDuration) {
            Flush();
        }
    }
}

void TAggregation::Flush() {
    AFL_VERIFY(!Flushed);
    AFL_VERIFY(Units.size());
    Flushed = true;
    auto task = std::make_shared<NOlap::TBuildSlicesTask>(BufferActorId, std::move(Units));
    NConveyor::TInsertServiceOperator::AsyncTaskToExecute(task);
}

}   // namespace NKikimr::NColumnShard::NWriting

template <>
struct THash<NKikimr::NColumnShard::NWriting::TAggregationId> {
    inline ui64 operator()(const NKikimr::NColumnShard::NWriting::TAggregationId& x) const noexcept {
        return x.CalcHash();
    }
};
