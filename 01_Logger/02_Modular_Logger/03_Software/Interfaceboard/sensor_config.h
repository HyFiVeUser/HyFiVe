/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: sensor IDs
 */

#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

// SELECTED_SENSOR | Manufacturer        | Parameter                     | Model                          | sensor_type_ID  | Voltage  | FW
//-----------------:---------------------:-------------------------------:--------------------------------:-----------------:----------:-----
// 1               : blue_robotics       : pressure                      : bar30                          : 1               : +3.3V    : 1
// 2               : blue_robotics       : temperature                   : celsius_fast_response          : 2               : +3.3V    : 1
// 3               : keller              : pressure                      : series_20                      : 6               : +3.3V    : 2
// 4               : atlas_scientific    : conductivity                  : k0.1 | k1.0                    : 3 | 10          : +3.3V    : 1
// 5               : pyroscience         : oxygen                        : oxycap_sub | oxycap_hs_sub     : 9 | 11          : +3.3V    : 1
// 6               : Turner              : turbidity | phycoerythrin     : C-Flour_TRB | C-Flour_PE       : 12 | 13         : +5.0V    : 3

// SELECTED_SENSOR
#define SELECTED_SENSOR 6

//in code:
enum SENSOR_LIST{
  blue_robotics_pressure_bar30                      = 1,
  blue_robotics_temperature_celsius_fast_response   = 2,
  atlas_scientific_conductivity_k01                 = 3,
  presens_oxygen_NAUOIW                             = 4,
  presens_oxygen_NAUYOP                             = 5,
  keller_pressure_series_20                         = 6,
  aml_temperature_ct_xchange                        = 7,
  aml_conductivity_ct_xchange                       = 8,
  pyroscience_oxygen_oxycap_sub                     = 9,
  atlas_scientific_conductivity_k10                 = 10,
  pyroscience_oxygen_oxycap_hs_sub                  = 11,
  turner_turbidity_CFlour_TRB                       = 12,
  turner_phycoerythrin_CFlour_PE                    = 13,
};

#endif
