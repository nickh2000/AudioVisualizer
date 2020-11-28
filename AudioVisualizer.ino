//music input should be biased by 2.5 V and fed into analogPin 0
#define PIN 3 //LED strip output pin
#define LOG_OUT 1 // use the log output function
#define FHT_N 256 // set to 256 point fht
#define SMOOTH 1 //dictates the smoothness of the fourier output ie how much averaging is done between measurements
#define NUM 32 //number of LEDs
double pix[NUM]; //pixel array for each of the leds
int maxFreq; //value of the strongest frequency
double avgMaxFreq;//sum of the strongest frequencies in each bin-space
int amp; //amplitude of the waveform
double sensitivity = 100000000;
#include <FHT.h>
#include <Adafruit_NeoPixel.h>



Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  
  
  strip.begin();
  strip.setBrightness(90);
  //strip.show();
 Serial.begin(2000000);
   
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0b11100101; // set the adc to free running mode 
  ADMUX =  0b00000000;// use adc0
  DIDR0 = 0x01;
  // turn off the digital input for adc0
  
}

void loop() {
  while(1) { // reduces jitter
    cli();  // UDRE interrupt slows this way down on arduino1.0
    int oldAmp = amp;
    amp = 0;
    for (int i = 0 ; i < FHT_N ; i++) {
      
      while(!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fht_input[i] = k; //put real data into bin      
      amp += abs(k)/4; //take amplitude of waveform
      Serial.println(k);
      
    }

    amp = .9*(float)amp + .1*(float)oldAmp;

     
    fht_window(); // window the data for better frequency response
    fht_reorder(); // reorder the data before doing the fht
    fht_run(); // process the data in the fht
    fht_mag_log(); // take the output of the fht
    
    sei();

    //find the strongest frequency in the spectrum
    maxFreq = 0;


    //***PUT VISUALIZING FUNCTION HERE***
    //------------------------------------------------

    
    simultaneous(amp);


    //------------------------------------------------
    strip.show();
    
    avgMaxFreq = 0;    
  }
}




void levels(int amplitude) {
    int freq;
    uint8_t maxFreq = FHT_N/32;
    for(int t = maxFreq; t < FHT_N/2; t ++) {
      if (fht_log_out[t] > fht_log_out[maxFreq]) {
        maxFreq = t;
      }
    }

    freq = maxFreq;


    uint8_t upFreq = 0;
    for(int t = upFreq; t < FHT_N/2; t ++) {
      if (fht_log_out[t] > fht_log_out[upFreq]) {
        upFreq = t;
      }
    }
  
  int vol = (amplitude * amplitude > 70000000) ? amplitude/500.0 : 0;
  int color = Wheel((int)(freq*10)%255, vol/8);
  for(int i=20; i < 28; i ++) {
    if ( i < vol)
      strip.setPixelColor(i, color);
    else if (vol < 20) strip.setPixelColor(i, dim(strip.getPixelColor(i)));
    else strip.setPixelColor(i, 0);
  }


  vol = amplitude*amplitude > 4e7  ? (fht_log_out[(int)upFreq]*amplitude)  / 8000: 0;
  color = Wheel((int)(upFreq* 50)%255, .4);
  
  for(int i = 0; i < 8; i ++){
    if (i < vol) strip.setPixelColor(11 - i, color);
    else strip.setPixelColor(11 - i, 0);
  }
}
 void simultaneous(double amplitude) {
  
  for(int t = FHT_N/32; t < FHT_N/2; t ++) {
      if (fht_log_out[t] > fht_log_out[maxFreq]) {
        maxFreq = t;
      }
    }
    
    int color = Wheel((int)(maxFreq*10)%255, (amplitude*amplitude - 1000000)/sensitivity);
   
    for (int i = 0; i < NUM; i ++)
      strip.setPixelColor(i, color);  
 }


 
//this function takes in a value between 0 and 255, as well as an amplitude, and outputs a color
//hue depends on WheelPos, brightness depends on amp
uint32_t Wheel(byte WheelPos, double amp) {
  WheelPos = 255 - WheelPos;
  if (amp < 0) {
    amp = 0;
  }
  if(WheelPos < 85) {
    return strip.Color((255 - WheelPos * 3)*amp, 0, WheelPos * 3 * amp, 0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3 * amp, (255 - WheelPos * 3) * amp, 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3 * amp, (255 - WheelPos * 3) * amp, 0);
}


uint32_t dim(uint32_t color) {
    double rate = 1.1;
    uint32_t  r = (color >> 16) & 0xFF;
    r /= rate;
    uint32_t  g = ((color >>8) & 0xFF);
    g /= rate;
    uint32_t  b = (color & 0xFF);
    b /= rate;

    


    
    return (r << 16) | (g << 8) | b;
    
}
