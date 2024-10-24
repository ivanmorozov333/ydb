#include "tx_blobs_written.h"

#include <ydb/core/tx/columnshard/blob_cache.h>
#include <ydb/core/tx/columnshard/engines/column_engine_logs.h>
#include <ydb/core/tx/columnshard/engines/insert_table/user_data.h>
#include <ydb/core/tx/columnshard/transactions/locks/write.h>

namespace NKikimr::NColumnShard {

bool TTxBlobsWritingFinished::DoExecute(TTransactionContext& txc, const TActorContext&) {
    TMemoryProfileGuard mpg("TTxBlobsWritingFinished::Execute");
    txc.DB.NoMoreReadsForTx();
    CommitSnapshot = Self->GetCurrentSnapshotForInternalModification();
    NActors::TLogContextGuard logGuard =
        NActors::TLogContextBuilder::Build(NKikimrServices::TX_COLUMNSHARD_BLOBS)("tablet_id", Self->TabletID())("tx_state", "execute");
    ACFL_DEBUG("event", "start_execute");
    auto& index = Self->MutableIndexAs<NOlap::TColumnEngineForLogs>();
    for (auto&& pack : Packs) {
        auto opFirst = Self->GetOperationsManager().GetOperationVerified((TOperationWriteId)pack.GetWriteMeta().front().GetWriteId());
        const bool isNoTxWrite = opFirst->GetBehaviour() == EOperationBehaviour::NoTxWrite;
        if (!isNoTxWrite) {
            AFL_VERIFY(pack.GetWriteMeta().size() == 1);
        }
        for (auto&& w : pack.GetWriteMeta()) {
            AFL_VERIFY(Self->TablesManager.IsReadyForWrite(w.GetTableId()));
            AFL_VERIFY(!w.HasLongTxId());
            auto operation = Self->OperationsManager->GetOperationVerified((TOperationWriteId)w.GetWriteId());
            Y_ABORT_UNLESS(operation->GetStatus() == EOperationStatus::Started);
        }
        auto& granule = index.MutableGranuleVerified(pack.GetPathId());
        for (auto&& portion : pack.MutablePortions()) {
            if (isNoTxWrite) {
                static TAtomicCounter Counter = 0;
                portion.GetPortionInfoConstructor()->SetInsertWriteId((TInsertWriteId)Counter.Inc());
            } else {
                AFL_VERIFY(pack.GetWriteMeta().size() == 1);
                portion.GetPortionInfoConstructor()->SetInsertWriteId(Self->InsertTable->BuildNextWriteId(txc));
            }
            pack.AddInsertWriteId(portion.GetPortionInfoConstructor()->GetInsertWriteIdVerified());
            portion.Finalize(Self, txc);
            if (isNoTxWrite) {
                granule.CommitImmediateOnExecute(txc, *CommitSnapshot, portion.GetPortionInfo());
            } else {
                granule.InsertPortionOnExecute(txc, portion.GetPortionInfo());
            }
        }
    }

    NOlap::TBlobManagerDb blobManagerDb(txc.DB);
    if (WritingActions) {
        WritingActions->OnExecuteTxAfterWrite(*Self, blobManagerDb, true);
    }
    std::set<TOperationWriteId> operationIds;
    for (auto&& pack : Packs) {
        for (auto&& w : pack.GetWriteMeta()) {
            auto operation = Self->OperationsManager->GetOperationVerified((TOperationWriteId)w.GetWriteId());
            if (!operationIds.emplace(operation->GetWriteId()).second) {
                continue;
            }
            Y_ABORT_UNLESS(operation->GetStatus() == EOperationStatus::Started);
            operation->OnWriteFinish(txc, pack.GetInsertWriteIds(), operation->GetBehaviour() == EOperationBehaviour::NoTxWrite);
            Self->OperationsManager->LinkInsertWriteIdToOperationWriteId(pack.GetInsertWriteIds(), operation->GetWriteId());
            if (operation->GetBehaviour() == EOperationBehaviour::NoTxWrite) {
                auto ev = NEvents::TDataEvents::TEvWriteResult::BuildCompleted(Self->TabletID());
                Results.emplace_back(std::move(ev), w.GetSource(), operation->GetCookie());
            } else {
                auto& info = Self->OperationsManager->GetLockVerified(operation->GetLockId());
                NKikimrDataEvents::TLock lock;
                lock.SetLockId(operation->GetLockId());
                lock.SetDataShard(Self->TabletID());
                lock.SetGeneration(info.GetGeneration());
                lock.SetCounter(info.GetInternalGenerationCounter());
                lock.SetPathId(w.GetTableId());
                auto ev = NEvents::TDataEvents::TEvWriteResult::BuildCompleted(Self->TabletID(), operation->GetLockId(), lock);
                Results.emplace_back(std::move(ev), w.GetSource(), operation->GetCookie());
            }
        }
    }
    return true;
}

void TTxBlobsWritingFinished::DoComplete(const TActorContext& ctx) {
    TMemoryProfileGuard mpg("TTxBlobsWritingFinished::Complete");
    NActors::TLogContextGuard logGuard =
        NActors::TLogContextBuilder::Build(NKikimrServices::TX_COLUMNSHARD_BLOBS)("tablet_id", Self->TabletID())("tx_state", "complete");
    const auto now = TMonotonic::Now();
    if (WritingActions) {
        WritingActions->OnCompleteTxAfterWrite(*Self, true);
    }

    for (auto&& i : Results) {
        i.DoSendReply(ctx);
    }
    auto& index = Self->MutableIndexAs<NOlap::TColumnEngineForLogs>();
    std::set<ui64> pathIds;
    for (auto&& pack : Packs) {
        auto& granule = index.MutableGranuleVerified(pack.GetPathId());
        auto opFirst = Self->GetOperationsManager().GetOperationVerified((TOperationWriteId)pack.GetWriteMeta().front().GetWriteId());
        const bool isNoTxWrite = opFirst->GetBehaviour() == EOperationBehaviour::NoTxWrite;
        if (!isNoTxWrite) {
            AFL_VERIFY(pack.GetWriteMeta().size() == 1);
        }
        for (auto&& portion : pack.GetPortions()) {
            if (opFirst->GetBehaviour() == EOperationBehaviour::WriteWithLock || isNoTxWrite) {
                if (!isNoTxWrite || Self->GetOperationsManager().HasReadLocks(w.GetTableId())) {
                    auto evWrite = std::make_shared<NOlap::NTxInteractions::TEvWriteWriter>(
                        w.GetTableId(), portion.GetPKBatch(), Self->GetIndexOptional()->GetVersionedIndex().GetPrimaryKey());
                    Self->GetOperationsManager().AddEventForLock(*Self, opFirst->GetLockId(), evWrite);
                }
            }
            granule.InsertPortionOnComplete(portion.GetPortionInfo());
        }
        for (auto&& w : pack.GetWriteMeta()) {
            AFL_VERIFY(!w.HasLongTxId());
            auto op = Self->GetOperationsManager().GetOperationVerified((TOperationWriteId)w.GetWriteId());
            pathIds.emplace(op->GetPathId());
            if (pack.IsNoTxWriting()) {
                AFL_VERIFY(CommitSnapshot);
                Self->OperationsManager->AddTemporaryTxLink(op->GetLockId());
                Self->OperationsManager->CommitTransactionOnComplete(*Self, op->GetLockId(), *CommitSnapshot);
            }
            Self->Counters.GetCSCounters().OnWriteTxComplete(now - w.GetWriteStartInstant());
            Self->Counters.GetCSCounters().OnSuccessWriteResponse();
        }
    }
    Self->SetupCompaction(pathIds);
    Self->Counters.GetTabletCounters()->IncCounter(COUNTER_IMMEDIATE_TX_COMPLETED);
}

TTxBlobsWritingFinished::TTxBlobsWritingFinished(TColumnShard* self, const NKikimrProto::EReplyStatus writeStatus,
    const std::shared_ptr<NOlap::IBlobsWritingAction>& writingActions, std::vector<TInsertedPortions>&& packs,
    const std::vector<TFailedWrite>& fails)
    : TBase(self, "TTxBlobsWritingFinished")
    , PutBlobResult(writeStatus)
    , Packs(std::move(packs))
    , WritingActions(writingActions) {
    Y_UNUSED(PutBlobResult);
    for (auto&& i : fails) {
        auto ev = NEvents::TDataEvents::TEvWriteResult::BuildCompleted(Self->TabletID());
        auto op = Self->GetOperationsManager().GetOperationVerified((TOperationWriteId)i.GetWriteMeta().GetWriteId());
        Results.emplace_back(std::move(ev), i.GetWriteMeta().GetSource(), op->GetCookie());
    }
}

}   // namespace NKikimr::NColumnShard
