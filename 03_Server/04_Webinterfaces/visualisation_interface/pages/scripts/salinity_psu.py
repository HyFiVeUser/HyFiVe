import gsw as gsw
import json
import sys
import math


def calculate_salinity(measurements):
    salinity_psu = []
    
    for index, measurement in enumerate(measurements):
        try:
            conductivity, temperature, pressure = measurement
            
            # check for conductivity <= 0
            if float(conductivity) <= 0:
                print(
                    f"INVALID CONDUCTIVITY AT INDEX {index}: "
                    f"conductivity={conductivity}, "
                    f"temperature={temperature}, "
                    f"pressure={pressure}",
                    file=sys.stderr
                )
                
                # Return null in JSON instead of NaN
                salinity_psu.append(None)
                continue
                
            # calculate salinity
            pressure_gsw = (float(pressure) - 1013) / 100
            sp = gsw.conversions.SP_from_C(
                float(conductivity),
                float(temperature),
                pressure_gsw
            )
            
            # check result for NaN
            if math.isnan(sp):
                print(
                    f"NaN FOUND AT INDEX {index}: "
                    f"conductivity={conductivity}, "
                    f"temperature={temperature}, "
                    f"pressure={pressure}",
                    file=sys.stderr
                )
                
                # Return null in JSON instead of NaN
                salinity_psu.append(None)
                continue
                
            # check result for infinity
            if math.isinf(sp):
                print(
                    f"INF FOUND AT INDEX {index}: "
                    f"conductivity={conductivity}, "
                    f"temperature={temperature}, "
                    f"pressure={pressure}",
                    file=sys.stderr
                )
                
                # Return null in JSON instead of inf
                salinity_psu.append(None)
                continue
                
            # return result to stdout
            salinity_psu.append(float(sp))
            
            
        except Exception as e:
            print(
                f"ERROR AT INDEX {index}: {measurement}",
                file=sys.stderr
            )
            print(str(e), file=sys.stderr)
            
            # return null for valid JSON
            salinity_psu.append(None)
            
            
    return salinity_psu


if __name__ == "__main__":
    # read data from stdin
    input_data = json.load(sys.stdin)
    
    measurements = input_data["measurements"]
    salinity_results = calculate_salinity(measurements)
    
    # Valid JSON output
    print(json.dumps(salinity_results))