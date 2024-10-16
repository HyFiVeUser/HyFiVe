import numpy as np

def O2ptoO2c(pO2,T,S,P=0):
    #function O2conc=O2ptoO2c(pO2,T,S,P)
    #
    # convert oxygen partial pressure to molar oxygen concentration
    #
    # inputs:
    #   pO2    - oxygen partial pressure in mbar
    #   T      - temperature in °C
    #   S      - salinity (PSS-78)
    #   P      - hydrostatic pressure in dbar (default: 0 dbar)
    #
    # output:
    #   O2conc - oxygen concentration in ml L-1 (can easily be changed to umol L-1)
    #
    # according to recommendations by SCOR WG 142 "Quality Control Procedures
    # for Oxygen and Other Biogeochemical Sensors on Floats and Gliders"
    #
    # Henry Bittig
    # Laboratoire d'Océanographie de Villefranche-sur-Mer, France
    # bittig@obs-vlfr.fr
    # 28.10.2015
    # 19.04.2018, v1.1, fixed typo in B2 exponent

    xO2     = 0.20946; # mole fraction of O2 in dry air (Glueckauf 1951)
    pH2Osat = 1013.25*(np.exp(24.4543-(67.4509*(100./(T+273.15)))-(4.8489*np.log(((273.15+T)/100)))-0.000544*S)); # saturated water vapor in mbar
    sca_T   = np.log((298.15-T)/(273.15+T)); # scaled temperature for use in TCorr and SCorr
    TCorr   = 44.6596*np.exp(2.00907+3.22014*sca_T+4.05010*sca_T**2+4.94457*sca_T**3-2.56847e-1*sca_T**4+3.88767*sca_T**5); # temperature correction part from Garcia and Gordon (1992), Benson and Krause (1984) refit mL(STP) L-1; and conversion from mL(STP) L-1 to umol L-1

    Scorr   = np.exp(S*(-6.24523e-3-7.37614e-3*sca_T-1.03410e-2*sca_T**2-8.17083e-3*sca_T**3)-4.88682e-7*S**2); # salinity correction part from Garcia and Gordon (1992), Benson and Krause (1984) refit ml(STP) L-1
    Vm      = 0.317; # molar volume of O2 in m3 mol-1 Pa dbar-1 (Enns et al. 1965)
    R       = 8.314; # universal gas constant in J mol-1 K-1

    O2conc_umolL=pO2/(xO2*(1013.25-pH2Osat))*(TCorr*Scorr)/np.exp(Vm*P/(R*(T+273.15)));

    O2conc_mlL=O2conc_umolL/44.6596

    return O2conc_mlL
