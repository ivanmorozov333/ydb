IF (NOT WITH_VALGRIND)
    UNITTEST_FOR(ydb/core/tx/schemeshard)

    FORK_SUBTESTS()

    SPLIT_FACTOR(20)

    IF (SANITIZER_TYPE OR WITH_VALGRIND)
        SIZE(LARGE)
        INCLUDE(${ARCADIA_ROOT}/ydb/tests/large.inc)
    ELSE()
        SIZE(LARGE)
        TAG(ya:fat)
    ENDIF()

    PEERDIR(
        library/cpp/getopt
        library/cpp/regex/pcre
        library/cpp/svnversion
        ydb/core/testlib/default
        ydb/core/tx
        ydb/core/tx/schemeshard/ut_helpers
        yql/essentials/public/udf/service/exception_policy
    )

    YQL_LAST_ABI_VERSION()

    SRCS(
        ut_split_merge_reboots.cpp
    )

    END()
ENDIF()
