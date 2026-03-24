#include <Arduino.h>
#include <FastLED.h>

#define DATA_PIN    5
#define INC_MODE    8
#define INC_COL     9

#define LED_S       8
#define BRIGHTNESS  255
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define UPDATES_PER_SECOND 100
#define DELAY 100
// Define the array of leds
CRGB leds[LED_S];

struct col{
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct HSV{
  float h;
  float s;
  float v;
};

col C = {0xFF, 0x00, 0x00};
col inside = C;
HSV Chsv = {0, 1, 1}; // standard red HSV

col gradient[LED_S] = {
{0xFF, 0x00, 0x00},
{0xDF, 0x00, 0x00},
{0xCF, 0x00, 0x00},
{0xAF, 0x00, 0x00},
{0x8F, 0x00, 0x00},
{0x4F, 0x00, 0x00},
{0x2F, 0x00, 0x00},
{0x00, 0x00, 0x00},
};

HSV hgrad[LED_S] = {
{0, 1, 1},
{0, 1, .9},
{0, 1, .7},
{0, 1, .5},
{0, 1, .3},
{0, 1, .1},
{0, 1, .0},
{0, 1, .0},
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

HSV RGBtoHSV(col colour){
  float r = colour.r / 255.f;
  float g = colour.g / 255.f;
  float b = colour.b / 255.f;

  float cmax = maximum3(r,g,b);
  float cmin = minimum3(r,b,b);
  float delta = cmax - cmin;

  HSV ret;

  if(delta < 1e-6f){
    ret.h = 0.f;
  } else if (cmax == r){
    ret.h = 60.f * fmodf(((g - b) / delta), 6.f);
  } else if (cmax == g){
    ret.h = 60.f * (((b - r) / delta) + 2.f);
  } else{
    ret.h = 60.f * (((r - g) / delta) + 4.f);
  }

  if(ret.h < 0.f) ret.h += 360.f;

  ret.s = (cmax <= 0.f) ? 0.f : (delta/cmax);

  ret.v = cmax;

  if(ret.h < 0.f) ret.h = 0.f;
  if(ret.h > 360.f) ret.h = 360.f;
  if(ret.s < 0.f) ret.h = 0.f;
  if(ret.s > 1.f) ret.h = 1.f;
  if(ret.v < 0.f) ret.h = 0.f;
  if(ret.v > 1.f) ret.h = 1.f;

  return ret;
}

col HSVtoRGB(HSV col){
  col.h = fmodf(col.h, 360.0f);
  if(col.h < 0) col.h += 360.0f;
  if(col.s < 0) col.s = 0; else if (col.s > 1) col.s = 1;
  if(col.v < 0) col.v = 0; else if (col.v > 1) col.v = 1;

  float c = col.v * col.s;
  float x = c *(1.0f - fabsf(fmodf(col.h / 60.0f, 2.0f) - 1.0f));
  float m = col.v - c;

  float r1, g1, b1;
  if (col.h < 60.0f) {
    r1 = c; g1 = x; b1 = 0;
  } else if (col.h < 120.0f) {
    r1 = x; g1 = c; b1 = 0;    
  } else if (col.h < 180.0f) {
    r1 = 0; g1 = c; b1 = x;    
  } else if (col.h < 240.0f) {
    r1 = 0; g1 = x; b1 = c;    
  } else if (col.h < 300.0f) {
    r1 = x; g1 = 0; b1 = c;    
  } else {
    r1 = c; g1 = 0; b1 = x;    
  }

  return {
    (uint8_t)roundf((r1 + m) * 255.0f),
    (uint8_t)roundf((g1 + m) * 255.0f),
    (uint8_t)roundf((b1 + m) * 255.0f)    
  };
}


void Mult(col* color, float mult){
  color->r = color->r * mult;
  color->g = color->g * mult;
  color->b = color->b * mult;
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

inline void ScanOne(col colour){
  for(int i = 0; i < LED_S; ++i){
    if(i == wrap(offset))
      leds[i].setRGB(colour.r, colour.g, colour.b);
    else
      leds[i].setRGB(0,0,0);
  }

  offset++;
  if(offset >= LED_S) offset = 0;
}

float fade = 0;
bool isUp = 1;
inline void FadeInOut(col Color, float fade){
  HSV hsv = RGBtoHSV(Color);
  hsv.v -= fade;
  Color = HSVtoRGB(hsv);

  Constant(Color);
  FastLED.show();
}

inline void ScanTwo(col Color){
  for(int i = 0; i < LED_S; ++i){
    if(i == wrap(offset) || i == wrap(offset + LED_S/2))
      leds[i].setRGB(Color.r, Color.g, Color.b);
    else
      leds[i].setRGB(0,0,0);
  }

  offset++;
  if(offset >= LED_S/2) offset = 0;
}

inline void ScanGrad(){
  for(int i = 0; i < LED_S; ++i){
      leds[i] = (CRGB){
      gradient[wrap(i + offset)].r, 
      gradient[wrap(i + offset)].g,
      gradient[wrap(i + offset)].b};
  }

  offset++;
  if(offset >= LED_S) offset = 0;
}

inline void Constant(col Color){
  for(int i = 0; i < LED_S; ++i){
    leds[i] = (CRGB){Color.r, Color.g, Color.b};
  }
}

uint8_t mode = 0;

void setup() {
  Mult(&inside, 0.25);

  Serial.begin(115200);
  pinMode(INC_MODE, INPUT);
  pinMode(INC_COL, INPUT);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, LED_S);
  FastLED.setBrightness(BRIGHTNESS);
}


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
    Chsv.h += 10;
    C = HSVtoRGB(Chsv);
    Serial.print(C.r); Serial.print('\t');
    Serial.print(C.g); Serial.print('\t');
    Serial.print(C.b); Serial.print('\t');
    Serial.print(Chsv.h); Serial.print('\t');
    Serial.print(Chsv.s); Serial.print('\t');
    Serial.print(Chsv.v); Serial.println();

    for(int i = 0; i < LED_S; ++i){
      hgrad[i].h += 10.f;
      if(hgrad[i].h > 360.f) hgrad[i].h = 10.f;
      gradient[i] = HSVtoRGB(hgrad[i]);
    }
  }

  switch(mode){
    case 0:
      Constant((col){0,0,0});
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
      Constant((col){0x10, 0x10, 0x10});
      FastLED.show();
      delay(DELAY);
      break;
  }

  if(isUp) fade+= .1f;
  else fade-= .1f;


  if(fade >= 1) isUp = 0;
  if(fade <= 0) isUp = 1;
}
