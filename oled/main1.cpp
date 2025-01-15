#include <Arduino.h>
#include <include.h>

// SPDX-FileCopyrightText: 2011 Limor Fried/ladyada for Adafruit Industries
//
// SPDX-License-Identifier: MIT

// Thermistor Example #3 from the Adafruit Learning System guide on Thermistors
// https://learn.adafruit.com/thermistor/overview by Limor Fried, Adafruit Industries
// MIT License - please keep attribution and consider buying parts from Adafruit

// which analog pin to connect
#define THERMISTORPIN A0
#define RELAYPIN 13
#define VCCPIN A4
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000 // when did I make this 9680? This should be 10000 - did I calibrate the beta coefficient with this like this? Need to recalibrate
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 10
// The beta coefficient of the thermistor (usually 3000-4000)
// #define BCOEFFICIENT 3950
#define BCOEFFICIENT 4074.46
// the value of the 'other' resistor
#define SERIESRESISTOR 9860
// #define TARGETTEMPERATURE 71
#define TARGETTEMPERATURE 71
#define OFFSET 0.66
#define RELAYON LOW
#define RELAYOFF HIGH
#define ADC_ADJUSTMENT 4.5

int samples[NUMSAMPLES];
int vccsamples[NUMSAMPLES];

float getTemperature();
void getDiff();
float getAverageADC();

int state = 0;
int mode = 0;
float previousTemperature = 0;
float currentTemperature = 0;
long counter = 0;
int relayState = RELAYON;

void setup(void)
{
  Serial.begin(9600);
  analogReference(EXTERNAL);
  pinMode(RELAYPIN, OUTPUT);
  Serial.println("LABEL, Date, Time, Current Temp, Target Temp, Adjusted ADC, Info");
  // Serial.println("LABEL, Date, Time, Current Temp, Off Reading, On Reading, Diff");
  digitalWrite(RELAYPIN, RELAYON); // relay on
}

void loop5(void)
{
  // getDiff();
  // delay(5000);
  Serial.println(getAverageADC());
  delay(1000);
}

void loop2(void)
{
  switch (mode)
  {
  case 0: // initial heating state

    currentTemperature = getTemperature();
    if (currentTemperature > TARGETTEMPERATURE - OFFSET)
    {
      mode = 1;
      digitalWrite(RELAYPIN, RELAYOFF); // relay off
    }
    delay(1000);
    break;
  case 1: // reaching peak state
    if (counter % 1 == 0)
    {
      if (digitalRead(RELAYPIN) == HIGH)
      {
        digitalWrite(RELAYPIN, LOW);
        state = 0;
      }
      else
      {
        digitalWrite(RELAYPIN, HIGH);
        state = 1;
      }
    }
    getTemperature();
    counter++;
    delay(1000);
    break;
  }
}

void loop(void)
{
  switch (state)
  {
  case 0: // initial heating state
    currentTemperature = getTemperature();
    if (currentTemperature > TARGETTEMPERATURE - OFFSET)
    {
      state = 1;
      digitalWrite(RELAYPIN, RELAYOFF); // relay off
    }
    delay(1000);
    break;
  case 1: // reaching peak state
    currentTemperature = getTemperature();
    if (currentTemperature < previousTemperature)
    {
      state = 2;
    }
    previousTemperature = currentTemperature;
    delay(1000L * 60); // check every minute so we don't get fooled by noise
    break;
  case 2: // simple on-off state
    currentTemperature = getTemperature();
    if (currentTemperature < TARGETTEMPERATURE)
    {
      digitalWrite(RELAYPIN, RELAYON); // relay on
    }
    else
    {
      digitalWrite(RELAYPIN, RELAYOFF); // relay off
    }
    delay(1000);
    break;
  }
}

float getAverageADC()
{
  uint8_t i;
  float average;

  // take N samples in a row, with a slight delay
  for (i = 0; i < NUMSAMPLES; i++)
  {
    samples[i] = analogRead(THERMISTORPIN);
    // Serial.print((String)samples[i] + " ");
    delay(10);
  }
  Serial.println();

  // average all the samples out
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++)
  {
    average += samples[i];
  }
  average /= NUMSAMPLES;
  return average;
}

float analogToTemperature(float adc)
{
  // convert the value to resistance
  adc = 1023 / adc - 1;
  adc = SERIESRESISTOR / adc;

  double steinhart;
  steinhart = adc / THERMISTORNOMINAL;              // (R/Ro)
  steinhart = log(steinhart);                       // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                        // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                      // Invert
  steinhart -= 273.15;                              // convert absolute temp to C
  return steinhart;
}

void getDiff()
{
  digitalWrite(RELAYPIN, HIGH);
  delay(100);
  float relayOffReading = getAverageADC();
  digitalWrite(RELAYPIN, LOW);
  delay(100);
  float relayOnReading = getAverageADC();
  float temperature = analogToTemperature(relayOffReading);
  float diff = relayOnReading - relayOffReading;

  Serial.println((String) "DATA,DATE, TIME," + temperature + "," + relayOffReading + "," + relayOnReading + "," + diff);
}

float getTemperature()
{

  uint8_t i;
  float average;

  // take N samples in a row, with a slight delay
  for (i = 0; i < NUMSAMPLES; i++)
  {
    samples[i] = analogRead(THERMISTORPIN);
    // Serial.print((String)samples[i] + " ");

    delay(10);
  }

  // average all the samples out
  average = 0;

  for (i = 0; i < NUMSAMPLES; i++)
  {
    average += samples[i];
  }
  average /= NUMSAMPLES;

  if (digitalRead(RELAYPIN) == RELAYON)
  {
    average -= ADC_ADJUSTMENT;
  }

  // Serial.print("Average analog reading ");
  // Serial.println(average);
  float adc = average;
  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  // Serial.print("Thermistor resistance ");
  // Serial.println(average);

  double steinhart;
  steinhart = average / THERMISTORNOMINAL;          // (R/Ro)
  steinhart = log(steinhart);                       // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                        // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                      // Invert
  steinhart -= 273.15;                              // convert absolute temp to C

  // Serial.print("Temperature ");
  Serial.println((String) "DATA,DATE,TIME," + steinhart + "," + TARGETTEMPERATURE + "," + adc + "," + state);

  // printf(steinhart);
  // Serial.println(" *C");

  return steinhart;
}