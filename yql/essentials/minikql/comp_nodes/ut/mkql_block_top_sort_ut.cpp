#include "mkql_computation_node_ut.h"
#include <yql/essentials/minikql/mkql_runtime_version.h>

#include <yql/essentials/minikql/computation/mkql_computation_node_holders.h>

#include <cstring>

namespace NKikimr {
namespace NMiniKQL {

Y_UNIT_TEST_SUITE(TMiniKQLBlockTopTest) {
    Y_UNIT_TEST_LLVM(TopByFirstKeyAsc) {
        TSetup<LLVM> setup;
        TProgramBuilder& pb = *setup.PgmBuilder;

        const auto dataType = pb.NewDataType(NUdf::TDataType<const char*>::Id);
        const auto tupleType = pb.NewTupleType({dataType, dataType});

        const auto keyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("key one");
        const auto keyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("key two");

        const auto longKeyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key one");
        const auto longKeyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key two");

        const auto value1 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 1");
        const auto value2 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 2");
        const auto value3 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 3");
        const auto value4 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 4");
        const auto value5 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 5");
        const auto value6 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 6");
        const auto value7 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 7");
        const auto value8 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 8");
        const auto value9 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 9");

        const auto data1 = pb.NewTuple(tupleType, {keyOne, value1});

        const auto data2 = pb.NewTuple(tupleType, {keyTwo, value2});
        const auto data3 = pb.NewTuple(tupleType, {keyTwo, value3});

        const auto data4 = pb.NewTuple(tupleType, {longKeyOne, value4});

        const auto data5 = pb.NewTuple(tupleType, {longKeyTwo, value5});
        const auto data6 = pb.NewTuple(tupleType, {longKeyTwo, value6});
        const auto data7 = pb.NewTuple(tupleType, {longKeyTwo, value7});
        const auto data8 = pb.NewTuple(tupleType, {longKeyTwo, value8});
        const auto data9 = pb.NewTuple(tupleType, {longKeyTwo, value9});

        const auto list = pb.NewList(tupleType, {data1, data2, data3, data4, data5, data6, data7, data8, data9});

        const auto wideFlow = pb.ExpandMap(pb.ToFlow(list),
            [&](TRuntimeNode item) -> TRuntimeNode::TList { return {pb.Nth(item, 0U), pb.Nth(item, 1U)}; });
        const auto blockStream = pb.WideToBlocks(pb.FromFlow(wideFlow));
        const auto topBlocks = pb.WideTopBlocks(blockStream,
            pb.NewDataLiteral<ui64>(4ULL), {{0U, pb.NewDataLiteral<bool>(true)}});
        const auto topFlow = pb.ToFlow(pb.WideFromBlocks(topBlocks));
        const auto pgmReturn = pb.Collect(pb.NarrowMap(topFlow,
            [&](TRuntimeNode::TList items) -> TRuntimeNode { return pb.NewTuple(tupleType, items); }
        ));

        const auto graph = setup.BuildGraph(pgmReturn);
        const auto iterator = graph->GetValue().GetListIterator();
        NUdf::TUnboxedValue item;
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 1");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 3");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 2");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 4");
        UNIT_ASSERT(!iterator.Next(item));
        UNIT_ASSERT(!iterator.Next(item));
    }

    Y_UNIT_TEST_LLVM(TopByFirstKeyDesc) {
        TSetup<LLVM> setup;
        TProgramBuilder& pb = *setup.PgmBuilder;

        const auto dataType = pb.NewDataType(NUdf::TDataType<const char*>::Id);
        const auto tupleType = pb.NewTupleType({dataType, dataType});

        const auto keyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("key one");
        const auto keyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("key two");

        const auto longKeyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key one");
        const auto longKeyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key two");

        const auto value1 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 1");
        const auto value2 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 2");
        const auto value3 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 3");
        const auto value4 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 4");
        const auto value5 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 5");
        const auto value6 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 6");
        const auto value7 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 7");
        const auto value8 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 8");
        const auto value9 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 9");

        const auto data1 = pb.NewTuple(tupleType, {keyOne, value1});

        const auto data2 = pb.NewTuple(tupleType, {keyTwo, value2});
        const auto data3 = pb.NewTuple(tupleType, {keyTwo, value3});

        const auto data4 = pb.NewTuple(tupleType, {longKeyOne, value4});

        const auto data5 = pb.NewTuple(tupleType, {longKeyTwo, value5});
        const auto data6 = pb.NewTuple(tupleType, {longKeyTwo, value6});
        const auto data7 = pb.NewTuple(tupleType, {longKeyTwo, value7});
        const auto data8 = pb.NewTuple(tupleType, {longKeyTwo, value8});
        const auto data9 = pb.NewTuple(tupleType, {longKeyTwo, value9});

        const auto list = pb.NewList(tupleType, {data1, data2, data3, data4, data5, data6, data7, data8, data9});

        const auto wideFlow = pb.ExpandMap(pb.ToFlow(list),
            [&](TRuntimeNode item) -> TRuntimeNode::TList { return {pb.Nth(item, 0U), pb.Nth(item, 1U)}; });
        const auto blockStream = pb.WideToBlocks(pb.FromFlow(wideFlow));
        const auto topBlocks = pb.WideTopBlocks(blockStream,
            pb.NewDataLiteral<ui64>(6ULL), {{0U, pb.NewDataLiteral<bool>(false)}});
        const auto topFlow = pb.ToFlow(pb.WideFromBlocks(topBlocks));
        const auto pgmReturn = pb.Collect(pb.NarrowMap(topFlow,
            [&](TRuntimeNode::TList items) -> TRuntimeNode { return pb.NewTuple(tupleType, items); }
        ));

        const auto graph = setup.BuildGraph(pgmReturn);
        const auto iterator = graph->GetValue().GetListIterator();
        NUdf::TUnboxedValue item;
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 9");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 8");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 6");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 5");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 7");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 4");
        UNIT_ASSERT(!iterator.Next(item));
        UNIT_ASSERT(!iterator.Next(item));
    }

    Y_UNIT_TEST_LLVM(TopBySecondKeyAsc) {
        TSetup<LLVM> setup;
        TProgramBuilder& pb = *setup.PgmBuilder;

        const auto dataType = pb.NewDataType(NUdf::TDataType<const char*>::Id);
        const auto tupleType = pb.NewTupleType({dataType, dataType});

        const auto keyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("key one");
        const auto keyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("key two");

        const auto longKeyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key one");
        const auto longKeyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key two");

        const auto value1 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 1");
        const auto value2 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 2");
        const auto value3 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 3");
        const auto value4 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 4");
        const auto value5 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 5");
        const auto value6 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 6");
        const auto value7 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 7");
        const auto value8 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 8");
        const auto value9 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 9");

        const auto data1 = pb.NewTuple(tupleType, {keyOne, value1});

        const auto data2 = pb.NewTuple(tupleType, {keyTwo, value2});
        const auto data3 = pb.NewTuple(tupleType, {keyTwo, value3});

        const auto data4 = pb.NewTuple(tupleType, {longKeyOne, value4});

        const auto data5 = pb.NewTuple(tupleType, {longKeyTwo, value5});
        const auto data6 = pb.NewTuple(tupleType, {longKeyTwo, value6});
        const auto data7 = pb.NewTuple(tupleType, {longKeyTwo, value7});
        const auto data8 = pb.NewTuple(tupleType, {longKeyTwo, value8});
        const auto data9 = pb.NewTuple(tupleType, {longKeyTwo, value9});

        const auto list = pb.NewList(tupleType, {data1, data2, data3, data4, data5, data6, data7, data8, data9});

        const auto wideFlow = pb.ExpandMap(pb.ToFlow(list),
            [&](TRuntimeNode item) -> TRuntimeNode::TList { return {pb.Nth(item, 0U), pb.Nth(item, 1U)}; });
        const auto blockStream = pb.WideToBlocks(pb.FromFlow(wideFlow));
        const auto topBlocks = pb.WideTopBlocks(blockStream,
            pb.NewDataLiteral<ui64>(3ULL), {{1U, pb.NewDataLiteral<bool>(true)}});
        const auto topFlow = pb.ToFlow(pb.WideFromBlocks(topBlocks));
        const auto pgmReturn = pb.Collect(pb.NarrowMap(topFlow,
            [&](TRuntimeNode::TList items) -> TRuntimeNode { return pb.NewTuple(tupleType, items); }
        ));

        const auto graph = setup.BuildGraph(pgmReturn);
        const auto iterator = graph->GetValue().GetListIterator();
        NUdf::TUnboxedValue item;
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 1");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 2");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 3");
        UNIT_ASSERT(!iterator.Next(item));
        UNIT_ASSERT(!iterator.Next(item));
    }

    Y_UNIT_TEST_LLVM(TopBySecondKeyDesc) {
        TSetup<LLVM> setup;
        TProgramBuilder& pb = *setup.PgmBuilder;

        const auto dataType = pb.NewDataType(NUdf::TDataType<const char*>::Id);
        const auto tupleType = pb.NewTupleType({dataType, dataType});

        const auto keyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("key one");
        const auto keyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("key two");

        const auto longKeyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key one");
        const auto longKeyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key two");

        const auto value1 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 1");
        const auto value2 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 2");
        const auto value3 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 3");
        const auto value4 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 4");
        const auto value5 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 5");
        const auto value6 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 6");
        const auto value7 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 7");
        const auto value8 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 8");
        const auto value9 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 9");

        const auto data1 = pb.NewTuple(tupleType, {keyOne, value1});

        const auto data2 = pb.NewTuple(tupleType, {keyTwo, value2});
        const auto data3 = pb.NewTuple(tupleType, {keyTwo, value3});

        const auto data4 = pb.NewTuple(tupleType, {longKeyOne, value4});

        const auto data5 = pb.NewTuple(tupleType, {longKeyTwo, value5});
        const auto data6 = pb.NewTuple(tupleType, {longKeyTwo, value6});
        const auto data7 = pb.NewTuple(tupleType, {longKeyTwo, value7});
        const auto data8 = pb.NewTuple(tupleType, {longKeyTwo, value8});
        const auto data9 = pb.NewTuple(tupleType, {longKeyTwo, value9});

        const auto list = pb.NewList(tupleType, {data1, data2, data3, data4, data5, data6, data7, data8, data9});

        const auto wideFlow = pb.ExpandMap(pb.ToFlow(list),
            [&](TRuntimeNode item) -> TRuntimeNode::TList { return {pb.Nth(item, 0U), pb.Nth(item, 1U)}; });
        const auto blockStream = pb.WideToBlocks(pb.FromFlow(wideFlow));
        const auto topBlocks = pb.WideTopBlocks(blockStream,
            pb.NewDataLiteral<ui64>(2ULL), {{1U, pb.NewDataLiteral<bool>(false)}});
        const auto topFlow = pb.ToFlow(pb.WideFromBlocks(topBlocks));
        const auto pgmReturn = pb.Collect(pb.NarrowMap(topFlow,
            [&](TRuntimeNode::TList items) -> TRuntimeNode { return pb.NewTuple(tupleType, items); }
        ));

        const auto graph = setup.BuildGraph(pgmReturn);
        const auto iterator = graph->GetValue().GetListIterator();
        NUdf::TUnboxedValue item;
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 9");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 8");
        UNIT_ASSERT(!iterator.Next(item));
        UNIT_ASSERT(!iterator.Next(item));
    }

    Y_UNIT_TEST_LLVM(TopSortByFirstSecondAscDesc) {
        TSetup<LLVM> setup;
        TProgramBuilder& pb = *setup.PgmBuilder;

        const auto dataType = pb.NewDataType(NUdf::TDataType<const char*>::Id);
        const auto tupleType = pb.NewTupleType({dataType, dataType});

        const auto keyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("key one");
        const auto keyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("key two");

        const auto longKeyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key one");
        const auto longKeyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key two");

        const auto value1 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 1");
        const auto value2 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 2");
        const auto value3 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 3");
        const auto value4 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 4");
        const auto value5 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 5");
        const auto value6 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 6");
        const auto value7 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 7");
        const auto value8 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 8");
        const auto value9 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 9");

        const auto data1 = pb.NewTuple(tupleType, {keyOne, value1});

        const auto data2 = pb.NewTuple(tupleType, {keyTwo, value2});
        const auto data3 = pb.NewTuple(tupleType, {keyTwo, value3});

        const auto data4 = pb.NewTuple(tupleType, {longKeyOne, value4});

        const auto data5 = pb.NewTuple(tupleType, {longKeyTwo, value5});
        const auto data6 = pb.NewTuple(tupleType, {longKeyTwo, value6});
        const auto data7 = pb.NewTuple(tupleType, {longKeyTwo, value7});
        const auto data8 = pb.NewTuple(tupleType, {longKeyTwo, value8});
        const auto data9 = pb.NewTuple(tupleType, {longKeyTwo, value9});

        const auto list = pb.NewList(tupleType, {data1, data2, data3, data4, data5, data6, data7, data8, data9});

        const auto wideFlow = pb.ExpandMap(pb.ToFlow(list),
            [&](TRuntimeNode item) -> TRuntimeNode::TList { return {pb.Nth(item, 0U), pb.Nth(item, 1U)}; });
        const auto blockStream = pb.WideToBlocks(pb.FromFlow(wideFlow));
        const auto topSortBlocks = pb.WideTopSortBlocks(blockStream,
            pb.NewDataLiteral<ui64>(4ULL), {{0U, pb.NewDataLiteral<bool>(true)}, {1U, pb.NewDataLiteral<bool>(false)}});
        const auto topSortFlow = pb.ToFlow(pb.WideFromBlocks(topSortBlocks));
        const auto pgmReturn = pb.Collect(pb.NarrowMap(topSortFlow,
            [&](TRuntimeNode::TList items) -> TRuntimeNode { return pb.NewTuple(tupleType, items); }
        ));

        const auto graph = setup.BuildGraph(pgmReturn);
        const auto iterator = graph->GetValue().GetListIterator();
        NUdf::TUnboxedValue item;
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 1");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 3");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 2");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 4");
        UNIT_ASSERT(!iterator.Next(item));
        UNIT_ASSERT(!iterator.Next(item));
    }

    Y_UNIT_TEST_LLVM(TopSortByFirstSecondDescAsc) {
        TSetup<LLVM> setup;
        TProgramBuilder& pb = *setup.PgmBuilder;

        const auto dataType = pb.NewDataType(NUdf::TDataType<const char*>::Id);
        const auto tupleType = pb.NewTupleType({dataType, dataType});

        const auto keyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("key one");
        const auto keyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("key two");

        const auto longKeyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key one");
        const auto longKeyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key two");

        const auto value1 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 1");
        const auto value2 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 2");
        const auto value3 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 3");
        const auto value4 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 4");
        const auto value5 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 5");
        const auto value6 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 6");
        const auto value7 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 7");
        const auto value8 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 8");
        const auto value9 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 9");

        const auto data1 = pb.NewTuple(tupleType, {keyOne, value1});

        const auto data2 = pb.NewTuple(tupleType, {keyTwo, value2});
        const auto data3 = pb.NewTuple(tupleType, {keyTwo, value3});

        const auto data4 = pb.NewTuple(tupleType, {longKeyOne, value4});

        const auto data5 = pb.NewTuple(tupleType, {longKeyTwo, value5});
        const auto data6 = pb.NewTuple(tupleType, {longKeyTwo, value6});
        const auto data7 = pb.NewTuple(tupleType, {longKeyTwo, value7});
        const auto data8 = pb.NewTuple(tupleType, {longKeyTwo, value8});
        const auto data9 = pb.NewTuple(tupleType, {longKeyTwo, value9});

        const auto list = pb.NewList(tupleType, {data1, data2, data3, data4, data5, data6, data7, data8, data9});

        const auto wideFlow = pb.ExpandMap(pb.ToFlow(list),
            [&](TRuntimeNode item) -> TRuntimeNode::TList { return {pb.Nth(item, 0U), pb.Nth(item, 1U)}; });
        const auto blockStream = pb.WideToBlocks(pb.FromFlow(wideFlow));
        const auto topSortBlocks = pb.WideTopSortBlocks(blockStream,
            pb.NewDataLiteral<ui64>(6ULL), {{0U, pb.NewDataLiteral<bool>(false)}, {1U, pb.NewDataLiteral<bool>(true)}});
        const auto topSortFlow = pb.ToFlow(pb.WideFromBlocks(topSortBlocks));
        const auto pgmReturn = pb.Collect(pb.NarrowMap(topSortFlow,
            [&](TRuntimeNode::TList items) -> TRuntimeNode { return pb.NewTuple(tupleType, items); }
        ));

        const auto graph = setup.BuildGraph(pgmReturn);
        const auto iterator = graph->GetValue().GetListIterator();
        NUdf::TUnboxedValue item;
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 5");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 6");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 7");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 8");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 9");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 4");
        UNIT_ASSERT(!iterator.Next(item));
        UNIT_ASSERT(!iterator.Next(item));
    }

    Y_UNIT_TEST_LLVM(TopSortBySecondFirstAscDesc) {
        TSetup<LLVM> setup;
        TProgramBuilder& pb = *setup.PgmBuilder;

        const auto dataType = pb.NewDataType(NUdf::TDataType<const char*>::Id);
        const auto tupleType = pb.NewTupleType({dataType, dataType});

        const auto keyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("key one");
        const auto keyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("key two");

        const auto longKeyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key one");
        const auto longKeyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key two");

        const auto value1 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 1");
        const auto value2 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 2");
        const auto value3 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 3");
        const auto value4 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 4");
        const auto value5 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 5");
        const auto value6 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 6");
        const auto value7 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 7");
        const auto value8 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 8");
        const auto value9 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 9");

        const auto data1 = pb.NewTuple(tupleType, {keyOne, value1});

        const auto data2 = pb.NewTuple(tupleType, {keyTwo, value2});
        const auto data3 = pb.NewTuple(tupleType, {keyTwo, value3});

        const auto data4 = pb.NewTuple(tupleType, {longKeyOne, value4});

        const auto data5 = pb.NewTuple(tupleType, {longKeyTwo, value5});
        const auto data6 = pb.NewTuple(tupleType, {longKeyTwo, value6});
        const auto data7 = pb.NewTuple(tupleType, {longKeyTwo, value7});
        const auto data8 = pb.NewTuple(tupleType, {longKeyTwo, value8});
        const auto data9 = pb.NewTuple(tupleType, {longKeyTwo, value9});

        const auto list = pb.NewList(tupleType, {data1, data2, data3, data4, data5, data6, data7, data8, data9});

        const auto wideFlow = pb.ExpandMap(pb.ToFlow(list),
            [&](TRuntimeNode item) -> TRuntimeNode::TList { return {pb.Nth(item, 0U), pb.Nth(item, 1U)}; });
        const auto blockStream = pb.WideToBlocks(pb.FromFlow(wideFlow));
        const auto topSortBlocks = pb.WideTopSortBlocks(blockStream,
                                                        pb.NewDataLiteral<ui64>(4ULL),
                                                        {{1U, pb.NewDataLiteral<bool>(true)},
                                                         {0U, pb.NewDataLiteral<bool>(false)}});
        const auto topSortFlow = pb.ToFlow(pb.WideFromBlocks(topSortBlocks));
        const auto pgmReturn = pb.Collect(pb.NarrowMap(topSortFlow,
            [&](TRuntimeNode::TList items) -> TRuntimeNode { return pb.NewTuple(tupleType, items); }
        ));

        const auto graph = setup.BuildGraph(pgmReturn);
        const auto iterator = graph->GetValue().GetListIterator();
        NUdf::TUnboxedValue item;
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 1");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 2");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 3");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 4");
        UNIT_ASSERT(!iterator.Next(item));
        UNIT_ASSERT(!iterator.Next(item));
    }

    Y_UNIT_TEST_LLVM(TopSortBySecondFirstDescAsc) {
        TSetup<LLVM> setup;
        TProgramBuilder& pb = *setup.PgmBuilder;

        const auto dataType = pb.NewDataType(NUdf::TDataType<const char*>::Id);
        const auto tupleType = pb.NewTupleType({dataType, dataType});

        const auto keyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("key one");
        const auto keyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("key two");

        const auto longKeyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key one");
        const auto longKeyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key two");

        const auto value1 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 1");
        const auto value2 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 2");
        const auto value3 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 3");
        const auto value4 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 4");
        const auto value5 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 5");
        const auto value6 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 6");
        const auto value7 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 7");
        const auto value8 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 8");
        const auto value9 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 9");

        const auto data1 = pb.NewTuple(tupleType, {keyOne, value1});

        const auto data2 = pb.NewTuple(tupleType, {keyTwo, value2});
        const auto data3 = pb.NewTuple(tupleType, {keyTwo, value3});

        const auto data4 = pb.NewTuple(tupleType, {longKeyOne, value4});

        const auto data5 = pb.NewTuple(tupleType, {longKeyTwo, value5});
        const auto data6 = pb.NewTuple(tupleType, {longKeyTwo, value6});
        const auto data7 = pb.NewTuple(tupleType, {longKeyTwo, value7});
        const auto data8 = pb.NewTuple(tupleType, {longKeyTwo, value8});
        const auto data9 = pb.NewTuple(tupleType, {longKeyTwo, value9});

        const auto list = pb.NewList(tupleType, {data1, data2, data3, data4, data5, data6, data7, data8, data9});

        const auto wideFlow = pb.ExpandMap(pb.ToFlow(list),
            [&](TRuntimeNode item) -> TRuntimeNode::TList { return {pb.Nth(item, 0U), pb.Nth(item, 1U)}; });
        const auto blockStream = pb.WideToBlocks(pb.FromFlow(wideFlow));
        const auto topSortBlocks = pb.WideTopSortBlocks(blockStream,
                                                        pb.NewDataLiteral<ui64>(6ULL),
                                                        {{1U, pb.NewDataLiteral<bool>(false)},
                                                         {0U, pb.NewDataLiteral<bool>(true)}});
        const auto topSortFlow = pb.ToFlow(pb.WideFromBlocks(topSortBlocks));
        const auto pgmReturn = pb.Collect(pb.NarrowMap(topSortFlow,
            [&](TRuntimeNode::TList items) -> TRuntimeNode { return pb.NewTuple(tupleType, items); }
        ));

        const auto graph = setup.BuildGraph(pgmReturn);
        const auto iterator = graph->GetValue().GetListIterator();
        NUdf::TUnboxedValue item;
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 9");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 8");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 7");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 6");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 5");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 4");
        UNIT_ASSERT(!iterator.Next(item));
        UNIT_ASSERT(!iterator.Next(item));
    }
}

Y_UNIT_TEST_SUITE(TMiniKQLBlockSortTest) {
    Y_UNIT_TEST_LLVM(SortByFirstKeyAsc) {
        TSetup<LLVM> setup;
        TProgramBuilder& pb = *setup.PgmBuilder;

        const auto dataType = pb.NewDataType(NUdf::TDataType<const char*>::Id);
        const auto tupleType = pb.NewTupleType({dataType, dataType});

        const auto keyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("key one");
        const auto keyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("key two");

        const auto longKeyOne = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key one");
        const auto longKeyTwo = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long key two");

        const auto value1 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 1");
        const auto value2 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 2");
        const auto value3 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 3");
        const auto value4 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 4");
        const auto value5 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 5");
        const auto value6 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 6");
        const auto value7 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 7");
        const auto value8 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 8");
        const auto value9 = pb.NewDataLiteral<NUdf::EDataSlot::String>("very long value 9");

        const auto data1 = pb.NewTuple(tupleType, {keyOne, value1});

        const auto data2 = pb.NewTuple(tupleType, {keyTwo, value2});
        const auto data3 = pb.NewTuple(tupleType, {keyTwo, value3});

        const auto data4 = pb.NewTuple(tupleType, {longKeyOne, value4});

        const auto data5 = pb.NewTuple(tupleType, {longKeyTwo, value5});
        const auto data6 = pb.NewTuple(tupleType, {longKeyTwo, value6});
        const auto data7 = pb.NewTuple(tupleType, {longKeyTwo, value7});
        const auto data8 = pb.NewTuple(tupleType, {longKeyTwo, value8});
        const auto data9 = pb.NewTuple(tupleType, {longKeyTwo, value9});

        const auto list = pb.NewList(tupleType, {data1, data2, data3, data4, data5, data6, data7, data8, data9});

        const auto wideFlow = pb.ExpandMap(pb.ToFlow(list),
            [&](TRuntimeNode item) -> TRuntimeNode::TList { return {pb.Nth(item, 0U), pb.Nth(item, 1U)}; });
        const auto blockStream = pb.WideToBlocks(pb.FromFlow(wideFlow));
        const auto sortBlocks = pb.WideSortBlocks(blockStream,
            {{0U, pb.NewDataLiteral<bool>(true)}});
        const auto sortFlow = pb.ToFlow(pb.WideFromBlocks(sortBlocks));
        const auto pgmReturn = pb.Collect(pb.NarrowMap(sortFlow,
            [&](TRuntimeNode::TList items) -> TRuntimeNode { return pb.NewTuple(tupleType, items); }
        ));

        const auto graph = setup.BuildGraph(pgmReturn);
        const auto iterator = graph->GetValue().GetListIterator();
        NUdf::TUnboxedValue item;
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 1");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 2");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 3");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key one");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 4");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 5");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 6");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 7");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 8");
        UNIT_ASSERT(iterator.Next(item));
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(0), "very long key two");
        UNBOXED_VALUE_STR_EQUAL(item.GetElement(1), "very long value 9");
        UNIT_ASSERT(!iterator.Next(item));
        UNIT_ASSERT(!iterator.Next(item));
    }
}

}
}
