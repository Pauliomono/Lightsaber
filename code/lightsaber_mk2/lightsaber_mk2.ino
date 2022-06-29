#include <Adafruit_LSM6DS33.h>
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <Audio.h>
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>
#include <play_fs_wav.h>
#include <Bounce2.h>

//accel declarations
Adafruit_LSM6DS33 lsm6ds33;
#define ax accel.acceleration.x
#define ay accel.acceleration.y
#define az accel.acceleration.z
#define gx gyro.acceleration.x
#define gy gyro.acceleration.y
#define gz gyro.acceleration.z

//var declarations
float a;
float v;
float angle;
float t0 = 0;
float t1;
float dt;
int hitfx;
int pixel;
int state;
bool swt;
bool power = false;
bool hstat;
float vvol;
float avol;

//lights declarations
#define NUM_PIXELS 144
#define POWER_PIN 10
#define NEOPIXEL_PIN 5
#define swing 1
#define hit 35
#define start1 50
#define end1 100
#define start2 180
#define end2 330
CRGB leds[NUM_PIXELS];
CRGB bat[1];

//audio declarations
AudioPlayFSWav           playWav1;
AudioPlayFSWav           playWav2;
AudioPlayFSWav           playWav3;
AudioPlayFSWav           playWav4;
AudioMixer4              mixer1;
AudioOutputAnalogStereo  audioOutput;    // Dual DACs
AudioConnection          patchCord1(playWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playWav2, 0, mixer1, 1);
AudioConnection          patchCord3(playWav3, 0, mixer1, 2);
AudioConnection          patchCord4(playWav4, 0, mixer1, 3);
AudioConnection          patchCord5(mixer1, 0, audioOutput, 0);

//storage declaratons
Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS, PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;

//switch
Bounce switch1 = Bounce();

void setup() {

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
//switch setup
switch1.attach(9,INPUT_PULLUP);
switch1.interval(10); 
 
//initialize storage
flash.begin();
fatfs.begin(&flash);

//accelerometer initialize
  Serial.begin(9600);
  Serial.println("Starting LSM6DS33...");
  if (!lsm6ds33.begin_I2C()) { 
    Serial.println("Couldnt start LSM6DS33");
    while (1);
  }
lsm6ds33.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
lsm6ds33.setGyroRange(LSM6DS_GYRO_RANGE_500_DPS);

lsm6ds33.configInt1(false, false, true); // accelerometer DRDY on INT1
lsm6ds33.configInt2(false, true, false); // gyro DRDY on INT2

//audio initialize
AudioMemory(64);

//enable wing
pinMode(POWER_PIN, OUTPUT);
digitalWrite(POWER_PIN, LOW);

//initialize battery indicator
FastLED.addLeds<WS2812B, 8, GRB>(bat, 1);
//initialize lights
FastLED.addLeds<WS2812B, NEOPIXEL_PIN, GRB>(leds, NUM_PIXELS);

// check battery
battery();
}

//battery lvl function
void battery()
{
  float voltage = analogRead(A6);
  voltage *= 6.6;
  voltage /= 1024;
  Serial.println(voltage);
  if (voltage > 3.8){
    bat[0] = CRGB(7, 0, 7);
    FastLED.show();
  }
  else if (voltage > 3.73){
    bat[0] = CRGB(15, 7, 0);
    FastLED.show();
  }
  else {
    bat[0] = CRGB(15, 0, 0);
    FastLED.show();
  }
}

//sound function main hum
void playBase()
{
  File f;
  f = fatfs.open("idle.wav");
  playWav1.play(f);
}

//sound function for pitched up swing
void playH()
{
  File f;
  f = fatfs.open("high.wav");
  playWav2.play(f);
}

//sound function for pitched down swing
void playL()
{
  File f;
  f = fatfs.open("low.wav");
  playWav3.play(f);
}

//sound function for effects
void playFx(const char *filename)
{
  File f;
  f = fatfs.open(filename);
  playWav4.play(f);
}

//hit light animations
void burst(CRGB leds[]){
  fill_solid( &(leds[0]), NUM_PIXELS, CRGB( 63, 127, 127));
  FastLED.show();
  delay(100);
  fill_solid( &(leds[0]), NUM_PIXELS, CRGB(0, 45, 60));
  FastLED.show();
}

//on function
void on(CRGB leds[]){
  float vol;
  power = true;
  digitalWrite(POWER_PIN, HIGH);
  FastLED.clear();
  FastLED.show();
  mixer1.gain(3, .5);
  playFx("on.wav");
  playBase();
  for (int i=0; i < NUM_PIXELS; i = i + 4){
  leds[i]= CRGB(0, 15, 20);
  leds[i+1]= CRGB(0, 45, 60);
  leds[i+2]= CRGB(0, 45, 60);
  leds[i+3]= CRGB(0, 45, 60);
  vol = sq(i);
  mixer1.gain(0, vol/20736);  
  delay(20);
  FastLED.show();
  }
  while (playWav4.isPlaying()){}  
  while (swt){
    switch1.update();
    swt = !switch1.read();
  }
}

//off function
void off(CRGB leds[]){
  float vol;
  power = false;
  mixer1.gain(3, .5);
  playFx("off.wav");
  for (int i=NUM_PIXELS; i > 0; i = i - 4){
  leds[i]= CRGB(0, 0, 0);
  leds[i+1]= CRGB(0, 0, 0);
  leds[i+2]= CRGB(0, 0, 0);
  leds[i+3]= CRGB(0, 0, 0);
  if (playWav1.isPlaying() == false) {
    playBase();
  }
  vol = sq(i);
  mixer1.gain(0, vol/20736);
  delay(30);
  FastLED.show();
  }
  playWav1.stop();
  FastLED.clear();
  FastLED.show();
  while (swt){
      switch1.update();
      swt = !switch1.read();
    }
  while (playWav4.isPlaying()){}
  digitalWrite(POWER_PIN, LOW);
  battery();
}

//main code
void loop() {
switch1.update();
swt = !switch1.read();
if(swt){
  on(leds);
}
  while(power){
  switch1.update();
  swt = !switch1.read();
  if(swt){
    off(leds);
  }
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp; 
    lsm6ds33.getEvent(&accel, &gyro, &temp);
    t0 = t1;
    t1 = millis();
    dt = t1 - t0;
  if (playWav1.isPlaying() == false) {
    playBase();
  }
    v = sqrt(sq(gy)+sq(gz));
    angle = angle + v * dt/1000*180/3.14;
    if (angle >= 360) {
      angle = angle - 360;
    }
    a = sqrt(sq(ax)+sq(ay)+sq(az));
    
    vvol = v/7;
    if (angle < 180) {
      avol = (angle - 50)/50;
    }
    else if (angle >=180){
      avol = (angle - 180)/150;
    }
    Serial.print("\ta: "); Serial.print(a);
    Serial.print("\tv: "); Serial.print(v);
    Serial.print("\tangle: "); Serial.print(angle);
    Serial.print("\tavol: "); Serial.print(avol);
    Serial.print("\tvvol: "); Serial.print(vvol);
    Serial.print("\tvdt: "); Serial.println(dt);
    if (a > hit && v > swing && playWav4.isPlaying() == false){
      hitfx = random(0, 3);
      mixer1.gain(3, 1);
      if (hitfx == 0){
        playFx("hit1.wav");
      }
      if (hitfx == 1){
        playFx("hit2.wav");
      }
      if (hitfx == 2){
        playFx("hit3.wav");
      }
      burst(leds); 
    }
  if (v > swing){
    if (angle < start1) {
      if (playWav2.isPlaying() == false) {
        playH();
      }
      mixer1.gain(1, vvol);
      mixer1.gain(2, 0);
    }
    else if (angle >=start1 && angle <end1) {
      if (playWav2.isPlaying() == false) {
        playH();
      }
      if (playWav3.isPlaying() == false) {
        playL();
      }
      mixer1.gain(2, avol*vvol);  
      mixer1.gain(1, (1 - avol)*vvol);
    }
    else if (angle >= end1 && angle < start2) {
      if (playWav3.isPlaying() == false) {
        playL();
      }
      mixer1.gain(2, vvol);
      mixer1.gain(1, 0); 
    }
    else if (angle >= start2 && angle < end2) { 
      if (playWav2.isPlaying() == false) {
        playH();
      }
      if (playWav3.isPlaying() == false) {
        playL();
      }
      mixer1.gain(2, (1 - avol)*vvol);  
      mixer1.gain(1, avol*vvol);
    }
    else if (angle >= end2) {
      if (playWav2.isPlaying() == false) {
        playH();
      }
      mixer1.gain(1, vvol);      
      mixer1.gain(2, 0);
    }
    mixer1.gain(0, 1 - vvol);
    }
    else {
      playWav2.stop();
      playWav3.stop();
      angle = 0;
    }
  }
}
