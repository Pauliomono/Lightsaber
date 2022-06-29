#include <Adafruit_LSM6DS33.h>
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
int pixel;
int state;
bool swt;
bool power = false;

//lights declarations
#define NUM_PIXELS 144
#define POWER_PIN 10
#define NEOPIXEL_PIN 5
#define slow 3
#define medium 6
#define fast 11
CRGB leds[NUM_PIXELS];

//audio declarations
AudioPlayFSWav           playWav1;
AudioOutputAnalogStereo  audioOutput;    // Dual DACs
AudioConnection          patchCord1(playWav1, 1, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 0, audioOutput, 1);

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
  Serial.begin(115200);
  Serial.println("Starting LSM6DS33...");
  if (!lsm6ds33.begin_I2C()) { 
    Serial.println("Couldnt start LSM6DS33");
    while (1);
  }
lsm6ds33.setAccelRange(LSM6DS_ACCEL_RANGE_4_G);
lsm6ds33.setGyroRange(LSM6DS_GYRO_RANGE_1000_DPS);

lsm6ds33.configInt1(false, false, true); // accelerometer DRDY on INT1
lsm6ds33.configInt2(false, true, false); // gyro DRDY on INT2

//audio initialize
 AudioMemory(8);

//enable wing
pinMode(POWER_PIN, OUTPUT);

//initialize lights
FastLED.addLeds<WS2812B, NEOPIXEL_PIN, GRB>(leds, NUM_PIXELS);
}

//sound fx function
void playFile(const char *filename){
  File f;
  f = fatfs.open(filename);
  playWav1.play(f);
}

//LED animations
void burst(CRGB leds[]){
  for (int i=0; i < NUM_PIXELS; i++){
  leds[i] *=2;
  }
  FastLED.show();
  

  delay(100);
  for (int i=0; i < NUM_PIXELS; i++){
  leds[i] /=2;
  }
  FastLED.show();

  
}

//on function
void on(CRGB leds[]){
  power = true;
  
  digitalWrite(POWER_PIN, HIGH);
  FastLED.clear();
  FastLED.show();
  playFile("on.wav");
  for (int i=0; i < NUM_PIXELS; i++){
  leds[i].red   = 0;
  leds[i].green = 15;
  leds[i].blue  = 20;
  delay(1);
  FastLED.show();
  }
  while (swt){
      switch1.update();
      swt = !switch1.read();
    }
}

//off function
void off(CRGB leds[]){
  power = false;
  playFile("off.wav");
  for (int i=NUM_PIXELS; i > 0; i--){
  leds[i].red   = 0;
  leds[i].green = 0;
  leds[i].blue  = 0;
  delay(1);
  FastLED.show();
  }
  FastLED.clear();
  FastLED.show();
  while (swt){
      switch1.update();
      swt = !switch1.read();
    }
  digitalWrite(POWER_PIN, LOW);
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
    
    if (playWav1.isPlaying() == false) {
      state = 0;
      playFile("idle.wav");
    }
    v = sqrt(sq(gy)+sq(gz));
    a = sqrt(sq(ax)+sq(ay)+sq(az));
    Serial.print("\tvx: "); Serial.print(gx);
    Serial.print("\tvy: "); Serial.print(gy);
    Serial.print("\tvz: "); Serial.print(gz);
    Serial.print("\tv: "); Serial.print(v);
    Serial.print("\tax: "); Serial.print(ax);
    Serial.print("\tay: "); Serial.print(ay);
    Serial.print("\taz: "); Serial.print(az);
    Serial.print("\ta: "); Serial.print(a);
    Serial.print("\t\tstate: ");
    Serial.println(state);
    if (v > slow && v < medium && state == 0){
      state = 1;
      playFile("slow.wav");
    }
    if (v > medium && v < fast && state <= 1){
      state = 2;
      playFile("medium.wav");
    }
    if (v > fast && state <= 2){
      state = 3;
      playFile("fast.wav");
      burst(leds);
    }
  }
}
