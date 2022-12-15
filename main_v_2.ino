/*Version 7
   LED improvements with
   LED address change and green colour delay increase
*//**********************************************
  Main code
***********************************************/
#include "arduino_secrets.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_NeoPixel.h>



//Pin Defentions
#define NUMPIXELS 6
#define PIXEL_PIN D8
Adafruit_NeoPixel pixels(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
int solenoidPin = D4;
int batt_pin = D3;
int batt_pin2 = 12;
int ssr_pin = D2;

int16_t fid;
int ss_pin = D9;
//Intiazlizing Rpm pins
const int rpm_pin = D5;
volatile unsigned long cntTime = 0;
volatile unsigned long cnt = 0;
volatile int rpm = -1;
//Intializing relay locking pin
const int relay_pin = D7;
//Intializing the side stand pin

//Stand, motor and solenoid States
int stand_down = HIGH;
int stand_up = LOW;
int Bat_open = HIGH;
int Bat_close = LOW;
int Motor_locked = HIGH;
int Motor_unlocked = LOW;
int button_pressed = HIGH;
int button_unpressed = LOW;
volatile bool lock_state = true;
int count = 0;
int val = -1;

#include "thingProperties.h"
#include <Adafruit_Fingerprint.h>

#define mySerial Serial2


Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

int myUser[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int a;




void doCount() //interrupt callback should be as short as possible!
{
  if (digitalRead(rpm_pin) == LOW)
  {
    cnt++;
    cntTime = millis();
  }
}
void setup()
{
  //float LvVoltage = readVoltage(34);
  //Serial.println(BatteryVoltage);
  //Serial.println(LvVoltage);

  Serial.begin(115200);
  delay(1000);
  pinMode(ss_pin, INPUT) ;

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");

  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);



  //Initalization for Solednoid
  pinMode(solenoidPin, OUTPUT);
  //Initialization for RPM
  pinMode(rpm_pin, INPUT_PULLUP);
  digitalWrite(rpm_pin, HIGH);
  attachInterrupt(rpm_pin, doCount, FALLING);
  cntTime = millis();
  //Intialization of the Side Stand
  pinMode(ss_pin, INPUT);
  //Intialization of the Relay
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, HIGH);

  pinMode(batt_pin, OUTPUT);
  pinMode(batt_pin2, INPUT_PULLUP);
  pinMode(ssr_pin, OUTPUT);
}

void loop()
{
  pixels.clear();
  pixels.setBrightness(50);


  a = digitalRead(ss_pin);
  if (a == 1 ) {
    ArduinoCloud.update();

  }
  else {
    Serial.println("NOt Connected to IoT");
  }

  if (digitalRead(ss_pin) == LOW)
  {
    if (lock_state == true)
    {
      val = getFingerprintID();
      if (myUser[val - 1] == 1)
      {
        if (digitalRead(ss_pin) == LOW)
        {


          digitalWrite(relay_pin, LOW); //UNLOCK
          digitalWrite(ssr_pin, LOW); // high
          Serial.println("Motor Should be locked");
          pixels.setPixelColor(0, pixels.Color(0, 255, 0)); //green
          pixels.show();
          lock_state = false;
          Serial.print("State should be false:"); Serial.println(lock_state);
        }
        val = -1;
      }
      else
      {

        digitalWrite(relay_pin, HIGH);
        digitalWrite(ssr_pin, HIGH); //low
        Serial.println("Wrong Finger Print");
        Serial.println("Motor Should be unlocked");
        pixels.setPixelColor(0, pixels.Color(255, 0, 0)); //red
        pixels.show();
      }
    }
    else if (digitalRead(ss_pin) == LOW)
    {
      Serial.println("Passed through first if statement");
      if (lock_state == false)
      {
        Serial.println("Do Nothing");
        pixels.setPixelColor(0, pixels.Color(0, 100, 0));
        pixels.setPixelColor(1, pixels.Color(0, 100, 0));
        pixels.setPixelColor(2, pixels.Color(100, 100, 0));
        pixels.setPixelColor(3, pixels.Color(100, 100, 0));
        pixels.setPixelColor(4, pixels.Color(100, 0, 0));
        pixels.show();
        digitalWrite(relay_pin, LOW);
        digitalWrite(ssr_pin, LOW); //high

      }
    }
    else
    {
      Serial.println("Pls get the side stand up");
      pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      pixels.show();
      digitalWrite(relay_pin, HIGH);
      digitalWrite(ssr_pin, HIGH); //low
      pixels.setPixelColor(0, pixels.Color(0, 255, 0));
      pixels.show();
      lock_state = true;
      //Serial.print("State should be true:");Serial.println(lock_state);
      Serial.print("State should be true:"); Serial.println(lock_state);
    }
  }
  if (digitalRead(batt_pin) == HIGH || digitalRead(batt_pin2) == LOW)
  {
    if (lock_state == true)
    {
      if (digitalRead(ss_pin) == HIGH)
      {
        val = getFingerprintID();
        if (myUser[val - 1] == 1)
        {
          Serial.println("Battery Button Pressed");
          digitalWrite(solenoidPin, HIGH);
          delay(5000);
          digitalWrite(solenoidPin, LOW);
          Serial.println("Out of Solenoid Loop");
        }
        val = -1;
      }
    }
  }



  else
  {
    // Serial.println("No Free Slot in Flash Library");//add database wipe
  }


  /* if (digitalRead(ss_pin) == HIGH)
    {
     int batt_cap = battery();
     if (batt_cap == 60)
     {
       pixels.setPixelColor(1, pixels.Color(100, 0, 0));
       pixels.setPixelColor(2, pixels.Color(100, 100, 0));
       pixels.setPixelColor(3, pixels.Color(100, 100, 0));
       pixels.setPixelColor(4, pixels.Color(0, 100, 0));
       pixels.show();
     }
     //digitalWrite(ssr_pin,LOW);
     delay(3000);
     if (digitalRead(ss_pin) == HIGH)
     {
       pixels.setPixelColor(0, pixels.Color(255, 0, 0));
       pixels.show();
       //delay(1000);
       Serial.println("Sidestand Condition");
       lock_state = true;
       Serial.print("State should be true:"); Serial.println(lock_state);
       digitalWrite(relay_pin, HIGH );
       digitalWrite(ssr_pin, HIGH); //low
     }
    }*/
  // Serial.print("State at the end of the code:"); Serial.println(lock_state);
  //Serial.println("End of the loop");
}




//Miscellaneous functions

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}




uint8_t getFingerprintEnroll() {
  pixels.setPixelColor(0, pixels.Color(255, 255, 0));
  pixels.show();
  int p = -1;

  int id;
  for (int i = 0; i < 10; i++)
  {
    if (myUser[i] == 0)
    {
      id = i + 1;
      myUser[i] = 1;
      break;
    }
    else {
      continue;
    }

  }


  Serial.print("Enrolling ID #");
  Serial.println(id);

  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}




/*Since Enroll is READ_WRITE variable, onEnrollChange() is
  executed every time a new value is received from IoT Cloud.*/

void onEnrollChange()  {
  // Add your code here to act upon Enroll change
  if (enroll && a == 1)
  {
    Serial.println("Ready to enroll a fingerprint!");
    getFingerprintEnroll();

  }
}

void onBatteryBayChange()  {
  // Add your code here to act upon BatteryBay change
  if (battery_bay && a == 1);
  {
    val = getFingerprintID();
    if(myUser[val]==1)
    {
      Serial.println("Battery bay is open");
    }
  }
}
