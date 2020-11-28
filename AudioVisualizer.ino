
#define PIN 3 //LED strip output pin
#define LOG_OUT 1 // use the log output function
#define FHT_N 256 // set to 256 point fht

#define NUM 32 //number of LEDs

double avgMaxFreq;//sum of the strongest frequencies in each bin-space
int amp; //average amplitude of the last waveform sample



//-----Adjust this variable to change the sensitivity of the visualizer to volume--------------------------

double sensitivity = 1000000;

//----------------------------------------------------


#include <FHT.h>
#include <Adafruit_NeoPixel.h>


Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  
  strip.begin();
  strip.setBrightness(90); 
  Serial.begin(2000000);
   
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0b11100101; // set the adc to free running mode 
  ADMUX =  0b00000000;// use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0
  
}

void loop() {
  while(1) { // reduces jitter
    cli();  // UDRE interrupt slows this way down on arduino1.0

    //Save the old amplitude to average it with the new amplitude
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


    //average new amplitude with old one
    amp = .9*(float)amp + .1*(float)oldAmp;

     
    fht_window(); // window the data for better frequency response
    fht_reorder(); // reorder the data before doing the fht
    fht_run(); // process the data in the fht
    fht_mag_log(); // take the output of the fht
    
    sei();

    //function sets LED colors based on frequency, and brightness based on volume simultaneously
    simultaneous(amp);

    strip.show();
    
     
  }
}


//function handles displaying LED's based on frequency and volume content of signal 
 void simultaneous(double amplitude) {

  
  //Find the maximum frequency using FFT
  int maxFreq = 0;
  for(int t = FHT_N/32; t < FHT_N/2; t ++) {
      if (fht_log_out[t] > fht_log_out[maxFreq]) {
        maxFreq = t;
      }
    }

    //Set the color and brightness according to frequency and volume
    
    int color = Wheel((int)(maxFreq*10)%255);

    strip.setBrightness((amplitude*amplitude - 1000000)/sensitivity);
    
   //set all pixels to the same color
    for (int i = 0; i < NUM; i ++)
      strip.setPixelColor(i, color);  
 }


 
//this function takes in a value between 0 and 255, as well as an amplitude, and outputs a color
//hue depends on WheelPos, brightness depends on amp
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (amp < 0) {
    amp = 0;
  }
  if(WheelPos < 85) {
    return strip.Color((255 - WheelPos * 3), 0, WheelPos * 3, 0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, (255 - WheelPos * 3), 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, (255 - WheelPos * 3) * amp, 0);
}
