/* Starting with Arduino OLED coding
 *  for " arduino oled i2c tutorial : 0.96" 128 X 32 for beginners"
 *  subscribe for more arduino Tuorials and Projects
https://www.youtube.com/channel/UCM6rbuieQBBLFsxszWA85AQ?sub_confirmation=1
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ezButton.h>

// Display settings
#define OLED_RESET 4
#define VRX_PIN A1 // 0: down, 1023: up
#define VRY_PIN A2 // 0: left, 1023: right
#define SW_PIN 8
#define SELECT_CHANGE_COOLDOWN 100
#define ADC_HIGH_THRESOLD 900
#define ADC_LOW_THRESHOLD 100
#define ADC_OFFSET_THRESHOLD 100
#define MIN_COOKING_TIME_THRESHOLD 1800
#define MIN_TARGET_TEMP_THRESHOLD 45
Adafruit_SSD1306 display(OLED_RESET);
ezButton button(SW_PIN);

// Heating settings
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
#define OFFSET 0.66
#define RELAYON LOW
#define RELAYOFF HIGH
#define ADC_ADJUSTMENT 4.5

int xValue = 0;
int yValue = 0;
int bValue = 0;
float targetTemp = 50;
unsigned long remainingSeconds = 10;
int currentSelection = 0;
unsigned long prevChangeTime = millis();
int programState = 0;
int heatingState = 0;
String formattedTime = "";

int samples[NUMSAMPLES];
float getTemperature();
float previousTemperature = 0;
float currentTemperature = 0;

void setup()
{
    // cli(); // stops interrupts
    analogReference(EXTERNAL);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    // button.setDebounceTime(1); // after testing, we don't need debouncing - it only makes responsiveness worse
    display.clearDisplay();
    display.setRotation(2);
    Serial.begin(9600);
    pinMode(RELAYPIN, OUTPUT);
    Serial.println("LABEL, Date, Time, Current Temp, Target Temp, Adjusted ADC, Info");
    digitalWrite(RELAYPIN, RELAYOFF);

    // set timer1 interrupt at 1Hz
    TCCR1A = 0; // set entire TCCR1A register to 0
    TCCR1B = 0; // same for TCCR1B
    TCNT1 = 0;  // initialize counter value to 0
    // set compare match register for 1hz increments
    OCR1A = 15624; // = (16*10^6) / (1*1024) - 1 (must be <65536)
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS10 and CS12 bits for 1024 prescaler
    TCCR1B |= (1 << CS12) | (1 << CS10);
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);
}

String formatTime(unsigned long totalSeconds)
{
    unsigned long hours = totalSeconds / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = (totalSeconds % 3600) % 60;
    return (hours < 10 ? "0" : "") + String(hours) + ":" +
           (minutes < 10 ? "0" : "") + String(minutes) + ":" +
           (seconds < 10 ? "0" : "") + String(seconds);
}

void loop()
{
    button.loop();

    switch (programState)
    {
    case 0:
    {

        xValue = analogRead(VRX_PIN);
        yValue = analogRead(VRY_PIN);

        Serial.println((String)xValue + " " + (String)yValue);
        bValue = button.getState();

        if (button.isPressed())
        {
            if (currentSelection == 2)
            {
                programState = 1;
                currentTemperature = getTemperature();
                digitalWrite(RELAYPIN, RELAYON);
                break;
            }
        }

        if (millis() - prevChangeTime > SELECT_CHANGE_COOLDOWN)
        {
            if (xValue < ADC_LOW_THRESHOLD)
            {
                currentSelection = (currentSelection - 1 + 3) % 3;
            }
            else if (xValue > ADC_HIGH_THRESOLD)
            {
                currentSelection = (currentSelection + 1) % 3;
            }
            prevChangeTime = millis();
        }

        if (currentSelection == 0)
        {
            if (yValue > ADC_HIGH_THRESOLD)
            {
                targetTemp += 0.5;
            }
            else if (yValue < ADC_LOW_THRESHOLD && targetTemp > MIN_TARGET_TEMP_THRESHOLD)
            {
                targetTemp -= 0.5;
            }
        }
        else if (currentSelection == 1)
        {
            if (yValue > ADC_HIGH_THRESOLD)
            {
                remainingSeconds += 300;
            }
            else if (yValue < ADC_LOW_THRESHOLD && remainingSeconds > MIN_COOKING_TIME_THRESHOLD)
            {
                remainingSeconds -= 300;
            }
        }

        formattedTime = formatTime(remainingSeconds);

        display.setTextSize(1);
        display.setTextColor(WHITE);

        display.clearDisplay();
        display.setCursor(0, 0);

        display.print("Set Target:");
        display.setCursor(75, 0);
        display.print(targetTemp, 1);
        display.print(" C");
        display.setCursor(0, 8);
        display.println("Set Timer:");
        display.setCursor(75, 8);
        display.println(formattedTime);
        display.setCursor(75, 16);
        display.println("Confirm");
        display.setCursor(1, 0);

        display.setCursor(68, currentSelection * 8);
        display.print("[");
        display.setCursor(122, currentSelection * 8);
        display.print("]");

        display.display();
        break;
    }
    case 1:
    {
        if (button.isPressed())
        {
            targetTemp = 50;
            remainingSeconds = 3600;
            programState = 0;
            heatingState = 0;
            currentSelection = 0;
            digitalWrite(RELAYPIN, RELAYOFF);
            break;
        }
        sei(); // allows interrupts

        formattedTime = formatTime(remainingSeconds);

        display.clearDisplay();
        display.setCursor(0, 0);

        display.print("Target:");
        display.setCursor(75, 0);

        display.print(targetTemp, 1);
        display.print(" C");
        display.setCursor(0, 8);
        display.println("Time Left:");
        display.setCursor(75, 8);
        display.println(formattedTime);
        display.setCursor(0, 16);
        display.print("Temperature: ");
        display.setCursor(75, 16);
        display.print(currentTemperature, 1);
        display.print(" C");
        display.setCursor(0, 24);

        switch (heatingState)
        {
        case 0: // initial heating state

            if (millis() - prevChangeTime > 1000)
            {
                prevChangeTime = millis();
                currentTemperature = getTemperature();
                if (currentTemperature > targetTemp - OFFSET)
                {
                    heatingState = 1;
                    prevChangeTime = 0;
                    digitalWrite(RELAYPIN, RELAYOFF); // relay off
                }
            }
            display.print("Heating up...           ");
            break;
        case 1: // reaching peak state
            if (millis() - prevChangeTime > 1000L * 60)
            {
                prevChangeTime = millis();
                currentTemperature = getTemperature();
                if (currentTemperature < previousTemperature)
                {
                    heatingState = 2;
                    prevChangeTime = 0;
                }
                previousTemperature = currentTemperature;
            }
            display.print("Looking for peak...         ");
            break;
        case 2: // simple on-off state
            if (millis() - prevChangeTime > 1000)
            {
                prevChangeTime = millis();
                currentTemperature = getTemperature();
                if (currentTemperature < targetTemp)
                {
                    digitalWrite(RELAYPIN, RELAYON); // relay on
                }
                else
                {
                    digitalWrite(RELAYPIN, RELAYOFF); // relay off
                }
            }
            display.print("Stabilizing...           ");
            break;
        }
        display.display();

        if (remainingSeconds <= 0)
        {
            programState = 2;
            digitalWrite(RELAYPIN, RELAYOFF);
        }

        break;
    }
    case 2:
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);

        display.println("Cooking complete!");
        display.setCursor(0, 16);
        display.println("Press the button to");
        display.println("start over...");
        display.display();

        break;
    }
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
    Serial.println((String) "DATA,DATE,TIME," + steinhart + "," + targetTemp + "," + adc + "," + heatingState);

    // printf(steinhart);
    // Serial.println(" *C");

    return steinhart;
}

ISR(TIMER1_COMPA_vect)
{
    if (heatingState == 2)
    {
        remainingSeconds--;
    }
}