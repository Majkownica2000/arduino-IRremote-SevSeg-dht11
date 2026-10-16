#include <Arduino.h>
#include <SevSeg.h>
#include <DHT.h>
#include <IRRemote.hpp>
#include <Bounce2.h>

#define IR_RECEIVE_PIN A1
#define BUTTON_PIN A0
SevSeg sevseg;
DHT dht;
Bounce2::Button button = Bounce2::Button();

bool weatherState = false;

unsigned long codes[] = {
    3910598400, 4077715200, 3877175040, 2707357440, 4144561920, 3810328320, 2774204160, 3175284480, 2907897600, 3041591040};

int codesCount = sizeof(codes) / sizeof(codes[0]);

byte findNumber(unsigned long code)
{
  for (byte i = 0; i < codesCount; i++)
  {
    if (code == codes[i])
    {
      return i;
    }
  }
}

void setup()
{
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // Start the receiver
  Serial.begin(9600);
  button.attach(BUTTON_PIN, INPUT);
  button.interval(5);
  button.setPressedState(LOW);
  byte numDigits = 2;
  byte digitPins[] = {2, 3};
  byte segmentPins[] = {4, 5, 6, 7, 8, 9, 10, 11};
  bool resistorsOnSegments = false;
  byte hardwareConfig = COMMON_ANODE;
  bool updateWithDelays = false;
  bool leadingZeros = false;
  bool disableDecPoint = false;
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
               updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(90);
  dht.setup(12);
}

void loop()
{

  if (IrReceiver.decode())
  {
    unsigned long kod = IrReceiver.decodedIRData.decodedRawData;
    sevseg.setNumber(findNumber(kod));
    Serial.println(kod); // Print the statement required to send this data

    IrReceiver.resume(); // Enable receiving of the next value
  }

  button.update();
  if (button.pressed())
  {
    weatherState = !weatherState;
    sevseg.setNumber(weatherState ? dht.getTemperature() : dht.getHumidity());
  }
  sevseg.refreshDisplay();
}