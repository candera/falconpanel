/*
Copyright (c) 2014 NicoHood
See the readme for credit to other people.

Gamepad example

Press a button and demonstrate Gamepad actions
*/

#define DO_LOG false

#if DO_LOG
  #define LOG(x) Serial.print(x);
  #define LOGLN(x) Serial.println(x);
#else
  #define LOG(x)
  #define LOGLN(x)
#endif

const int pinLed = LED_BUILTIN;
const int pinButton = 2;

class Control {
public:
  virtual void setup() = 0;
  virtual void update() = 0;
};

class PushButton : public Control {
private:
  int _pin;
  int _dxButton;
  
public:
  PushButton(int pin, int dxButton) {
    _pin = pin;
    _dxButton = dxButton;
  }
  
  virtual void setup() {
    pinMode(_pin, INPUT_PULLUP);
  }
  
  virtual void update() {
    LOG("Pin ");
    LOG(_pin);
    LOG(" on dxButton ");
    LOG(_dxButton);
    LOG(" is ");
    if (digitalRead(_pin)) {
      LOGLN("up");
      Gamepad.release(_dxButton);
    }
    else {
      LOGLN("down");
      Gamepad.press(_dxButton);
    }
  }
};

const int UP = 0;
const int MIDDLE = 1;
const int DOWN = 2;
const int NONE = -1;

class OnOffOnSwitch : public Control {
private:
  int _countdown;
  int _pinUp;
  int _pinDown;
  int _dxButtonUp;
  int _dxButtonMiddle;
  int _dxButtonDown;
  int _duration;
  int _last = NONE;
  
public:
  OnOffOnSwitch(int pinUp, int pinDown, int dxButtonUp, int dxButtonMiddle, int dxButtonDown, int duration) {
    _pinUp = pinUp;
    _pinDown = pinDown;
    _dxButtonUp = dxButtonUp;
    _dxButtonMiddle = dxButtonMiddle;
    _dxButtonDown = dxButtonDown;
    _duration = duration;
  }
  
  virtual void setup() {
    pinMode(_pinUp, INPUT_PULLUP);
    pinMode(_pinDown, INPUT_PULLUP);
  }
  
  virtual void update() {
    int current;
    
    if (!digitalRead(_pinUp)) {
      current = UP;
    }
    else if (!digitalRead(_pinDown)) {
      current = DOWN;
    }
    else {
      current = MIDDLE;
    }
    
    if (current != _last) {
      _countdown = _duration;
      _last = current;
    }
    
    if (_countdown > 0) {
      _countdown--;
      switch (current) {
        case UP:
          Gamepad.press(_dxButtonUp);
          Gamepad.release(_dxButtonDown);
          Gamepad.release(_dxButtonMiddle);
          break;
        case DOWN:
          Gamepad.release(_dxButtonUp);
          Gamepad.press(_dxButtonDown);
          Gamepad.release(_dxButtonMiddle);
          break;
        case MIDDLE:
          Gamepad.release(_dxButtonUp);
          Gamepad.release(_dxButtonDown);
          Gamepad.press(_dxButtonMiddle);
          break;
      }
    }
    else {
      Gamepad.release(_dxButtonDown);
      Gamepad.release(_dxButtonUp);
      Gamepad.release(_dxButtonMiddle);
    }
  }
};

class OnOffSwitch : public Control {
private:
  int _countdown;
  int _pin;
  int _dxButtonUp;
  int _dxButtonDown;
  int _duration;
  int _last = NONE;
  
public:
  OnOffSwitch(int pin, int dxButtonUp, int dxButtonDown, int duration) {
    _pin = pin;
    _dxButtonUp = dxButtonUp;
    _dxButtonDown = dxButtonDown;
    _duration = duration;
  }
  
  virtual void setup() {
    pinMode(_pin, INPUT_PULLUP);
  }
  
  virtual void update() {
    int current;
    
    if (!digitalRead(_pin)) {
      current = UP;
    }
    else {
      current = DOWN;
    }
    
    if (current != _last) {
      _countdown = _duration;
      _last = current;
    }
    
    if (_countdown > 0) {
      _countdown--;
      switch (current) {
        case UP:
          Gamepad.press(_dxButtonUp);
          Gamepad.release(_dxButtonDown);
          break;
        case DOWN:
          Gamepad.release(_dxButtonUp);
          Gamepad.press(_dxButtonDown);
          break;
      }
    }
    else {
      Gamepad.release(_dxButtonDown);
      Gamepad.release(_dxButtonUp);
    }
  }
};

/* TODO
class SwitchingRotary : public Control {
}
*/

Control* controls[] = 
{ //new PushButton(2, 1),
  //new PushButton(3, 2)
  new OnOffSwitch(3, 1, 2, 3)
};

const int controlCount = sizeof(controls)/sizeof(Control*);

void setup() {
  pinMode(pinLed, OUTPUT);
  pinMode(pinButton, INPUT_PULLUP);
 
  for (int i = 0; i < controlCount; ++i) {
    controls[i]->setup();
  }

  // Sends a clean report to the host. This is important on any Arduino type.
  // Make sure all desired USB functions are activated in USBAPI.h!
  Gamepad.begin();
  
#if DO_LOG
  Serial.begin(9600);
#endif
}

void loop() {

  for (int i = 0; i < controlCount; ++i) {
    controls[i]->update();
  }

  // functions before only set the values
  // this writes the report to the host
  Gamepad.write();

  // simple debounce
  delay(150);
}

/*
Prototypes:

void begin(void);
void end(void);
void write(void);
void press(uint8_t b);
void release(uint8_t b);
void releaseAll(void);
void buttons(uint32_t b);
void xAxis(int16_t a);
void yAxis(int16_t a);
void rxAxis(int16_t a);
void ryAxis(int16_t a);
void zAxis(int8_t a);
void rzAxis(int8_t a);
void dPad1(int8_t d);
void dPad2(int8_t d);

Definitions:
GAMEPAD_DPAD_CENTERED 0
GAMEPAD_DPAD_UP 1
GAMEPAD_DPAD_UP_RIGHT 2
GAMEPAD_DPAD_RIGHT 3
GAMEPAD_DPAD_DOWN_RIGHT 4
GAMEPAD_DPAD_DOWN 5
GAMEPAD_DPAD_DOWN_LEFT 6
GAMEPAD_DPAD_LEFT 7
GAMEPAD_DPAD_UP_LEFT 8
*/
