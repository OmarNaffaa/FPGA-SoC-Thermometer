#include "chu_init.h"
#include "gpio_cores.h"
#include "sseg_core.h"
#include "i2c_core.h"

/*
 	 Global Variables
*/
constexpr int numOfSsegDisp = 8;
constexpr int celsiusTempUnit = 12; // hex C
constexpr int fahrTempUnit = 15;    // hex F

constexpr int btnInputMask = 3;
enum Buttons {upperBtn = 0, rightBtn = 1, downBtn = 2, leftBtn = 3};

/*
 	 Function Prototypes
*/
float ReadTemp_Celsius(I2cCore *adt7420_p);
void DispSingleTemp(SsegCore *sseg_p, float tempVal, int unit);
void DispBothTemps(SsegCore *sseg_p, float fahrTemp, float celTemp);
void DispOff(SsegCore *sseg_p);
int numOfDigits(int value);

/*
 	 Core Instantiations
*/
DebounceCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
I2cCore adt7420(get_slot_addr(BRIDGE_BASE, S10_I2C));

int main()
{
	float tempC, tempF;

	DispOff(&sseg);

	while (1)
	{
		tempC = ReadTemp_Celsius(&adt7420);
		tempF = ((tempC * 9)/5) + 32;

		switch (btn.read_db() & btnInputMask)
		{
			case upperBtn:
				DispSingleTemp(&sseg, tempC, celsiusTempUnit);
				break;
			case downBtn:
				DispSingleTemp(&sseg, tempF, fahrTempUnit);
				break;
			case leftBtn:
				DispBothTemps(&sseg, tempF, tempC);
				break;
			case rightBtn:
				DispOff(&sseg);
				break;
			default:
				break;
		}

		sleep_ms(100);
	}
}

float ReadTemp_Celsius(I2cCore *adt7420_p) {
   const uint8_t DEV_ADDR = 0x4b;
   uint8_t wbytes[2], bytes[2];
   uint16_t tmp;
   float tmpC;

   // read 2 bytes
   //ack = adt7420_p->read_dev_reg_bytes(DEV_ADDR, 0x0, bytes, 2);
   wbytes[0] = 0x00;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 2, 0);

   // conversion
   tmp = (uint16_t) bytes[0];
   tmp = (tmp << 8) + (uint16_t) bytes[1];
   if (tmp & 0x8000) {
      tmp = tmp >> 3;
      tmpC = (float) ((int) tmp - 8192) / 16;
   } else {
      tmp = tmp >> 3;
      tmpC = (float) tmp / 16;
   }

   return tmpC;
}

void DispSingleTemp(SsegCore *sseg_p, float tempVal, int unit)
{
	/*
	   Displays the obtained voltage on the SSEG
	*/
	int dispVal;
	int decimalPos;
	if (((int)tempVal) >= 100)
	{
		dispVal = ((int) (tempVal * 10));
		decimalPos = 0x20;
	}
	else
	{
		dispVal = ((int) (tempVal * 100));
		decimalPos = 0x40;
	}


	int amntOfDigitsUsed = numOfDigits(dispVal) + 1;

	// turn off unneeded SSeg displays
	int digitIndex = (numOfSsegDisp - amntOfDigitsUsed);
	for (int i = 0; i < digitIndex; ++i)
		sseg_p->write_1ptn(0xff, i);

	sseg_p->set_dp(decimalPos);

	// write value to Sseg display
	int currDispVal;
	uint8_t convertedDispVal;

	// write temperature unit
	convertedDispVal = sseg_p->h2s(unit);
	sseg_p->write_1ptn(convertedDispVal, digitIndex);
	++digitIndex;

	// write temperature value
	while (digitIndex < numOfSsegDisp)
	{
		currDispVal = ((int)dispVal) % 10;
		convertedDispVal = sseg_p->h2s(currDispVal);
		sseg_p->write_1ptn(convertedDispVal, digitIndex);

		++digitIndex;
		dispVal /= 10;
	}
}

void DispBothTemps(SsegCore *sseg_p, float fahrTemp, float celTemp)
{
	DispOff(sseg_p);

	int decimalPos_Fahr, decimalPos_Cel;
	int dispVal_Fahr, dispVal_Cel;

	if (((int)fahrTemp) >= 100)
	{
		dispVal_Fahr = ((int) (fahrTemp * 10));
		decimalPos_Fahr = 0x20;
	}
	else
	{
		dispVal_Fahr = ((int) (fahrTemp * 100));
		decimalPos_Fahr = 0x40;
	}

	if (((int)celTemp) >= 100)
	{
		dispVal_Cel = ((int) (celTemp * 10));
		decimalPos_Cel = 0x02;
	}
	else
	{
		dispVal_Cel = ((int) (celTemp * 100));
		decimalPos_Cel = 0x04;
	}

	int currDispVal;
	uint8_t convertedDispVal;

	int amntOfDigits_Fahr = numOfDigits((int) dispVal_Fahr);
	int amntOfDigits_Cel  = numOfDigits((int) dispVal_Cel);

	// display celsius value
	int digitIndex = 0;
	while (digitIndex < amntOfDigits_Cel)
	{
		currDispVal = ((int)dispVal_Cel) % 10;
		convertedDispVal = sseg_p->h2s(currDispVal);
		sseg_p->write_1ptn(convertedDispVal, digitIndex);

		++digitIndex;
		dispVal_Cel /= 10;
	}

	// display fahrenheit value
	digitIndex = 4;
	while (digitIndex < (amntOfDigits_Fahr+4))
	{
		currDispVal = ((int)dispVal_Fahr) % 10;
		convertedDispVal = sseg_p->h2s(currDispVal);
		sseg_p->write_1ptn(convertedDispVal, digitIndex);

		++digitIndex;
		dispVal_Fahr /= 10;
	}
	sseg_p->set_dp(decimalPos_Fahr + decimalPos_Cel);
}

void DispOff(SsegCore *sseg_p)
{
	int i;
	for (i = 0; i < 8; i++)
		sseg.write_1ptn(0xff, i);
	sseg.set_dp(0x00);
}

int numOfDigits(int value)
{
	int digits = 1;

	while (value / 10 != 0)
	{
		++digits;
		value /= 10;
	}

	return digits;
}
