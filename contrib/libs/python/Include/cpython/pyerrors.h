#pragma once

#ifdef USE_PYTHON3
#include <contrib/tools/python3/Include/cpython/pyerrors.h>
#else
#error "No <cpython/pyerrors.h> in Python2"
#endif
