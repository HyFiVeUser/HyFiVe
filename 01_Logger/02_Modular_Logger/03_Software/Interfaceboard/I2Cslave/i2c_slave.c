/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: I2C Slave Interface
 */

#include <I2Cslave/i2c_slave.h>
#include <msp430.h>

void (*TI_receive_callback)(unsigned char receive);
void (*TI_transmit_callback)(unsigned char volatile *send_next);
void (*TI_start_callback)(void);

void I2C_slaveInit(void (*SCallback)(),
                   void (*TCallback)(unsigned char volatile *value),
                   void (*RCallback)(unsigned char value),
                   unsigned char slave_address)
{
    UCB0CTLW0 = UCSWRST; // put eUSCI_B in reset state
    UCB0CTLW0 |= UCMODE_3 | UCSYNC;
    ;                                    // I2C Slave, synchronous mode
    UCB0I2COA0 = slave_address | UCOAEN; // Own Address is $address

    UCB0CTLW0 &= ~UCSWRST;                          // eUSCI_B in operational state
    UCB0IE |= UCSTTIE | UCTXIE0 | UCSTPIE | UCRXIE; // UCSTTIE + UCTXIE + UCRXIE;       // enable TX&RX-interrupt

    TI_start_callback = SCallback;
    TI_receive_callback = RCallback;
    TI_transmit_callback = TCallback;
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B0_VECTOR
__interrupt void USCIB0_ISR(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(USCI_B0_VECTOR))) USCIB0_ISR(void)
#else
#error Compiler not supported!
#endif
{

    unsigned int a = __even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG);
    switch (a)
    {
    case USCI_NONE: // Vector 0: No interrupts
        break;
    case USCI_I2C_UCALIFG: // Vector 2: ALIFG
        break;
    case USCI_I2C_UCNACKIFG: // Vector 4: NACKIFG
        break;
    case USCI_I2C_UCSTTIFG: // Vector 6: STTIFG
        TI_start_callback();
        break;
    case USCI_I2C_UCSTPIFG:   // Vector 8: STPIFG
        UCB0IFG &= ~UCSTPIFG; // clear stop bit flag
        LPM0_EXIT;            // Exit active CPU
        break;
    case USCI_I2C_UCRXIFG3: // Vector 10: RXIFG3
        break;
    case USCI_I2C_UCTXIFG3: // Vector 12: TXIFG3
        break;
    case USCI_I2C_UCRXIFG2: // Vector 14: RXIFG2
        break;
    case USCI_I2C_UCTXIFG2: // Vector 16: TXIFG2
        break;
    case USCI_I2C_UCRXIFG1: // Vector 18: RXIFG1
        break;
    case USCI_I2C_UCTXIFG1: // Vector 20: TXIFG1
        break;
    case USCI_I2C_UCRXIFG0: // Vector 22: RXIFG0 Data received
        TI_receive_callback(UCB0RXBUF);
        break;
    case USCI_I2C_UCTXIFG0: // Vector 24: TXIFG0 transmit buffer empty
        TI_transmit_callback((volatile unsigned char *)&UCB0TXBUF);
        break;
    case USCI_I2C_UCBCNTIFG: // Vector 26: BCNTIFG
        break;
    case USCI_I2C_UCCLTOIFG: // Vector 28: clock low time-out
        break;
    case USCI_I2C_UCBIT9IFG: // Vector 30: 9th bit
        break;
    default:
        break;
    }
}
