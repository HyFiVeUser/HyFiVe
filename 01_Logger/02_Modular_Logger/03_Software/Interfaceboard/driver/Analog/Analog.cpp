/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Driver to read from analog in. linear calibration function, see getCalculatedValue()
 */

#include <driver/Analog/Analog.h>
#include "sensor_config.h"

//Analog::Analog()
//{
//    // TODO Auto-generated constructor stub
//
//}
volatile uint16_t ADC_Value_adc;

Analog::~Analog()
{
    // TODO Auto-generated destructor stub
}

bool Analog::setCalib(float cal, uint8_t coeffToSet)
{
    //no calibration possible until now
    switch (coeffToSet){
    case 1:
        Co1 = cal;
        break;
    case 2:
        Co2 = cal;
        break;
    case 3:
        Co3 = cal;
        break;
    case 4:
        Co4 = cal;
        break;
    }
    calibrated = true;
    return true;
}

bool Analog::getCalibrated()
{
    return calibrated;
}

uint8_t Analog::getParameter(){
        return 0x05;
}

uint8_t Analog::getExternParameter(){
        return 0xFF;
}

uint32_t Analog::getVersion(){
    return (turner_turbidity_CFlour_TRB ) | (turner_phycoerythrin_CFlour_PE << 8);
}

bool Analog::init()
{
    P1DIR &= ~BIT7; // Interface GPIO 1 - P1.7
    P1DIR &= ~BIT6; // Interface GPIO 2 - P1.6
    P1DIR &= ~BIT5; // Interface GPIO 3 - P1.5
    P1DIR &= ~BIT4; // Interface GPIO 4 - P1.4
    P1REN |= BIT7 | BIT6 | BIT5 | BIT4;
    P2OUT |= BIT7 | BIT6 | BIT5 | BIT4;


    // Analog Sensor Enable Output
    P1DIR |= BIT0; // Output 1.0 als Output setzen
    P1OUT &= ~BIT0; // Output 1.0 als Low setzen
    // Configure ADC A1 pin
        SYSCFG2 |= 1; // ge�ndert von |= auf = 1

     // Check PxSEL Register for Shut Down GPIO

    // Disable the GPIO power-on default high-impedance mode to activate
       // previously configured port settings
    //   PM5CTL0 &= ~LOCKLPM5;

    P1SEL0 |= 1;
    P1SEL1 |= 1;
    P1DIR &= ~BIT0;

   // Configure ADC0
    ADCCTL0 &= ~ADCENC;             // Disable ADC
    ADCCTL0  |= ADCSHT_2;           // ADCON, S&H=16 ADC clks
    ADCCTL1  |= ADCSHP;             // ADCCLK = MODOSC; sampling timer

    ADCCTL2 &= ~ADCRES;
    ADCCTL2 |= ADCRES_2;            // 12-bit conversion results

    ADCMCTL0 |= ADCINCH_0;          // A0 ADC input select; Vref = AVCC
    ADCIE    |= ADCIE0;             // Enable ADC conv complete interrupt
    ADCCTL0  |= ADCON;              // Turn ADC On

    return true;
}

bool Analog::hibernate(){
    // Die Masse des Sensors kann �ber einen Mosfet abgeschaltet werden
    P1OUT &= ~BIT0; // Output 1.0 als Low setzen
    return true;
}

bool Analog::wakeup(){
    // Die Masse des Sensors kann �ber einen Mosfet angeschaltet werden werden
    P1OUT |= BIT0;
    return true;
}

bool Analog::startConversion(){

    ADCCTL0 |= ADCENC | ADCSC;          // Sampling and conversion start
   __bis_SR_register(LPM0_bits | GIE);  // LPM0, ADC_ISR will force exit
   ADC_Value = ADC_Value_adc;
    return true;
}

bool Analog::getRAWValue(int64_t *aval){
    aval[0] = (ADC_Value);
    aval[1] = -1;
    return true;
}

bool Analog::getCalculatedValue(int64_t *aval){

    float val;
    //val = ((ADC_Value/1024.0)*5 - Co1)*Co2; // 10-biz
    val = ((ADC_Value/4096.0)*5 - Co1)*Co2; //12-bit

    union {
        float float_variable;
        unsigned char temp_array[4];
    } u;
    u.float_variable = val;
    memcpy(&aval[0], u.temp_array, 4);

    return true;
}

uint8_t Analog::crc4(uint16_t n_prom[]) {
    uint16_t n_rem = 0;

    n_prom[0] = ((n_prom[0]) & 0x0FFF);
    n_prom[7] = 0;

    for ( uint8_t i = 0 ; i < 16; i++ ) {
        if ( i%2 == 1 ) {
            n_rem ^= (uint16_t)((n_prom[i>>1]) & 0x00FF);
        } else {
            n_rem ^= (uint16_t)(n_prom[i>>1] >> 8);
        }
        for ( uint8_t n_bit = 8 ; n_bit > 0 ; n_bit-- ) {
            if ( n_rem & 0x8000 ) {
                n_rem = (n_rem << 1) ^ 0x3000;
            } else {
                n_rem = (n_rem << 1);
            }
        }
    }

    n_rem = ((n_rem >> 12) & 0x000F);

    return n_rem ^ 0x00;
}

// ADC interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC_VECTOR))) ADC_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
    {
        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:
            break;
        case ADCIV_ADCTOVIFG:
            break;
        case ADCIV_ADCHIIFG:
            break;
        case ADCIV_ADCLOIFG:
            break;
        case ADCIV_ADCINIFG:
            break;
        case ADCIV_ADCIFG:
            ADC_Value_adc = ADCMEM0;
            ADCIFG = 0;
            break;          // Clear CPUOFF bit from 0(SR)
        default:
            break;
    }
}
