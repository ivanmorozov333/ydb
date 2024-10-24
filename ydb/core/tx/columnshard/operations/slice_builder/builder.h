#pragma once
#include <ydb/core/formats/arrow/size_calcer.h>
#include <ydb/core/tx/columnshard/columnshard_private_events.h>
#include <ydb/core/tx/columnshard/engines/scheme/versions/abstract_scheme.h>
#include <ydb/core/tx/columnshard/engines/writer/buffer/actor.h>
#include <ydb/core/tx/columnshard/operations/common/context.h>
#include <ydb/core/tx/conveyor/usage/abstract.h>
#include <ydb/core/tx/data_events/write_data.h>

namespace NKikimr::NOlap {

class TBuildSlicesTask: public NConveyor::ITask {
private:
    std::vector<TAggregationUnit> Units;
    const ui64 TabletId;
    const NActors::TActorId BufferActorId;
    std::optional<std::vector<NArrow::TSerializedBatch>> BuildSlices();
    void ReplyError(const TString& message, const NColumnShard::TEvPrivate::TEvWriteBlobsResult::EErrorClass errorClass);

protected:
    virtual TConclusionStatus DoExecute(const std::shared_ptr<ITask>& taskPtr) override;

public:
    virtual TString GetTaskClassIdentifier() const override {
        return "Write::ConstructBlobs::Slices";
    }

    TBuildSlicesTask(const NActors::TActorId bufferActorId, std::vector<TAggregationUnit>&& units)
        : Units(std::move(units))
        , TabletId(WriteData.GetWriteMeta().GetTableId())
        , BufferActorId(bufferActorId) {
    }
};
}   // namespace NKikimr::NOlap
