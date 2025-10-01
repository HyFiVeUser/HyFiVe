'''
 * SPDX-FileCopyrightText: (C) 2024 Mathis Bjoerner, Leibniz Institute for Baltic Sea Research Warnemuende
 * SPDX-FileCopyrightText: (C) 2025 Mathis Mahler, Thünen Institute of Baltic Sea Fisheries
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Routine to create NetCDF files from data in the InfluxDB
'''

import xarray as xr
import pandas as pd
from datetime import datetime as dt
from loadLocalData import *
import json
from influxdb_client import InfluxDBClient
import argparse
import datetime
import os
import sys


'''
    This script runs on the raspberry Pi to create .nc files of recent deployments

    This is an updated version of create_netcdf.py, where the code was split into different functions
    to make it more readable and maintainable. Also some variables can be set via command line arguments, which should work also in NodeRed.

    --- ATTENTION --- NodeRed needs to be adapted and checked before running this script there!

    Helpful for understanding and creating
    https://docs.xarray.dev/en/stable/generated/xarray.Dataset.html         dataset
    https://wiki.esipfed.org/Attribute_Convention_for_Data_Discovery_1-3    recommended attributes
    https://cfconventions.org/Data/cf-standard-names/current/build/cf-standard-name-table.html  standard names
'''

def handle_time(time_string, checks = ("d", "h", "m", "z", "Z"), ):
    '''
    give the correct time format for influx.
    e.g.: 2021-05-22T23:30:00Z
    checks: looking for given Strings at end of time_string to make sure conversion works

    '''
    if time_string is None:
        return "now()" 
    format = "%Y-%m-%dT%H:%M:%SZ"
    assert isinstance(time_string, str)
    assert time_string.endswith(checks)
    
    if time_string.endswith(("Z", "z")):
        print(datetime.strptime(time_string.upper(), format))
    
    for c in checks:
        if time_string.endswith(c):
            if time_string.startswith("-"):
                return time_string
            else:
                return "-" + time_string
    print("found no matching time format")
    raise ValueError(f"Couldn't find a matching time format! Use {checks} or define a time string according to this format {format}")

def assign_pressure_att(xr_data, h_pressure):
    calibration_pressure = ''
    for cal_coeff in ['k0', 'k1', 'k2', 'k3', 'k4', 'k5', 'k6', 'k7', 'k8', 'k9']:
        if cal_coeff in h_pressure:
            calibration_pressure += '"' + cal_coeff + '":"' + str(h_pressure[cal_coeff]) + '",'
    calibration_pressure = calibration_pressure[:-1]
    xr_data['pressure'].attrs = {
        'long_name': h_pressure['long_name'],
        'units': h_pressure['unit'],
        'sensor_id': h_pressure['sensor_id'],
        'SerialNumber': h_pressure['serial_number'],
        'sensor_type': '{"sensor_type_id": "' + str(h_pressure['sensor_type_id']) + '", "manufacturer": "' +
                    h_pressure['manufacturer'] + '", "model_name": "' + h_pressure['model_name'] + '"}',
        # Lets use manufactures names defined by http://vocab.nerc.ac.uk/collection/L35/current/
        'accuracy': h_pressure['accuracy'],
        'resolution': h_pressure['resolution'],
        'coverage_content_type': 'coordinate',  # What is that?
        'positive': 'down',  # necessary?         # increase with depth
        'calibration_coefficients': '{' + calibration_pressure + '}'
    }
    return xr_data

def get_xr_data(deployment, deployment_id, header, data_vars):
    xr_data = xr.Dataset(
        # coordinates as a dictionary, time is dimension
        coords = {'time': deployment['_time'],
                'latitude': (['time'], list(deployment['latitude'])),
                'longitude': (['time'], list(deployment['longitude'])),
                'pressure': (['time'], list(deployment['pressure']))},
        data_vars = data_vars
    )

    # Assign global attributes
    dtnow = dt.now().strftime("%Y-%m-%dT%H:%M:%SZ")
    xr_data.attrs['date_created'] = dtnow
    xr_data.attrs['history'] = f'File created at {dtnow} using xarray in Python'
    xr_data.attrs['software_version'] = '{"logger": "0.1", "deckunit": "0.1"}'

    # Convention: If multiple attributes of one entity shall be named, use a dictionary
    xr_data.attrs['deployment'] = '{"deployment_id": "' + str(deployment_id) + '", "time_start": "' + str(
        deployment['_time'].iloc[0]) + '", "time_end": "' + str(
        deployment['_time'].iloc[-1]) + '", "position_start": "' + str(
        deployment['latitude'].iloc[0]) + ',' + str(
        deployment['longitude'].iloc[0]) + '", "position_end":"' + str(
        deployment['latitude'].iloc[-1]) + ',' + str(deployment['longitude'].iloc[-1]) + '"}'
    
    xr_data.attrs['logger_id'] = deployment['logger_id'].iloc[0]
    xr_data.attrs['deckunit_id'] = header.deckunit_id
    xr_data.attrs['platform_id'] = header.platform_id
    xr_data.attrs['vessel'] = '{"id": "' + str(header.vessel_id) + '", "name": "' + header.vessel_name + '"}'
    xr_data.attrs['contact'] = '{"id": "' + str(header.contact_id) + '",' \
                                '"first_name":"' + header.contact_f_name + '",' \
                                '"last_name": "' + header.contact_l_name + '"}'
    # Assign coordinates attributes
    xr_data['time'].attrs = {
        'standard_name': 'time',
        'long_name': 'Time UTC',
        'coverage_content_type': 'coordinate',
    }

    xr_data['latitude'].attrs = {
        'standard_name': 'latitude',
        'long_name': 'latitude',
        'units': 'degrees_north',
        'coverage_content_type': 'coordinate'
    }

    xr_data['longitude'].attrs = {
        'standard_name': 'longitude',
        'long_name': 'longitude',
        'units': 'degrees_east',
        'coverage_content_type': 'coordinate'
    }
    return xr_data
    
def handle_parameters(xr_data, parameter, influx_client, query_params):
    if parameter == 'pressure':
        return xr_data
    query_params["_parameter"] = str(parameter)

    header_all = get_local_header(influx_client, query_params)
    if header_all.empty:
        print('empty header')
        return xr_data
    else:
        header = header_all.iloc[0]
    calibration = ''
    for cal_coeff in ['k0', 'k1', 'k2', 'k3', 'k4', 'k5', 'k6', 'k7', 'k8', 'k9']:
        if cal_coeff in header:
            calibration += '"' + cal_coeff + '":"' + str(header[cal_coeff]) + '",'
    calibration = calibration[:-1]

    xr_data[parameter].attrs = {
        'long_name': header['long_name'],
        'units': header['unit'],
        'sensor_id': header['sensor_id'],
        'SerialNumber': header['serial_number'],
        'sensor_type': '{"sensor_type_id": "' + str(header['sensor_type_id']) + '", "manufacturer": "' +
                        header['manufacturer'] + '", "model_name": "' + header['model_name'] + '"}',
        # Lets use manufactures names defined by http://vocab.nerc.ac.uk/collection/L35/current/
        #'accuracy': header['accuracy'],
        # 'resolution': header['resolution'],
        'coverage_content_type': 'physicalMeasurement',
        'calibration_coefficients': '{' + calibration + '}'
    }
    return xr_data
    
def handle_deployments(deployment, deployment_id, influx_client, query_params):
    query_params["_deploymentID"] = str(deployment_id)

    # deployment_to_netcdf = deployment_to_netcdf.drop(deployment_to_netcdf[deployment_to_netcdf.latitude == -999].
    #                                                 index)     # this is a dummy for no GPS
    if len(deployment) < 2 or deployment.pressure.iloc[-1] > 1250\
            or int(deployment_id) in stored_deployments[str(logger_id)]:    # reasons to abort creation
        print('Abort creating netcdf file')
        return

    df_parameters = get_local_parameters(influx_client, query_params) # get list of parameters
    data_vars = {}
    # TODO Handling of deployments that have varying parameters, so don't adhere to the same list of parameters.
    for parameter in df_parameters.parameter:
        if parameter == 'pressure':
            data_vars['depth'] = (['time'], (deployment[parameter] - 1000) / 100)
        elif parameter not in deployment.columns:
            # this is a quick fix for varying parameters in deployments, but not a good solution
            # There should be a warning or something if a parameter is missing!
            print(f'Parameter {parameter} not in data columns')
            continue
        else:
            data_vars[parameter] = (['time'], deployment[parameter])
            if parameter + '_raw' in deployment:
                data_vars[parameter + '_raw'] = (['time'], deployment[parameter + '_raw'])
    query_params["_parameter"] = "logger"
    header_logger = get_local_header(influx_client, query_params).iloc[0]
    xr_data = get_xr_data(deployment, deployment_id, header_logger, data_vars)

    # Assign pressure attributes
    query_params["_parameter"] = "pressure"

    h_press = get_local_header(influx_client, query_params).iloc[-1]
    # print(h_press)
    calibration_pressure = ''
    for cal_coeff in ['k0', 'k1', 'k2', 'k3', 'k4', 'k5', 'k6', 'k7', 'k8', 'k9']:
        if cal_coeff in h_press:
            calibration_pressure += '"' + cal_coeff + '":"' + str(h_press[cal_coeff]) + '",'
    calibration_pressure = calibration_pressure[:-1]

    xr_data['pressure'].attrs = {
        'long_name': h_press['long_name'],
        'units': h_press['unit'],
        'sensor_id': h_press['sensor_id'],
        'SerialNumber': h_press['serial_number'],
        'sensor_type': '{"sensor_type_id": "' + str(h_press['sensor_type_id']) + '", "manufacturer": "' +
                        h_press['manufacturer'] + '", "model_name": "' + h_press['model_name'] + '"}',
        # Lets use manufactures names defined by http://vocab.nerc.ac.uk/collection/L35/current/
        'accuracy': h_press['accuracy'],
        'resolution': h_press['resolution'],
        'coverage_content_type': 'coordinate',  # What is that?
        'positive': 'down',  # necessary?         # increase with depth
        'calibration_coefficients': '{' + calibration_pressure + '}'
    }

    # Assign variable attributes
    for parameter in df_parameters.parameter:
        if parameter not in deployment.columns:
            continue
        xr_data = handle_parameters(xr_data, parameter, influx_client, query_params)
        

    xr_data['depth'].attrs = {
        'long_name': 'depth',
        'units': 'm',
        'accuracy': '',
        'resolution': '',
        'coverage_content_type': 'modelResult',
    }
    return xr_data

def store_netcdf(xr_data, logger_id, deployment_id):
    # sepcify encoding
    myencoding = {
        'time': {
            # 'dtype': 'datetime64[ns]',
            '_FillValue': None
        },
        'longitude': {
            'dtype': 'float64',
            '_FillValue': None
        },
        'latitude': {
            'dtype': 'float64',
            '_FillValue': None
        },
        'pressure': {
            'dtype': 'float64',
            '_FillValue': None
        }
    }

    #xr_data.to_netcdf(f'/usr/src/node-red/netcdf/logger_{logger_id}_deployment_{deployment_id}.nc',
    #                    encoding=myencoding)
    xr_data.to_netcdf(f'files/logger_{logger_id}_deployment_{deployment_id}.nc',
                      encoding=myencoding)



influx_url = 'http://10.8.0.22:8086' # for debugging on local machine
# old_influx_token = 'Iv6qi9VOSwV4iiZ5SF7VjTXGZh_3nzi9d8foXtTX4J4AgDwIzyWsuAf-nGVE2xriLxY48_2lVBPe2Z5RvqzeIg=='
# influx_token = "xWVCtY6PdJO7eKSM4LI2Ttr9MDJ1llhfVDIvYVsPyOrDilPf-AObypi_szSatn3CEtS11NVd7ogZb4YHHoKsiA==" # should be a valid token, but for which box?
influx_token = "YX62f8Vddj6akXlQxrF_5ef7aIPv_Z8lImt1npfPjKb16qKLJszvuTYTs6tAe7Pnvw_PnjNrPHB9V7csrDkJhw==" # debug token for DB05

parser = argparse.ArgumentParser()
parser.add_argument("--debug", help="additional information for debugging", action="store_true")
parser.add_argument("--influx_url", help="url to the influx database", default='http://192.168.1.123:8086')
parser.add_argument("--influx_token", help="token to the influx database", default="")
parser.add_argument("--json_path", help="path to recent_deployments.json file", default='python_scripts/recent_deployments.json')
parser.add_argument("--time_start", help="set the start time for the data. Either in Days (e.g. 14d) or minutes (e.g. 15m), or set a specific time in format yyyy-MM-ddTHH:mm:ssZ (the 'T' and 'Z' are important!). Defaults to the last 2 weeks.", default='-4d')
parser.add_argument("--time_stop", help="set the stop time for the data. Either in Days (e.g. 14d) or minutes (e.g. 15m), or set a specific time in format yyyy-MM-ddTHH:mm:ssZ (the 'T' and 'Z' are important!). Default assumes time range ends now.", default=None)
parser.add_argument("--local", help="don't send any data to server and don't update recent_deployments. For Debugging purposes!", action="store_true")

opt = parser.parse_args()
parser.print_help()
if opt.debug:
    print(opt)


json_path = opt.json_path
if opt.influx_token == "":
    influx_token = 'Fi_QAY-Drn14r-riSN0HZbx2qjEdfBPZPh6Du1R8R0NZ2VW9fGcSQPHzlAqWiFMnW0qXnkVpaRlpQ4Uu-02emA=='
else:
    influx_token = opt.influx_token

time_start = handle_time(opt.time_start)
time_stop = handle_time(opt.time_stop)

query_params = {
        "_start": time_start,
        "_stop": time_stop
    }

# influx_client: The idea here is to have one client in the main function, and not several in loadLocalData.py, as was the case before.
#
# influx_client =InfluxDBClient(url = opt.influx_url, token = influx_token, org='hyfive')
influx_client =InfluxDBClient(url = "http://10.8.0.18:8086", token = influx_token, org='hyfive') # for debugging on local machine

loggerId = get_local_loggerId(influx_client, query_params)  # data frame with current ids, back to -1d
try:
    loggerNumber = loggerId.logger_id.shape[0]  # amount of logger (size of column)
except AttributeError:
    print("There are no Loggers available for given time range")
    sys.exit()
print("Amount of devices: " + str(loggerNumber))

if os.getcwd().endswith("python_scripts"):
    json_path = os.path.join(os.getcwd(), "recent_deployments.json")


for j in range(0, loggerNumber):  # for each logger
    logger_id = loggerId.logger_id[j]
    print("Run Logger: " + str(logger_id))
    json_file = open(json_path, 'r')
    stored_deployments = json.load(json_file)
    json_file.close()
    if str(logger_id) not in stored_deployments.keys():
        # print('I have to add the Logger to the dict')
        stored_deployments[str(logger_id)] = []

    
    query_params["_loggerID"] = str(logger_id)
    dataByLogger = get_local_dataframe(influx_client, query_params)
    # TODO proper Handling of list-objects
    try:
        if dataByLogger.empty:
            continue
        deployment_ids = dataByLogger.deployment_id.unique()
        for deployment_id in deployment_ids:  # for each deployment
            print('Deployment: ' + deployment_id)
            deployment_to_netcdf = dataByLogger[dataByLogger.deployment_id == deployment_id].reset_index()

            xr_data = handle_deployments(deployment_to_netcdf, deployment_id, influx_client, query_params)
            if xr_data is not None:
                stored_deployments[str(logger_id)].append(int(deployment_id))
                print(stored_deployments)

                # store_netcdf(xr_data, logger_id, deployment_id)
                # json_file = open('python_scripts/recent_deployments.json', 'w')
                # json.dump(stored_deployments, json_file)
                # json_file.close()
    except AttributeError:
        if isinstance(dataByLogger, list):
            print("influx returns a list. Reason might be different constellations of data. Handle each separately")
            for list_df in dataByLogger:
                deployment_ids = list_df.deployment_id.unique()
                for deployment_id in deployment_ids:
                    print('Deployment: ' + deployment_id)
                    deployment_to_netcdf = list_df[list_df.deployment_id == deployment_id].reset_index()

                    xr_data = handle_deployments(deployment_to_netcdf, deployment_id, influx_client, query_params)
                    if xr_data is not None:
                        stored_deployments[str(logger_id)].append(int(deployment_id))
                        print(stored_deployments)
                        store_netcdf(xr_data, logger_id, deployment_id)
                        # json_file = open('python_scripts/recent_deployments.json', 'w')
                        # json.dump(stored_deployments, json_file)
                        # json_file.close()
        else:
            dataByLogger.empty
            continue

    

        
