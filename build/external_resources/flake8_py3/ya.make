RESOURCES_LIBRARY()

IF (HOST_OS_DARWIN AND HOST_ARCH_ARM64 OR
    HOST_OS_DARWIN AND HOST_ARCH_X86_64 OR
    HOST_OS_LINUX AND HOST_ARCH_PPC64LE OR
    HOST_OS_LINUX AND HOST_ARCH_X86_64 OR
    HOST_OS_LINUX AND HOST_ARCH_AARCH64 OR
    HOST_OS_WINDOWS AND HOST_ARCH_X86_64)
ELSE()
    MESSAGE(FATAL_ERROR Unsupported host platform for FLAKE8_PY3)
ENDIF()

DECLARE_EXTERNAL_HOST_RESOURCES_BUNDLE(
    FLAKE8_PY3
    sbr:9245123262 FOR DARWIN-ARM64
    sbr:9245124509 FOR DARWIN
    sbr:6726869647 FOR LINUX-PPC64LE
    sbr:9245126434 FOR LINUX
    sbr:9245122502 FOR LINUX-AARCH64
    sbr:9245125693 FOR WIN32
)

END()
