
#include <Arduino.h>
#include <SevSeg.h>
#include <DHT.h>
#include <IRRemote.hpp>
#include <Bounce2.h>

#define IR_RECEIVE_PIN A1
#define BUTTON_PIN A0
#define MODE_BUTTON_PIN A2
#define IR_CONFIRM_CODE 2306720922
#define WEATHER 960954112
#define NUMBERS 1340550846

SevSeg sevseg;
DHT dht;
Bounce2::Button button = Bounce2::Button();
Bounce2::Button modeButton = Bounce2::Button();

bool weatherState = true; // true = počasí, false = IR ovladač
bool showTemperature = true;

unsigned long codes[] = {
    3373727488, 3272408832, 937712566,
    1543561370, 3339255552, 1320098106,
    1222479582, 1027800832, 3806107130,
    1258404506};

int codesCount = sizeof(codes) / sizeof(codes[0]);

byte enteredNumber = 0;
byte digitCount = 0;
byte confirmedNumber = 0;

byte findNumber(unsigned long code)
{
  for (byte i = 0; i < codesCount; i++)
  {
    if (code == codes[i])
    {
      return i;
    }
  }

  return 255;
}

void showWeather()
{
  float value;

  if (showTemperature)
  {
    value = dht.getTemperature();
  }
  else
  {
    value = dht.getHumidity();
  }

  if (isnan(value))
  {
    sevseg.setChars("--");
    return;
  }

  sevseg.setNumber((int)(value + 0.5f));
}

void setup()
{
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.begin(9600);

  // Tlačítko A0: teplota / vlhkost
  button.attach(BUTTON_PIN, INPUT);
  button.interval(5);
  button.setPressedState(LOW);

  // Tlačítko A2: režim počasí / ovladač
  modeButton.attach(MODE_BUTTON_PIN, INPUT);
  modeButton.interval(5);
  modeButton.setPressedState(LOW);

  byte numDigits = 2;
  byte digitPins[] = {2, 3};
  byte segmentPins[] = {4, 5, 6, 7, 8, 9, 10, 11};

  bool resistorsOnSegments = false;
  byte hardwareConfig = COMMON_ANODE;
  bool updateWithDelays = false;
  bool leadingZeros = false;
  bool disableDecPoint = false;

  sevseg.begin(
      hardwareConfig,
      numDigits,
      digitPins,
      segmentPins,
      resistorsOnSegments,
      updateWithDelays,
      leadingZeros,
      disableDecPoint);

  sevseg.setBrightness(90);
  dht.setup(12);

  showWeather();
}

void loop()
{
  button.update();
  modeButton.update();

  // A2: přepnutí režimu
  if (modeButton.pressed())
  {
    weatherState = !weatherState;

    if (weatherState)
    {
      showWeather();
      Serial.println("Rezim pocasi");
    }
    else
    {
      enteredNumber = 0;
      digitCount = 0;
      sevseg.setNumber(0);
      Serial.println("Rezim IR ovladace");
    }
  }

  // A0: přepnutí teploty / vlhkosti
  if (button.pressed() && weatherState)
  {
    showTemperature = !showTemperature;
    showWeather();
  }

  // IR ovladač
  if (IrReceiver.decode())
  {
    unsigned long kod = IrReceiver.decodedIRData.decodedRawData;

    Serial.println(kod);

    // WEATHER: přepnout přímo do režimu počasí
    if (kod == WEATHER)
    {
      weatherState = true;
      showWeather();
      Serial.println("Rezim pocasi");
    }

    // NUMBERS: přepnout přímo do režimu zadávání čísel
    else if (kod == NUMBERS)
    {
      weatherState = false;
      enteredNumber = 0;
      digitCount = 0;
      sevseg.setNumber(0);
      Serial.println("Rezim IR ovladace");
    }

    // Ostatní IR tlačítka fungují jen v režimu čísel
    else if (!weatherState)
    {
      // Potvrzení čísla
      if (kod == IR_CONFIRM_CODE)
      {
        if (digitCount > 0 &&
            enteredNumber >= 1 &&
            enteredNumber <= 99)
        {
          confirmedNumber = enteredNumber;
          sevseg.setNumber(confirmedNumber);

          Serial.print("Potvrzene cislo: ");
          Serial.println(confirmedNumber);
        }
      }
      else
      {
        byte digit = findNumber(kod);

        if (digit <= 9)
        {
          if (digitCount == 0)
          {
            enteredNumber = digit;
            digitCount = 1;
          }
          else if (digitCount == 1)
          {
            enteredNumber = enteredNumber * 10 + digit;
            digitCount = 2;
          }
          else
          {
            enteredNumber = digit;
            digitCount = 1;
          }

          sevseg.setNumber(enteredNumber);

          Serial.print("Zadane cislo: ");
          Serial.println(enteredNumber);
        }
      }
    }

    IrReceiver.resume();
  }

  sevseg.refreshDisplay();
}
