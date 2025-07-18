# -*- coding: utf-8 -*-
import logging
import os
import time
import requests
import subprocess
from google.protobuf import json_format
import pytest

from hamcrest import assert_that, equal_to, greater_than, not_none

from ydb.core.protos import config_pb2
from ydb.tests.library.common.msgbus_types import MessageBusStatus
from ydb.tests.library.common.protobuf_ss import AlterTableRequest
from ydb.tests.library.harness.kikimr_runner import KiKiMR
from ydb.tests.library.harness.kikimr_config import KikimrConfigGenerator
from ydb.tests.library.harness.util import LogLevels
from ydb.tests.library.fixtures import ydb_database_ctx
from ydb.tests.library.matchers.response_matchers import ProtobufWithStatusMatcher
from ydb.tests.oss.ydb_sdk_import import ydb

logger = logging.getLogger(__name__)


def get_db_counters(mon_port, service):
    counters_url = f"http://localhost:{mon_port}/counters/counters={service}/json"
    reply = requests.get(counters_url)
    if reply.status_code == 204:
        return None

    assert_that(reply.status_code, equal_to(200))
    counters = reply.json()

    assert_that(counters, not_none())
    return counters


class BaseDbCounters(object):
    @classmethod
    def setup_class(cls):
        cls.cluster = KiKiMR(
            KikimrConfigGenerator(
                additional_log_configs={
                    'SYSTEM_VIEWS': LogLevels.DEBUG
                }
            )
        )
        cls.cluster.start()

    @classmethod
    def teardown_class(cls):
        if hasattr(cls, 'cluster'):
            cls.cluster.stop()

    def setup_method(self, method=None):
        self.database = "/Root/users/{class_name}_{method_name}".format(
            class_name=self.__class__.__name__,
            method_name=method.__name__,
        )
        logger.debug("Create database %s" % self.database)
        self.cluster.create_database(
            self.database,
            storage_pool_units_count={
                'hdd': 1
            }
        )

        self.cluster.register_and_start_slots(self.database, count=1)
        self.cluster.wait_tenant_up(self.database)

    def teardown_method(self, method=None):
        logger.debug("Remove database %s" % self.database)
        self.cluster.remove_database(self.database)
        self.database = None

    def create_table(self, driver, table):
        with ydb.SessionPool(driver, size=1) as pool:
            with pool.checkout() as session:
                session.execute_scheme(
                    "create table `{}` (key Int32, value String, primary key(key));".format(
                        table
                    )
                )

    def check_db_counters(self, sensors_to_check, group):
        table = os.path.join(self.database, 'table')

        driver_config = ydb.DriverConfig(
            "%s:%s" % (self.cluster.nodes[1].host, self.cluster.nodes[1].port),
            self.database
        )

        with ydb.Driver(driver_config) as driver:
            self.create_table(driver, table)

            with ydb.SessionPool(driver, size=1) as pool:
                with pool.checkout() as session:
                    query = "select * from `{}`".format(table)
                    session.transaction().execute(query, commit_tx=True)

            for i in range(30):
                checked = 0

                counters = get_db_counters(self.cluster.slots[1].mon_port, 'db')
                if counters:
                    sensors = counters['sensors']
                    for sensor in sensors:
                        labels = sensor['labels']
                        if labels['sensor'] in sensors_to_check:
                            assert_that(labels['group'], equal_to(group))
                            assert_that(labels['database'], equal_to(self.database))
                            assert_that(labels['host'], equal_to(''))
                            assert_that(sensor['value'], greater_than(0))
                            checked = checked + 1

                    assert_that(checked, equal_to(len(sensors_to_check)))
                    break

                if checked > 0:
                    break

                time.sleep(5)


class TestKqpCounters(BaseDbCounters):
    def test_case(self):
        sensors_to_check = {
            'Requests/Bytes',
            'Requests/QueryBytes',
            'Requests/QueryExecute',
            'YdbResponses/Success',
            'Responses/Bytes',
        }
        self.check_db_counters(sensors_to_check, 'kqp')


def get_default_feature_flag_value(feature_flag_camel_case) -> bool:
    return getattr(config_pb2.TAppConfig().FeatureFlags, feature_flag_camel_case)


@pytest.fixture(
    scope="module",
    params=[True, False],
    ids=["enable_separate_quotas", "disable_separate_quotas"],
)
def ydb_cluster_configuration(request):
    extra_feature_flags = ['enable_db_counters']
    if request.param:
        extra_feature_flags.append("enable_separate_disk_space_quotas")
    else:
        # Note: in case the assert is failing remove the parametrization by this feature flag completely.
        # Unfortunately, it is not possible to disable a feature flag using the extra_feature_flags parameter.
        # So we must make sure that the default value for the particular feature flag is false, or
        # the test would not exhibit the behavior we would like it to.
        assert not get_default_feature_flag_value("EnableSeparateDiskSpaceQuotas")

    return dict(extra_feature_flags=extra_feature_flags)


@pytest.fixture(scope="function")
def ydb_database(ydb_cluster, ydb_root, ydb_safe_test_name):
    database = os.path.join(ydb_root, ydb_safe_test_name)

    with ydb_database_ctx(ydb_cluster, database, storage_pools={"hdd": 1, "hdd1": 1}):
        yield database


def ydbcli_db_schema_exec(node, operation_proto):
    endpoint = f"{node.host}:{node.port}"
    args = [
        node.binary_path,
        f"--server=grpc://{endpoint}",
        "db",
        "schema",
        "exec",
        operation_proto,
    ]
    command = subprocess.run(args, capture_output=True)
    assert command.returncode == 0, command.stderr.decode("utf-8")


def alter_database_quotas(node, database_path, database_quotas):
    logger.debug(f"adding storage quotas to db {database_path}")
    alter_proto = """ModifyScheme {
        OperationType: ESchemeOpAlterExtSubDomain
        WorkingDir: "%s"
        SubDomain {
            Name: "%s"
            DatabaseQuotas {
                %s
            }
        }
    }""" % (
        os.path.dirname(database_path),
        os.path.basename(database_path),
        database_quotas,
    )

    ydbcli_db_schema_exec(node, alter_proto)


def create_table(session, table):
    session.execute_scheme(
        f"""
        CREATE TABLE `{table}` (
            key Int32,
            value String FAMILY custom,
            PRIMARY KEY (key),
            FAMILY default (DATA = "hdd"),
            FAMILY custom (DATA = "hdd1")
        );
        """
    )


def alter_partition_config(client, table, partition_config):
    response = client.send_and_poll_request(
        AlterTableRequest(os.path.dirname(table), os.path.basename(table))
        .with_partition_config(partition_config)
        .protobuf
    )
    assert_that(response, ProtobufWithStatusMatcher(MessageBusStatus.MSTATUS_OK))


def insert_data(session, table):
    session.transaction().execute(
        f"""
        UPSERT INTO `{table}` (
            key,
            value
        )
        VALUES
            (1, "foo"),
            (2, "bar"),
            (3, "baz");
        """,
        commit_tx=True,
    )


def drop_table(session, table):
    session.drop_table(table)


def describe(client, path):
    return client.describe(path, token="")


def check_disk_quota_exceedance(client, database, retries=10, sleep_duration=5):
    for attempt in range(retries):
        path_description = describe(client, database)
        domain_description = path_description.PathDescription.DomainDescription
        quota_exceeded = domain_description.DomainState.DiskQuotaExceeded
        logger.debug(
            f"attempt: {attempt}\n"
            f"database storage usage: {domain_description.DiskSpaceUsage}"
            f"quotas: {domain_description.DatabaseQuotas}"
            f"quota exceedance state: {quota_exceeded}"
        )
        if quota_exceeded:
            return
        time.sleep(sleep_duration)

    assert False, "database did not move into DiskQuotaExceeded state"


def wait_for_stats(client, table, retries=10, sleep_duration=5):
    for attempt in range(retries):
        usage = describe(client, table).PathDescription.TableStats.StoragePools.PoolsUsage
        if len(usage) > 0:
            return usage
        time.sleep(sleep_duration)

    assert False, "haven't received non-null table stats in the alloted time"


# Note: the default total sleep time is 300 seconds, because 200 seconds can sometimes be not enough
def check_counters(mon_port, sensors_to_check, service="ydb", retries=60, sleep_duration=5):
    sensor_count = 0
    for _, expected_value in sensors_to_check.items():
        if isinstance(expected_value, list):
            sensor_count += len(expected_value)
        else:
            sensor_count += 1

    for attempt in range(retries + 1):
        counters = get_db_counters(mon_port, service)
        correct_sensors = 0
        if counters:
            for sensor in counters["sensors"]:
                labels = sensor["labels"]
                for target_name, expected_value in sensors_to_check.items():
                    if isinstance(expected_value, list):
                        if labels.get("api_service", "") != target_name:
                            continue
                        for x in expected_value:
                            if len(x) == [labels.get(k, "") == v if k != "value" else sensor[k] == v for k, v in x.items()].count(True):
                                correct_sensors += 1
                    else:
                        if labels["name"] == target_name:
                            logger.debug(
                                f"sensor {target_name}: expected {expected_value}, "
                                f'got {sensor["value"]} in {sleep_duration * attempt} seconds'
                            )
                            if sensor["value"] == expected_value:
                                correct_sensors += 1
                    if correct_sensors == sensor_count:
                        return

        logger.debug(
            f"got {correct_sensors} out of {len(sensors_to_check)} correct sensors "
            f"in {sleep_duration * attempt} seconds"
        )
        time.sleep(sleep_duration)

    assert False, (
        f"didn't receive expected values for sensors {sensors_to_check.keys()} "
        f"in {sleep_duration * retries} seconds"
    )


class TestStorageCounters:
    def test_storage_counters(self, ydb_cluster_configuration, ydb_cluster, ydb_database, ydb_client_session):
        database_path = ydb_database
        node = ydb_cluster.nodes[1]

        alter_database_quotas(
            node,
            database_path,
            """
            storage_quotas {
                unit_kind: "hdd"
                data_size_hard_quota: 2
                data_size_soft_quota: 1
            }
            storage_quotas {
                unit_kind: "hdd1"
                data_size_hard_quota: 20
                data_size_soft_quota: 10
            }
            """,
        )

        client = ydb_cluster.client
        quotas = describe(client, database_path).PathDescription.DomainDescription.DatabaseQuotas.storage_quotas
        assert len(quotas) == 2
        assert json_format.MessageToDict(quotas[0], preserving_proto_field_name=True) == {
            "unit_kind": "hdd",
            "data_size_hard_quota": "2",
            "data_size_soft_quota": "1",
        }
        assert json_format.MessageToDict(quotas[1], preserving_proto_field_name=True) == {
            "unit_kind": "hdd1",
            "data_size_hard_quota": "20",
            "data_size_soft_quota": "10",
        }

        slot_mon_port = ydb_cluster.slots[1].mon_port
        # Note 1: limit_bytes is equal to the database's SOFT quota
        # Note 2: .hdd counter aggregates quotas across all storage pool kinds with prefix "hdd", i.e. "hdd" and "hdd1"
        check_counters(slot_mon_port, {"resources.storage.limit_bytes.hdd": 11})

        pool = ydb_client_session(database_path)
        with pool.checkout() as session:
            table = os.path.join(database_path, "table")

            create_table(session, table)

            old_partition_config = describe(client, table).PathDescription.Table.PartitionConfig
            # this forces MemTable to be written out to the storage pools sooner
            old_partition_config.CompactionPolicy.InMemForceSizeToSnapshot = 1
            alter_partition_config(client, table, old_partition_config)
            new_partition_config = describe(client, table).PathDescription.Table.PartitionConfig
            assert_that(new_partition_config.CompactionPolicy.InMemForceSizeToSnapshot, equal_to(1))

            insert_data(session, table)
            if "enable_separate_disk_space_quotas" in ydb_cluster_configuration["extra_feature_flags"]:
                check_disk_quota_exceedance(client, database_path)

            btree_index_feature_flag = get_default_feature_flag_value("EnableLocalDBBtreeIndex")
            usage = wait_for_stats(client, table)
            assert len(usage) == 2
            assert json_format.MessageToDict(usage[0], preserving_proto_field_name=True) == {
                "PoolKind": "hdd",
                "DataSize": "50",
                "IndexSize": "0" if btree_index_feature_flag else "82",
            }
            assert json_format.MessageToDict(usage[1], preserving_proto_field_name=True) == {
                "PoolKind": "hdd1",
                "DataSize": "71",
                "IndexSize": "0",
            }
            used_bytes_by_tables = 121 if btree_index_feature_flag else 203

            # Note: .hdd counter aggregates usage across all storage pool kinds with prefix "hdd", i.e. "hdd" and "hdd1"
            check_counters(
                slot_mon_port,
                {
                    "resources.storage.used_bytes": used_bytes_by_tables,
                    "resources.storage.used_bytes.ssd": 0,
                    "resources.storage.used_bytes.hdd": used_bytes_by_tables,
                    "resources.storage.table.used_bytes": used_bytes_by_tables,
                    "resources.storage.table.used_bytes.ssd": 0,
                    "resources.storage.table.used_bytes.hdd": used_bytes_by_tables,
                },
            )

            drop_table(session, table)

            check_counters(
                slot_mon_port,
                {
                    "resources.storage.used_bytes": 0,
                    "resources.storage.used_bytes.ssd": 0,
                    "resources.storage.used_bytes.hdd": 0,
                    "resources.storage.table.used_bytes": 0,
                    "resources.storage.table.used_bytes.ssd": 0,
                    "resources.storage.table.used_bytes.hdd": 0,
                },
            )


def test_serverless_counters(ydb_cluster, ydb_endpoint, ydb_root, ydb_safe_test_name):
    hostel_db = os.path.join(ydb_root, "hostel_db", ydb_safe_test_name)
    ydb_cluster.create_hostel_database(
        hostel_db,
        storage_pool_units_count={
            'hdd': 1,
        },
    )

    ydb_cluster.register_and_start_slots(hostel_db, count=1)
    ydb_cluster.wait_tenant_up(hostel_db)

    serverless_db = os.path.join(ydb_root, "serverless", ydb_safe_test_name)
    ydb_cluster.create_serverless_database(
        serverless_db,
        hostel_db=hostel_db,
        attributes={
            "cloud_id": "CLOUD_ID_VAL",
            "folder_id": "FOLDER_ID_VAL",
            "database_id": "DATABASE_ID_VAL",
        },
    )

    driver_config = ydb.DriverConfig(ydb_endpoint, serverless_db, auth_token='root@builtin')
    driver = ydb.Driver(driver_config)
    driver.wait(120)

    session = driver.table_client.session().create()

    session.create_table(
        os.path.join(serverless_db, "table") ,
        ydb.TableDescription()
        .with_column(ydb.Column("id", ydb.OptionalType(ydb.DataType.Uint64)))
        .with_primary_key("id")
    )

    try:
        session.create_table(
            os.path.join(serverless_db, "invalid_table") ,
            ydb.TableDescription()
            .with_column(ydb.Column("id", ydb.OptionalType(ydb.DataType.Float)))
            .with_primary_key("id")
        )
    except ydb.issues.SchemeError:
        pass

    slot_mon_port = ydb_cluster.slots[1].mon_port
    expected_counters = {
        "table": [
            {
                "method": "CreateSession",
                "name": "api.grpc.request.count",
                "value": 1,
            },
            {
                "method": "CreateSession",
                "name": "api.grpc.response.count",
                "status": "SUCCESS",
                "value": 1,
            },
            {
                "method": "CreateTable",
                "name": "api.grpc.request.count",
                "value": 2,
            },
            {
                "method": "CreateTable",
                "name": "api.grpc.response.count",
                "status": "SUCCESS",
                "value": 1,
            },
            {
                "method": "CreateTable",
                "name": "api.grpc.response.count",
                "status": "SCHEME_ERROR",
                "value": 1,
            },
        ],
    }
    check_counters(slot_mon_port, expected_counters, "ydb_serverless")
