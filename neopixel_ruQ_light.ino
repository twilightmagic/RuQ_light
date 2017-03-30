/*
 * Rubics Cube simulation with NeoPixel LED chain
 */
#include <avr/pgmspace.h>

#define DO_MASK 0x10 //port mask bit 4 (Arduino Micro: portB.4 = D8, Arduino Uno/Nano: portB.4 = D12)
#define DO_PDIR DDRB //Direction register Port B
#define DO_PSTATE PORTB //Output state Register Port B
#define CHANNELS 3 //Number of color channels (3 for RGB LED like WS281x)
#define LEDS 54 //Number of LEDs in chain, base model 3x3x6=54

#define GREEN 0
#define RED 1
#define BLUE 2

#define TOP 0
#define BOTTOM 1
#define FRONT 2
#define BACK 3
#define LEFT 4
#define RIGHT 5


/* 
 *  LED assignment table, physical address (chain element - 1) vs. logical placemet on cube
 *  for illustration and numbering of elements see "ruQ_light LED mapping.xlsx" 
 */
const uint8_t pos_table[6][9] PROGMEM = {
  {53, 52, 51, 36, 37, 38, 35, 34, 33}, //top side
  {27, 28, 29, 44, 43, 42, 45, 46, 47},  //bottom side
  {20, 19, 18, 15, 16, 17,  2,  1,  0}, //front side (USB connector to the right)
  {26, 25, 24,  9, 10, 11,  8,  7,  6}, // back side
  {23, 22, 21, 12, 13, 14,  5,  4,  3}, //left side
  {48, 41, 30, 49, 40, 31, 50, 39, 32} // right side
};

/* 
 *  table for gamma correction or, more precisly, perceptive adoption.
 *  5 bit CIE Lightness to 8 bit PWM conversion 
*/
const uint8_t CIEL8[] PROGMEM = {
    0,   1,   2,   3,   4,   5,   7,   9,
   12,  15,  18,  22,  27,  32,  38,  44,
   51,  58,  67,  76,  86,  96, 108, 120,
  134, 148, 163, 180, 197, 216, 235, 255
};

/*
 * color definitions 
 */
const long def_col[6] = {
  0xD0B0A0, //white
  0xFF8004, //yellow
  0x0000FF, //blue
  0x00B000, //green
  0xC00000, //red
  0xA01200  //orange
};

uint8_t led_buffer[CHANNELS][LEDS];
float brightness = 0.5;


/****** F U N C T I O N S ********************************/


/*
 * Output one byte to WS281x LED chain
 * note: High pulse length is crucial, all interrupts disabled then
 */
void send_byte( byte b ){
  for ( int i = 0; i < 8; i++ ){
  noInterrupts();
    DO_PSTATE |= DO_MASK; //set DO high
    asm("nop\n");
    if ( b & 0x80 ){ //bit 7 high, 1-code (700ns high on bus)
      asm("nop\n nop\n nop\n nop\n nop\n nop\n");
    }
    DO_PSTATE &= ~DO_MASK; //set DO low
  interrupts();
    if ( i == 7) return; //omit unneccesary code, if byte is out
    if ( !( b & 0x80 )){ //bit 7 low, 0-code (ca.800ns low on bus)
      asm("nop\n nop\n nop\n nop\n nop\n");
    }
    b <<= 1;
  }
}


/*
 * use of send_byte() to transmit LED buffer contet to LED chain
 */
void data_out( void ){
  for ( int i=0; i<LEDS; i++ ){
    for ( int j=0; j<CHANNELS; j++ ){
      send_byte( led_buffer[j][i] );
    }
  }
  delayMicroseconds( 50 ); // reset chain (optional, not needed if update interval >> processing time + 50µs)
}

/*
 * adopt LED Brightness steps according to human perception
 * note: only 32 real steps; input values 0...3 result in 0
 */
uint8_t gamma( uint8_t inp ){
  return *(CIEL8) + int(inp+4) / 8;
}

/* 
 *  preparing led_buffer to contain cube initial state
 */
void cube_inital_state( void ){
  long color;
  for ( int side = TOP; side <= RIGHT; side++ ){
    color = def_col[side];
    for ( int segment = 0; segment < 9; segment++ ){
      led_buffer[RED][pgm_read_byte( *(pos_table) + side*9 + segment )] = gamma((( color & 0xFF0000 ) >> 16)* brightness );
      led_buffer[GREEN][pgm_read_byte( *(pos_table) + side*9 + segment )] = gamma((( color & 0xFF00 ) >> 8)* brightness );
      led_buffer[BLUE][pgm_read_byte( *(pos_table) + side*9 + segment )] = gamma(( color & 0xFF ) * brightness );
    }
  }
}

void setup() {
  //Serial.begin( 115200 ); //for debug purposes only
  DO_PDIR |= DO_MASK; //set DO output
  DO_PSTATE &= ~DO_MASK; //set DO low
  cube_inital_state();
  data_out();
}

void loop() {
//put your wild cube moving actions here! :)
}
