/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Implementation of battery charger control functions
 */

#include "Arduino.h"
#include "Charger.h"

#define On 1
#define Off 0

Charger_IC::Charger_IC()
{
}

void Charger_IC::Init()
{
	// Define Outputs
	pinMode(CE_Pin, OUTPUT);

	// Define Inputs
	pinMode(this->STAT_1, INPUT); // No Pullup
	pinMode(this->STAT_2, INPUT); // No Pullup
	pinMode(this->PG, INPUT);	  // No Pullup

	// Set Initial Status
	// digitalWrite(this->CE_Pin, LOW);

	this->CHG_Enable = 0;
	this->state = Charger_CHG_Suspended;
}

uint8_t Charger_IC::getState()
{
	// Check Polarity (!STATE_1)
	this->STAT_1 = digitalRead(this->STAT_1_PIN);
	this->STAT_2 = digitalRead(this->STAT_2_PIN);
	// Serial.print("Charge Enable = ");
	// Serial.print(digitalRead(this->CE_Pin));
	Serial.print(" | STAT_1 = ");
	Serial.print(this->STAT_1);
	Serial.print(" | STAT_2 = ");
	Serial.print(this->STAT_2);
	Serial.print(" |!PG = ");
	Serial.print(this->getPG());
	Serial.print(" | State = ");

	// Status Pins, siehe BQ24618 Datasheet S.23
	if ((STAT_1 == On) && (STAT_2 == Off))
	{
		this->state = Charger_CHG;
		Serial.println(" Charger Charging ");
	}
	else if ((STAT_1 == Off) && (STAT_2 == On))
	{
		this->state = Charger_CHG_Completed;
		Serial.println(" Charger Charging Completed");
	}
	else if ((STAT_1 == Off) && (STAT_2 == Off))
	{
		this->state = Charger_CHG_Suspended;
		Serial.println(" Charger Charging Suspended");
	}
	else
	{
		this->state = Charger_Undefined_State;
		Serial.println(" Charger_Undefined_State");
	}
	return this->state;
}

void Charger_IC::setState(uint8_t Charge_State)
{
}

void Charger_IC::setCHG_Enable(uint8_t CE_State)
{
	if (CE_State == 1)
	{
		Serial.println("Charge Enabled!");
		this->CHG_Enable = 1;
		digitalWrite(this->CE_Pin, HIGH);
	}
	else
	{
		Serial.println("Charge Disabled!");
		this->CHG_Enable = 0;
		digitalWrite(this->CE_Pin, LOW);
	}
}

uint8_t Charger_IC::getCHG_Enable()
{
	return this->CHG_Enable;
}

uint8_t Charger_IC::getPG()
{
	// PowerGood Pin (PG), siehe BQ24618 Datasheet S.23
	this->PG = digitalRead(this->PG_PIN); // check if true, !PG
	return this->PG;
}

void enableCharger()
{
	pinMode(GPIO_NUM_10, OUTPUT);
	// gpio_hold_dis(GPIO_NUM_10); // Deaktiviere das Halten, falls vorher aktiviert
	digitalWrite(GPIO_NUM_10, HIGH);
	gpio_hold_en(GPIO_NUM_10);
	gpio_deep_sleep_hold_en();
}

void disableCharger()
{
	pinMode(GPIO_NUM_10, OUTPUT);
	gpio_hold_dis(GPIO_NUM_10); // Deaktiviere das Halten, falls vorher aktiviert
	digitalWrite(GPIO_NUM_10, LOW);
	gpio_hold_en(GPIO_NUM_10);
	gpio_deep_sleep_hold_en();
}