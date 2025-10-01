'''
 * SPDX-FileCopyrightText: (C) 2024 Mathis Bjoerner, Leibniz Institute for Baltic Sea Research Warnemuende
 * SPDX-FileCopyrightText: (C) 2025 Mathis Mahler, Thünen Institute of Baltic Sea Fisheries
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Additional functions to get data from the InfluxDB. Most functions are designed to det data from the last day
'''

import pandas as pd
from influxdb_client import InfluxDBClient

"""
    Functions to assist in creating the .nc files on the Pi
"""


def get_local_loggerId(client, params):
    """
    :param client: InfluxDB-client
    :param range: [string] either relative as '-
    :return: dataframe of all loggers storing data in the last day
    """
    query_api = client.query_api()
    if params["_stop"] == "now()":
        stop_string = ""
    else:
        stop_string = f', stop: {params["_stop"]}'


    # the API is designed for Influx 2.0, easy querys would be by having different DBs for information on Calibration...
    bucket = 'localhyfive'                       # the database
    query = f'from(bucket: "{bucket}") \
              |> range(start: {params["_start"]}{stop_string})  \
              |> filter(fn: (r) => r["_measurement"] == "netcdf")\
              |> keyValues(keyColumns: ["logger_id"])\
              |> group()\
              |> pivot(rowKey: ["_value"], columnKey: ["_key"], valueColumn: "_value")'
    tables = query_api.query_data_frame(query)
    print(query)
    if tables.empty:
        return pd.DataFrame()
    return tables.drop(columns=['result', 'table'])


def get_local_dataframe(client, params):
    """
    :param client: InfluxDB-client
    :param params: dictionary of query parameters
    :return: data of the last day of this logger
    """
    query_api = client.query_api()

    # the API is designed for Influx 2.0, easy querys would be by having different DBs for information on Calibration...
    bucket = 'localhyfive'                       # the database
    query = f'from(bucket: "{bucket}") \
              |> range(start: {params["_start"]}, stop: {params["_stop"]})  \
              |> filter(fn: (r) => r["_measurement"] == "netcdf")\
              |> filter(fn: (r) => r["logger_id"] == "{params["_loggerID"]}")\
              |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")'
    tables = query_api.query_data_frame(query)
    print(type(tables))
    if isinstance(tables, list):
        print("List returned, not dataframe")
        return tables

    if tables.empty:
        print("Couldn't find data for given query")
        print(query)
        return pd.DataFrame()
    return tables.drop(columns=['result', 'table', '_start', '_stop', '_measurement'])


def get_local_parameters(client, params):
    """
    :param client: InfluxDB-client
    :param logger_id: specific logger
    :param deployment_id: specific deployment
    :return: list of parameters measured
    """
    
    query_api = client.query_api()

    # the API is designed for Influx 2.0, easy querys would be by having different DBs for information on Calibration...
    bucket = 'localhyfive'  # the database
    query = f'from(bucket: "{bucket}") \
                  |> range(start: {params["_start"]}, stop: {params["_stop"]})  \
                  |> filter(fn: (r) => r["_measurement"] == "attributes")\
                  |> filter(fn: (r) => r["logger_id"] == "{params["_loggerID"]}")\
                  |> filter(fn: (r) => r["deployment_id"] == "{params["_deploymentID"]}")\
                  |> filter(fn: (r) => r["_field"] == "sensor_id")\
                  |> unique()\
                  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")'
    tables = query_api.query_data_frame(query)
    if tables.empty:
        print("Couldn't find data for given query")
        print(query)
        return pd.DataFrame()
    return tables.drop(columns=['table', '_start', '_stop', '_measurement'])


def get_local_header(client, params):
    """
    :param client: InfluxDB-client
    :param device_id: specific logger
    :param deployment_id: specific deployment
    :param parameter: specific parameter
    :return: all information on this parameter
    """
    query_api = client.query_api()

    # print("Header, Logger: " + str(device_id))
    # print("Header, Parameter: " + parameter)
    # the API is designed for Influx 2.0, easy querys would be by having different DBs for information on Calibration...
    bucket = 'localhyfive'                       # the database
    query = f'from(bucket: "{bucket}") \
              |> range(start: {params["_start"]}, stop: {params["_stop"]})  \
                  |> filter(fn: (r) => r["_measurement"] == "attributes")\
                  |> filter(fn: (r) => r["logger_id"] == "{params["_loggerID"]}")\
                  |> filter(fn: (r) => r["deployment_id"] == "{params["_deploymentID"]}")\
              |> filter(fn: (r) => r["parameter"] == "{params["_parameter"]}")\
              |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")'
    tables = query_api.query_data_frame(query)
    if tables.empty:
        print("Couldn't find data for given query")
        print(query)
        return pd.DataFrame()

    return tables.drop(columns=['result', 'table', '_start', '_stop', '_measurement'])

