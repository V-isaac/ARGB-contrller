#include <Arduino.h>
#include <FastLED.h>

#define DATA_PIN    5
#define INC_MODE    10
#define INC_COL     9
#define INC_VAL     8

#define LED_S       8
#define BRIGHTNESS  255
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define UPDATES_PER_SECOND 100
#define DELAY 100
// Define the array of leds
CRGB leds[LED_S];

struct ReGrBl{ uint8_t r, g, b; };

struct HuSaVa{ float h, s, v; };

struct Colour;

void RGBtoHSV(Colour* c);
void HSVtoRGB(Colour* c);

struct Colour{
  ReGrBl ColRGB;
  HuSaVa ColHSV;

  void SetRGB(const Colour& c){
    ColRGB = c.ColRGB;
    RGBtoHSV(this);
  }
  void SetHSV(const Colour& c){
    ColHSV = c.ColHSV;
    HSVtoRGB(this);
  }
};

static inline float minimum(float a, float b){
  if(a > b) return b;
  else return a;
}

static inline float maximum(float a, float b){
  if(a < b) return b;
  else return a;
}

static inline float minimum3(float a, float b, float c){
  return minimum(a, minimum(b,c));
}

static inline float maximum3(float a, float b, float c){
  return maximum(a, maximum(b,c));
}

void RGBtoHSV(Colour* col){
  float r = col->ColRGB.r / 255.f;
  float g = col->ColRGB.g / 255.f;
  float b = col->ColRGB.b / 255.f;

  float cmax = maximum3(r,g,b);
  float cmin = minimum3(r,b,b);
  float delta = cmax - cmin;

  if(delta < 1e-6f){
    col->ColHSV.h = 0.f;
  } else if (cmax == r){
    col->ColHSV.h = 60.f * fmodf(((g - b) / delta), 6.f);
  } else if (cmax == g){
    col->ColHSV.h = 60.f * (((b - r) / delta) + 2.f);
  } else{
    col->ColHSV.h = 60.f * (((r - g) / delta) + 4.f);
  }

  // return
  if(col->ColHSV.h < 0.f) col->ColHSV.h += 360.f;
  col->ColHSV.s = (cmax <= 0.f) ? 0.f : (delta/cmax);
  col->ColHSV.v = cmax;

  // clamp 
  if(col->ColHSV.h < 0.f) col->ColHSV.h = 0.f;
  if(col->ColHSV.h > 360.f) col->ColHSV.h = 360.f;
  if(col->ColHSV.s < 0.f) col->ColHSV.h = 0.f;
  if(col->ColHSV.s > 1.f) col->ColHSV.h = 1.f;
  if(col->ColHSV.v < 0.f) col->ColHSV.h = 0.f;
  if(col->ColHSV.v > 1.f) col->ColHSV.h = 1.f;
}

void HSVtoRGB(Colour* col){
  col->ColHSV.h = fmodf(col->ColHSV.h, 360.0f);
  if(col->ColHSV.h < 0) col->ColHSV.h += 360.0f;
  if(col->ColHSV.s < 0) col->ColHSV.s = 0;
    else if (col->ColHSV.s > 1) col->ColHSV.s = 1;
  if(col->ColHSV.v < 0) col->ColHSV.v = 0; 
    else if (col->ColHSV.v > 1) col->ColHSV.v = 1;

  float c = col->ColHSV.v * col->ColHSV.s;
  float x = c *(1.0f - fabsf(fmodf(col->ColHSV.h / 60.0f, 2.0f) - 1.0f));
  float m = col->ColHSV.v - c;

  float r1, g1, b1;
  if (col->ColHSV.h < 60.0f) {
    r1 = c; g1 = x; b1 = 0;
  } else if (col->ColHSV.h < 120.0f) {
    r1 = x; g1 = c; b1 = 0;    
  } else if (col->ColHSV.h < 180.0f) {
    r1 = 0; g1 = c; b1 = x;    
  } else if (col->ColHSV.h < 240.0f) {
    r1 = 0; g1 = x; b1 = c;    
  } else if (col->ColHSV.h < 300.0f) {
    r1 = x; g1 = 0; b1 = c;    
  } else {
    r1 = c; g1 = 0; b1 = x;    
  }

  // return
  col->ColRGB.r = (uint8_t)roundf((r1 + m) * 255.0f);
  col->ColRGB.g = (uint8_t)roundf((g1 + m) * 255.0f);
  col->ColRGB.b = (uint8_t)roundf((b1 + m) * 255.0f);   
}


//Colour C = {0xF0, 0x00, 0xFF};
Colour C = {0xFF, 0x00, 0x00};
Colour inside = C;

Colour gradient[LED_S] = {
    {{0xF0, 0x00, 0xFF}, {}},
    {{0xD0, 0x00, 0xDF}, {}},
    {{0xC0, 0x00, 0xCF}, {}},
    {{0xA0, 0x00, 0xAF}, {}},
    {{0x80, 0x00, 0x8F}, {}},
    {{0x40, 0x00, 0x4F}, {}},
    {{0x20, 0x00, 0x2F}, {}},
    {{0x00, 0x00, 0x00}, {}},
};

void Mult(Colour* color, float mult){
  color->ColHSV.v = color->ColHSV.v * mult;
}

inline bool CheckMode(){
  return digitalRead(INC_MODE);
}

int wrap(int kx) {
  int range = LED_S;
  kx = (kx) % range;

  if (kx < 0)
    kx += range;

    return kx;
}

int offset = 0;

inline void ScanOne(Colour c){
  for(int i = 0; i < LED_S; ++i){
    if(i == wrap(offset))
      leds[i].setRGB(c.ColRGB.r, c.ColRGB.g, c.ColRGB.b);
    else
      leds[i].setRGB(0,0,0);
  }

  offset++;
  if(offset >= LED_S) offset = 0;
}

float fade = 0;
bool isUp = 1;

inline void FadeInOut(Colour c, float fade){
  c.ColHSV.v -= fade;
  c.SetHSV(c);

  Constant(c);
  FastLED.show();
}

inline void ScanTwo(Colour c){
  for(int i = 0; i < LED_S; ++i){
    if(i == wrap(offset) || i == wrap(offset + LED_S/2))
      leds[i].setRGB(c.ColRGB.r, c.ColRGB.g, c.ColRGB.b);
    else
      leds[i].setRGB(0,0,0);
  }

  offset++;
  if(offset >= LED_S/2) offset = 0;
}

inline void ScanGrad(){
  for(int i = 0; i < LED_S; ++i){
      leds[i] = (CRGB){
      gradient[wrap(i + offset)].ColRGB.r, 
      gradient[wrap(i + offset)].ColRGB.g,
      gradient[wrap(i + offset)].ColRGB.b};
  }

  offset++;
  if(offset >= LED_S) offset = 0;
}

inline void Constant(Colour c){
  for(int i = 0; i < LED_S; ++i){
    leds[i] = (CRGB){c.ColRGB.r, c.ColRGB.g, c.ColRGB.b};
  }
}

uint8_t mode = 0;

void setup() {
  Serial.begin(115200);
  Mult(&inside, 0.25);
  inside.SetHSV(inside);

  C.SetRGB(C);
  for(int i = 0; i < LED_S; ++i){
    gradient[i].SetRGB(gradient[i]);
  }

  Serial.println("initial color");
  Serial.println("r\tg\tb\th\ts\tv");
  Serial.print(C.ColRGB.r); Serial.print('\t');
  Serial.print(C.ColRGB.g); Serial.print('\t');
  Serial.print(C.ColRGB.b); Serial.print('\t');
  Serial.print(C.ColHSV.h); Serial.print('\t');
  Serial.print(C.ColHSV.s); Serial.print('\t');
  Serial.print(C.ColHSV.v); Serial.println();


  pinMode(INC_MODE, INPUT);
  pinMode(INC_COL, INPUT);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, LED_S);
  FastLED.setBrightness(BRIGHTNESS);
}

int valUp = 0;

void loop() {
  if(digitalRead(INC_MODE)){
    delay(500);
    mode++;
    if(mode > 5) mode = 0;
    Serial.println(mode);
  }

  if(digitalRead(INC_COL)){
    delay(100);
    Serial.println("changed color");
    Serial.println("r\tg\tb\th\ts\tv");
    C.ColHSV.h += 10;
    C.SetHSV(C);
    Serial.print(C.ColRGB.r); Serial.print('\t');
    Serial.print(C.ColRGB.g); Serial.print('\t');
    Serial.print(C.ColRGB.b); Serial.print('\t');
    Serial.print(C.ColHSV.h); Serial.print('\t');
    Serial.print(C.ColHSV.s); Serial.print('\t');
    Serial.print(C.ColHSV.v); Serial.println();

    for(int i = 0; i < LED_S; ++i){
      gradient[i].ColHSV.h += 10.f;
      if(gradient[i].ColHSV.h > 361.f) gradient[i].ColHSV.h = 10.f;
      gradient[i].SetHSV(gradient[i]);
    }
  }

  if(digitalRead(INC_VAL)){
    delay(100);
    Serial.println("changed value");
    Serial.println("r\tg\tb\th\ts\tv");
    if(!valUp){
      C.ColHSV.v -= .10;
      if(C.ColHSV.v < 0){
        C.ColHSV.v = 0;
        valUp = 1;
      } 
    } else{
      C.ColHSV.v += .10;
      if(C.ColHSV.v > 1){
        C.ColHSV.v = 1;
        valUp = 0;
      }
    }

    C.SetHSV(C);
    Serial.print(C.ColRGB.r); Serial.print('\t');
    Serial.print(C.ColRGB.g); Serial.print('\t');
    Serial.print(C.ColRGB.b); Serial.print('\t');
    Serial.print(C.ColHSV.h); Serial.print('\t');
    Serial.print(C.ColHSV.s); Serial.print('\t');
    Serial.print(C.ColHSV.v); Serial.println();

    /*
    for(int i = 0; i < LED_S; ++i){
      gradient[i].ColHSV.h += 10.f;
      if(gradient[i].ColHSV.h > 361.f) gradient[i].ColHSV.h = 10.f;
      gradient[i].SetHSV(gradient[i]);
    }
    */
  }

  switch(mode){
    case 0:
      Constant((Colour){{0,0,0},{}});
        FastLED.show();
      break;
    case 1:
      Constant(C);
      FastLED.show();
      break;
    case 2:
      ScanOne(C);
      FastLED.show();
      delay(DELAY);
      break;
    case 3:
      ScanTwo(C);
      FastLED.show();
      delay(DELAY);
      break;
    case 4:
      ScanGrad();
      FastLED.show();
      delay(DELAY);
      break;
    case 5:
      FadeInOut(C, fade);
      delay(50);
      break;
    default:
      Constant((Colour){{0x10, 0x10, 0x10}, {}});
      FastLED.show();
      delay(DELAY);
      break;
  }

  if(isUp) fade+= .1f;
  else fade-= .1f;


  if(fade >= 1) isUp = 0;
  if(fade <= 0) isUp = 1;
}
