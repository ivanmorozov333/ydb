#pragma once

#include <ydb/library/actors/core/event_local.h>
#include <ydb/core/tx/columnshard/columnshard_private_events.h>
#include <ydb/core/tx/data_events/write_data.h>
#include <ydb/core/formats/arrow/size_calcer.h>
#include <ydb/library/actors/testlib/common/events_scheduling.h>

namespace NKikimr::NOlap {
class TWritingContext;
}

namespace NKikimr::NColumnShard::NWriting {

class TEvAddInsertedDataToBuffer: public NActors::TEventLocal<TEvAddInsertedDataToBuffer, NColumnShard::TEvPrivate::EEv::EvWritingAddDataToBuffer> {
private:
    YDB_READONLY_DEF(std::shared_ptr<NEvWrite::TWriteData>, WriteData);
    YDB_READONLY_DEF(std::shared_ptr<arrow::RecordBatch>, RecordBatch);
    YDB_ACCESSOR_DEF(std::vector<NArrow::TSerializedBatch>, BlobsToWrite);
    YDB_ACCESSOR_DEF(std::shared_ptr<NOlap::TWritingContext>, Context);

public:
    explicit TEvAddInsertedDataToBuffer(const std::shared_ptr<NEvWrite::TWriteData>& writeData,
        const std::shared_ptr<arrow::RecordBatch>& recordBatch, const std::shared_ptr<NOlap::TWritingContext>& context)
        : WriteData(writeData)
        , RecordBatch(recordBatch)
        , Context(context) {
    }

    explicit TEvAddInsertedDataToBuffer(const std::shared_ptr<NEvWrite::TWriteData>& writeData, std::vector<NArrow::TSerializedBatch>&& blobs,
        const std::shared_ptr<arrow::RecordBatch>& recordBatch)
        : WriteData(writeData)
        , RecordBatch(recordBatch)
        , BlobsToWrite(blobs) {
    }
};

class TEvFlushNoTxBuffer: public NActors::TEventLocal<TEvFlushNoTxBuffer, NColumnShard::TEvPrivate::EEv::EvFlushNoTxBuffer> {
private:
public:
};


class TEvFlushBuffer: public NActors::TEventLocal<TEvFlushBuffer, NColumnShard::TEvPrivate::EEv::EvWritingFlushBuffer> {
private:
    static inline NActors::NTests::TGlobalScheduledEvents::TRegistrator TestScheduledEventRegistrator = (ui32)NColumnShard::TEvPrivate::EEv::EvWritingFlushBuffer;
public:
};


}
