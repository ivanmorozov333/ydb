# Generated by devtools/yamaker (pypi).

PY3_LIBRARY()

VERSION(2.14.3)

LICENSE(Apache-2.0)

PEERDIR(
    contrib/python/allure-python-commons
    contrib/python/pytest
)

NO_LINT()

PY_SRCS(
    TOP_LEVEL
    allure_pytest/__init__.py
    allure_pytest/compat.py
    allure_pytest/helper.py
    allure_pytest/listener.py
    allure_pytest/plugin.py
    allure_pytest/utils.py
)

RESOURCE_FILES(
    PREFIX contrib/python/allure-pytest/
    .dist-info/METADATA
    .dist-info/entry_points.txt
    .dist-info/top_level.txt
)

END()
