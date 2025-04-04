# Generated by devtools/yamaker (pypi).

PY3_LIBRARY()

VERSION(3.12.0)

LICENSE(BSD-3-Clause)

PEERDIR(
    contrib/python/wcwidth
)

NO_LINT()

PY_SRCS(
    TOP_LEVEL
    prettytable/__init__.py
    prettytable/_version.py
    prettytable/colortable.py
    prettytable/prettytable.py
)

RESOURCE_FILES(
    PREFIX contrib/python/prettytable/py3/
    .dist-info/METADATA
    .dist-info/top_level.txt
    prettytable/py.typed
)

END()

RECURSE_FOR_TESTS(
    tests
)
