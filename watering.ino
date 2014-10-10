#include <LiquidCrystal.h>

// Notes:
// * The relay used here has an active on pin LOW.  This may require tweaking
//
//
//



LiquidCrystal lcd(7,8,9,10,11,12);

// Set to 0 to test the device.  Uses seconds instead of hours/minutes
int PRODUCTION = 1;

int RELAY_PIN  = 13;
int SWITCH_PIN = 6;
int BUTTON_PIN = 4;
int POT1_PIN   = 1;
int POT2_PIN   = 2;

enum States { STATE_STARTUP, STATE_SETUP, STATE_WAIT, STATE_WATER };

// Current state.   Set to startup on power up
States state = STATE_STARTUP;

int setup_switch,button,pot1,pot2;
char line1[16]; // Text to be displayed on 1st line
char line2[16]; // Text to be displayed on 2nd line

// Total number of hours between watering
int  wait_hours=0;
// Total number of minutes to have the water on
int  water_minutes=0;

// Number of milliseconds left to wait
long  wait_left=0;
// Number of milliseconds left to water
long  water_left=0;

// Used to check passage of time
unsigned last_millis=0;

// Called every loop to update the contents of the screen
void update_screen()
{
  static char lcd_line1[16];
  static char lcd_line2[16];
  if (strcmp(line1,lcd_line1)!=0 || 
      strcmp(line2,lcd_line2)!=0)
  {
    strcpy(lcd_line1,line1);
    strcpy(lcd_line2,line2);
    lcd.clear();
    lcd.print(lcd_line1);
    lcd.setCursor(0,1);
    lcd.print(lcd_line2);
  }      
}

// Print the progress of the current time dependent state.
void print_left(const char* msg, long value)
{
  const char* units="seconds";
  value=value/1000;
  String s=msg;
  s+=" ";
  if (value>3600)
  {
    value=value/3600;
    units="hours";
  }
  else
  if (value>60)
  {
    value=value/60;
    units="minutes";
  }
  s+=String(value);
  s.toCharArray(line1,16);
  strcpy(line2,units);
}

// Displays a simple text message
void error_message(const char* msg)
{
  line2[0]=0;
  if (strcmp(msg,line1)!=0)
    strcpy(line1,msg);
}

// Displays a simple text message
void error_message(const String& s)
{
  line2[0]=0;
  s.toCharArray(line1,16);
}

void reset_wait_left()
{
  // Set wait left counter, according to wait_hours
  if (PRODUCTION)
  {
    // Convert minutes to milliseconds
    water_left = long(water_minutes)*1000*60;
    // Convert hours to milliseconds and subtract the watering time to align wait periods
    wait_left  = long(wait_hours)*1000*3600 - water_left;
  }
  else
  {
    // Debugging.  Instead of hours and minutes, use just seconds.
    water_left = water_minutes*1000;
    wait_left  = wait_hours*1000 - water_left;
  }
}

void startup_state()
{
  if (button) error_message("Depress Button");
  else
  if (!setup_switch) error_message("Lower Setup Sw");
  else
    state=STATE_SETUP;
  digitalWrite(RELAY_PIN,HIGH);
}

// Exponential filter to eliminate analog read jitter
float filtered_h=0;
float filtered_m=0;

void setup_state()
{
  pot1=analogRead(POT1_PIN);
  pot2=analogRead(POT2_PIN);
  
  // Convert potentiometer value to hours using   H=1+(p1/66)^2
  float h=(pot1*0.015);
  h=1+h*h;
  filtered_h=0.99*filtered_h+0.01*h;
  wait_hours=int(filtered_h);
  
  // Convert potentiometer value to minutes using   M=1+(p2/125)^2
  float m=(pot2*0.008);
  m=1+m*m;
  filtered_m=0.99*filtered_m+0.01*m;
  water_minutes=int(filtered_m);
  
  // Build display strings
  String s("Every ");
  s+=String(wait_hours);
  s+=" hours";
  s.toCharArray(line1,16);
  s="Water ";
  s+=String(water_minutes);
  s+=" min";
  s.toCharArray(line2,16);
  
  // Check if leaving setup state
  if (!setup_switch) 
  {
    reset_wait_left();
    last_millis = millis();
    state=STATE_WAIT;
  }
}

void wait_state()
{
  if (setup_switch)
  {
    state=STATE_SETUP;
    return;
  }
  if (button)
    wait_left=5000;
  print_left("Waiting",wait_left);
  unsigned cur=millis();
  unsigned dt=(cur-last_millis);
  last_millis=cur;
  wait_left-=dt;
  if (wait_left<=0)
  {
    reset_wait_left();
    state=STATE_WATER;
    digitalWrite(RELAY_PIN,LOW);
  }
}

void water_state()
{
  if (setup_switch)
  {
    state=STATE_SETUP;
    digitalWrite(RELAY_PIN,HIGH);
    return;
  }
  if (button)
    water_left=5000;
  print_left("Watering",water_left);
  unsigned cur=millis();
  unsigned dt=(cur-last_millis);
  last_millis=cur;
  water_left-=dt;
  if (water_left<=0)
  {
    reset_wait_left();
    state=STATE_WAIT;
    digitalWrite(RELAY_PIN,HIGH);
  }
}

void setup() 
{
  lcd.begin(16, 2);
  line1[0]=0;
  line2[0]=0;
  pinMode(RELAY_PIN,OUTPUT);
  pinMode(SWITCH_PIN,INPUT_PULLUP);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  digitalWrite(RELAY_PIN,HIGH);
}

void loop() 
{
  setup_switch=(digitalRead(SWITCH_PIN)==0 ? 1 : 0);
  button=(digitalRead(BUTTON_PIN)==0 ? 1 : 0);
  if (state == STATE_STARTUP) startup_state();
  else
  if (state == STATE_SETUP) setup_state();
  else
  if (state == STATE_WAIT) wait_state();
  else 
  if (state == STATE_WATER) water_state();
  else
    error_message("Unknown State");
  update_screen();
}
