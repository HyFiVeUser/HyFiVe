/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Smart Battery System (SBS)
 */

typedef volatile struct b_BatteryStatus
{
  uint16_t OCA : 1;
  uint16_t TCA : 1;
  uint16_t RSVD_1 : 1;
  uint16_t OTA : 1;
  uint16_t TDA : 1;
  uint16_t RSVD_2 : 1;
  uint16_t RCA : 1;
  uint16_t RTA : 1;
  uint16_t INIT : 1;
  uint16_t DSG : 1;
  uint16_t FC : 1;
  uint16_t FD : 1;
  uint16_t EC3 : 1;
  uint16_t EC2 : 1;
  uint16_t EC1 : 1;
  uint16_t EC0 : 1;
} _b_BatteryStatus;

/*typedef volatile union BatteryStatus{
    _b_BatteryStatus    Fields;
    uint16_t            Value;
}_BatteryStatus;*/

typedef volatile struct b_OperationStatus
{
  uint16_t IATA_CTERM : 1;
  uint16_t RSVD_1 : 1;
  uint16_t EMSHUT : 1;
  uint16_t SLPCC : 1;
  uint16_t SLPAD : 1;
  uint16_t SMBLCAL : 1;
  uint16_t INIT : 1;
  uint16_t SLEEPM : 1;
  uint16_t XL : 1;
  uint16_t CAL_OFFSET : 1;
  uint16_t CAL : 1;
  uint16_t AUTOCALM : 1;
  uint16_t AUTH : 1;
  uint16_t LED : 1;
  uint16_t SDM : 1;
  uint16_t SLEEP : 1;
  uint16_t XCHG : 1;
  uint16_t XDSG : 1;
  uint16_t PF : 1;
  uint16_t SS : 1;
  uint16_t SDV : 1;
  uint16_t SEC : 2;
  uint16_t BTP_INT : 1;
  uint16_t RSVD_2 : 1;
  uint16_t FUSE : 1;
  uint16_t PDSG : 1;
  uint16_t PCHG : 1;
  uint16_t CHG : 1;
  uint16_t DSG : 1;
  uint16_t PRES : 1;
} _b_OperationStatus;

/*typedef volatile union OperationStatus{
    _b_OperationStatus  Fields;
    uint16_t            Value;
}_OperationStatus;*/

typedef volatile struct b_ManufacturingStatus
{
  uint16_t CAL_EN : 1;
  uint16_t LT_TEST : 1;
  uint16_t PDSG_EN : 1;
  uint16_t RSVD : 3;
  uint16_t LED_EN : 1;
  uint16_t FUSE_EN : 1;
  uint16_t BBR_EN : 1;
  uint16_t PF_EN : 1;
  uint16_t LF_EN : 1;
  uint16_t FET_EN : 1;
  uint16_t GAUGE_EN : 1;
  uint16_t DSG_EN : 1;
  uint16_t CHG_EN : 1;
  uint16_t PCHG_EN : 1;
} _b_ManufacturingStatus;

/*typedef volatile union ManufacturingStatus{
    _b_OperationStatus  Fields;
    uint16_t            Value;
}_ManufacturingStatus;*/