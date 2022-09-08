#include <Adafruit_NeoPixel.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <HID-Project.h>

template<typename T>
struct TimeStampedValue {
  explicit TimeStampedValue(T value) : _value(value), _timeStamp(0) {}
  void set(const T& value) { _value = value; touch(); }
  operator const T&() const { return _value; }
  void touch() { _timeStamp = millis(); }
  unsigned long getTimeStamp() const { return _timeStamp; }
  
private:
  T _value;
  unsigned long _timeStamp;
};


#define LIGHT_PIN    10
#define VIBRA_PIN    16
#define NR_OF_PIXELS 8

#define TIMEOUT_VIBRA_MS   30
#define TIMEOUT_LIGHTS_MS 600

Adafruit_NeoPixel strip(NR_OF_PIXELS, LIGHT_PIN, NEO_GRB + NEO_KHZ800);
const uint32_t RED   = strip.Color(220,20,60);
const uint32_t GREEN = strip.Color(0,250,154);
const uint32_t BLUE  = strip.Color(255,0,255);
const uint32_t BLACK = strip.Color(0,0,0);

ClickEncoder encoder(A1, A0, A2); //PINS
TimeStampedValue<int16_t> value(0);
int16_t current = 0;
int16_t intensity = 0;

void timerIsr() {
  encoder.service();
}

void setup() {
  Serial.begin(9600);
  
  strip.begin();
  strip.show();

  pinMode(VIBRA_PIN, OUTPUT);  

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  Consumer.begin();
}

void loop() {
  current += encoder.getValue();
  auto diff = current - value;
  if (diff != 0) {
    //Serial.print("Encoder Diff: ");    
    //Serial.println(diff);

    if (diff < 0) {
      intensity = max(1, min(intensity + 1, 10));
      volumeChange(MEDIA_VOL_UP, GREEN);      
    } 
    else {
      intensity = min(-1, max(intensity - 1, -10));
      volumeChange(MEDIA_VOL_DOWN, RED);              
    }
    Serial.println(intensity);
    value.set(current);
  }

  ClickEncoder::Button b = encoder.getButton();
  if (b != ClickEncoder::Released) 
  //Released, Held , Open(default)
  //https://github.com/0xPIT/encoder/issues/14
  
  {
    Serial.println("Button");
    switch (b) {
      case ClickEncoder::Clicked:
        intensity = 9;
        volumeChange(MEDIA_VOL_MUTE, BLUE);              
        value.touch();
        break;
    }
  }
  else {
    //
    // Turn off LEDs / vibra after certain time of inactivity
    //
    if (millis() - value.getTimeStamp() > TIMEOUT_VIBRA_MS) {
      digitalWrite(VIBRA_PIN, LOW);    
    }
    if (millis() - value.getTimeStamp() > TIMEOUT_LIGHTS_MS) {
      setColor(BLACK);
      intensity = 0;
    }
  }
}

void volumeChange(uint16_t key, uint32_t color) {
  digitalWrite(VIBRA_PIN, HIGH);
  setColor(color);
  Consumer.write(key);
}


void setColor(uint32_t c) {
  strip.setBrightness(abs(intensity) * 255 / 10);
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
  }
  strip.show();
}
