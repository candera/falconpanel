/*
  Copyright (c) 2015 Craig Andera

  Falconpanel - a set of functions for turning an Arduino into a
  DirectX game controller.

  Modify the components array below to match your setup.

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

/* Abstracts the concept of a DirectX button. There are 32 available
   on the Gamepad, numbered from 1-32. At some point, I might extend
   this to cover the hat.*/
class DxButton {
 private:
  int _num;

 public:
  DxButton(int num) {
    _num = num;
  }

  void press() {
    Gamepad.press(_num);
  }

  void release() {
    Gamepad.release(_num);
  }
};

/* These next few classes shouldn't be necessary, but unfortunately I
   had no luck getting function pointers to work. So back to OOP
   land. */
class DxAxisAdapter {
 public:
  virtual void report(float val) = 0;
};

short scale16(float val) {
  return short(val * 65536) - 32768;
};

byte scale8(float val) {
  return byte(val * 256) - 128;
};

class DxXAxisAdapter : public DxAxisAdapter {
  virtual void report(float val) { Gamepad.xAxis(scale16(val)); };
};

class DxYAxisAdapter : public DxAxisAdapter {
  virtual void report(float val) { Gamepad.yAxis(scale16(val)); };
};

class DxZAxisAdapter : public DxAxisAdapter {
  virtual void report(float val) { Gamepad.zAxis(scale8(val)); };
};

class DxXRotAxisAdapter : public DxAxisAdapter {
  virtual void report(float val) { Gamepad.rxAxis(scale16(val)); };
};

class DxYRotAxisAdapter : public DxAxisAdapter {
  virtual void report(float val) { Gamepad.ryAxis(scale16(val)); };
};

class DxZRotAxisAdapter : public DxAxisAdapter {
  virtual void report(float val) { Gamepad.rzAxis(scale8(val)); };
};

/* Abstracts the concept of a DirectX axis. Axis values are normalized
   to a floating point number in the range 0.0 to 1.0 (inclusive). */
class DxAxis {
 private:
  DxAxisAdapter* _adapter;

  DxAxis(DxAxisAdapter* adapter) {
    _adapter = adapter;
  }

 public:
  static DxAxis* X() { return new DxAxis(new DxXAxisAdapter()); }
  static DxAxis* Y() { return new DxAxis(new DxYAxisAdapter()); }
  static DxAxis* Z() { return new DxAxis(new DxZAxisAdapter()); }
  static DxAxis* XRotation() { return new DxAxis(new DxXRotAxisAdapter()); }
  static DxAxis* YRotation() { return new DxAxis(new DxYRotAxisAdapter()); }
  static DxAxis* ZRotation() { return new DxAxis(new DxZRotAxisAdapter()); }

  void report(float val) {
    float clamped = min(max(val, 0.0), 1.0);
    _adapter->report(clamped);
  }
};

/* An "interface" class representing a thing that can be initialized.
   Important for things like pins on the Arduino, but also components
   that manage them. */
class Stateful {
 public:
  virtual void setup() = 0;
};

/* An "interface" class represnting a thing that wants to be called
   periodically. Useful for components that send events like button
   presses. */
class Updateable {
 public:
  virtual void update() = 0;
};

/* A source of boolean input. Abstract, through: might be a digital
   input pin or something else, like a selection from a
   multiplexer. */
class DigitalInput : public Stateful {
 public:
  virtual bool read() = 0;
};

/* A place that can accept digital output. Abstract: might not be a
   digital output pin on the Arduino. */
class DigitalOutput : public Stateful {
 public:
  virtual void write(bool val) = 0;
};

/* A source of analog input in the range 0.0 to 1.0, inclusive.
   Abstract. */
class AnalogInput : public Stateful {
 public:
  virtual float read() = 0;
};

/* A representation of a physical component in our game controller.
   Includes things like switches and knobs, but also things like
   mulitplexers. */
class Component : public Stateful, public Updateable {
};

/* A pin on the Arduino that we want to use as a digital input,
   configured with a pullup resistor. */
class DigitalInputPullupPin : public DigitalInput, public Stateful {
 private:
  int _pin;

 public:
  DigitalInputPullupPin(int pin) {
    _pin = pin;
  }
  virtual bool read() {
    return digitalRead(_pin);
  }

  virtual void setup() {
    pinMode(_pin, INPUT_PULLUP);
  }
};

class DigitalOutputPin : public DigitalOutput, public Stateful {
 private:
  int _pin;

 public:
  DigitalOutputPin(int pin) {
    _pin = pin;
  }

  virtual void write(bool val) {
    digitalWrite(_pin, val ? HIGH : LOW);
  }

  virtual void setup() {
    pinMode(_pin, OUTPUT);
  }
};

/* A pin on the Arduino that we want to use as an analog input. */
class AnalogInputPin : public AnalogInput {
 private:
  int _pin;

 public:
  AnalogInputPin(int pin) {
    _pin = pin;
  }

  virtual void setup() {
  }

  virtual float read() {
    return float(analogRead(_pin)) / 1024.0;
  }
};

/* Represents a physical, on-off, momentary pushbutton. The
   corresponding DX button is pressed for as long as the physical one
   is. */
class PushButton : public Component {
 private:
  DigitalInput* _in;
  DxButton* _dxButton;

 public:
  PushButton(DigitalInput* in, DxButton* dxButton) {
    _in = in;
    _dxButton = dxButton;
  }

  virtual void setup() {
    _in->setup();
  }

  virtual void update() {
    if (_in->read()) {
      _dxButton->release();
    }
    else {
      _dxButton->press();
    }
  }
};

const int UP = 0;
const int MIDDLE = 1;
const int DOWN = 2;
const int NONE = -1;

/* A physical, non-momentary, three-position switch, which will
   generate momentary presses of three DirectX buttons. */
class OnOffOnSwitch : public Component {
 private:
  int _countdown;
  DigitalInput* _inUp;
  DigitalInput* _inDown;
  DxButton* _dxButtonUp;
  DxButton* _dxButtonMiddle;
  DxButton* _dxButtonDown;
  int _duration;
  int _last = NONE;

 public:
  OnOffOnSwitch(DigitalInput* inUp, DigitalInput* inDown,
                DxButton* dxButtonUp, DxButton* dxButtonMiddle, DxButton* dxButtonDown,
                int duration) {
    _inUp = inUp;
    _inDown = inDown;
    _dxButtonUp = dxButtonUp;
    _dxButtonMiddle = dxButtonMiddle;
    _dxButtonDown = dxButtonDown;
    _duration = duration;
  }

  virtual void setup() {
    _inUp->setup();
    _inDown->setup();
  }

  virtual void update() {
    int current;

    if (!_inUp->read()) {
      current = UP;
    }
    else if (!_inDown->read()) {
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
        _dxButtonUp->press();
        _dxButtonDown->release();
        _dxButtonMiddle->release();
        break;
      case DOWN:
        _dxButtonUp->release();
        _dxButtonDown->press();
        _dxButtonMiddle->release();
        break;
      case MIDDLE:
        _dxButtonUp->release();
        _dxButtonDown->release();
        _dxButtonMiddle->press();
        break;
      }
    }
    else {
      _dxButtonDown->release();
      _dxButtonUp->release();
      _dxButtonMiddle->release();
    }
  }
};

/* A physical, on-off, non-momentary switch that will generate
   momentary presses of two different DirectX buttons. */
class OnOffSwitch : public Component {
 private:
  int _countdown;
  DigitalInput* _in;
  DxButton* _dxButtonUp;
  DxButton* _dxButtonDown;
  int _duration;
  int _last = NONE;

 public:
  OnOffSwitch(DigitalInput* in, DxButton* dxButtonUp, DxButton* dxButtonDown, int duration) {
    _in = in;
    _dxButtonUp = dxButtonUp;
    _dxButtonDown = dxButtonDown;
    _duration = duration;
  }

  virtual void setup() {
    _in->setup();
  }

  virtual void update() {
    int current;

    if (!_in->read()) {
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
        _dxButtonUp->press();
        _dxButtonDown->release();
        break;
      case DOWN:
        _dxButtonUp->release();
        _dxButtonDown->press();
        break;
      }
    }
    else {
      _dxButtonDown->release();
      _dxButtonUp->release();
    }
  }
};

/* Adapts a simple potentiometer into a DX axis and two DirectX
   buttons. The buttons will be momentarily pressed as the
   potentiometer passes through a configurable threshold in the on and
   off directions. */
class SwitchingRotary : public Component {
 private:
  float _last;
  AnalogInput* _in;
  DxButton* _dxButtonOn;
  DxButton* _dxButtonOff;
  DxAxis* _dxAxis;
  int _countdown;
  int _duration;
  float _threshold;


 public:
  SwitchingRotary(AnalogInput* in,
                  DxAxis* dxAxis, DxButton* dxButtonOn, DxButton* dxButtonOff,
                  int duration, float threshold) {
    _in = in;
    _dxButtonOn = dxButtonOn;
    _dxButtonOff = dxButtonOff;
    _duration = duration;
    _last = -1;
    _threshold = threshold;
    _dxAxis = dxAxis;
  }

  virtual void setup() {
    _in->setup();
  }

  virtual void update() {
    float val = _in->read();

    LOG("Switching rotary read val: ");
    LOGLN(val);

    if ((val >= _threshold) && (_last < _threshold)) {
      _dxButtonOn->press();
      _dxButtonOff->release();
      _countdown = _duration;
    }
    else if ((val <= _threshold) && (_last > _threshold)) {
      _dxButtonOn->release();
      _dxButtonOff->press();
      _countdown = _duration;
    }

    if (val >= _threshold) {
      // Scale the reported value from 0.0 at the threshold to 1.0 at
      // the max.
      float scaled = (val - _threshold) / (1.0 - _threshold);
      _dxAxis->report(scaled);
    }
    else {
      _dxAxis->report(0.0);
    }

    if (_countdown > 0) {
      _countdown--;
    }
    else {
      _dxButtonOn->release();
      _dxButtonOff->release();
    }

    _last = val;
  }
};

/* Support for the 74LS151 3-to-8 mux */
class IC74LS151 : public Component {
 private:
  DigitalOutput* _dout0;
  DigitalOutput* _dout1;
  DigitalOutput* _dout2;
  DigitalInput* _din;

  /* Adapts a LS151 mux to another control by satisfying the DigitalInput contract. */
  class IC54LS151InputLine : public DigitalInput {
  private:
    bool _addr0;
    bool _addr1;
    bool _addr2;
    IC74LS151* _mux;

  public:
    IC54LS151InputLine(IC74LS151* mux, byte addr) {
      _mux = mux;
      _addr0 = bitRead(addr, 0) == 1;
      _addr1 = bitRead(addr, 1) == 1;
      _addr2 = bitRead(addr, 2) == 1;
    }

    virtual bool read() {
      return _mux->read(_addr0, _addr1, _addr2);
    }

    virtual void setup() {
    }
  };

  bool read(bool addr0, bool addr1, bool addr2) {
    _dout0->write(addr0 ? HIGH : LOW);
    _dout1->write(addr1 ? HIGH : LOW);
    _dout2->write(addr2 ? HIGH : LOW);
    return _din->read();
  }

 public:
  IC74LS151(DigitalOutput* dout0, DigitalOutput* dout1, DigitalOutput* dout2, DigitalInput* din) {
    _dout0 = dout0;
    _dout1 = dout1;
    _dout2 = dout2;
    _din = din;
  }

  virtual void setup() {
    _dout0->setup();
    _dout1->setup();
    _dout2->setup();
    _din->setup();
  }

  virtual void update() {
  }

  DigitalInput* input(byte addr) {
    return new IC54LS151InputLine(this, addr);
  }

};

// I've got a 74LS151 3-to-8 mux with its address pins connected to
// Arduino pins 2-4, and with its output pin connected to Arduino pin
// 5. We have to declare this outside the components array below
// because we reference it in there.
IC74LS151* mux1 = new IC74LS151(new DigitalOutputPin(2),
                                new DigitalOutputPin(3),
                                new DigitalOutputPin(4),
                                new DigitalInputPullupPin(5));

// Configure our particular setup. Change this to match what you have.
Component* components[] =   {
  // List the mux here so its setup gets called
  mux1,
  // Taxi/landing lights (on mux input 3)
  new OnOffSwitch(mux1->input(3),
                  new DxButton(1),
                  new DxButton(2),
                  3),
  // FACK
  new PushButton(new DigitalInputPullupPin(6), new DxButton(3)),
  // Master Caution
  new PushButton(new DigitalInputPullupPin(7), new DxButton(4)),
  // Laser Arm
  new OnOffSwitch(new DigitalInputPullupPin(8),
                  new DxButton(5),
                  new DxButton(6),
                  3),
  // Master Arm
  new OnOffOnSwitch(new DigitalInputPullupPin(9),
                    new DigitalInputPullupPin(10),
                    new DxButton(7),
                    new DxButton(8),
                    new DxButton(9),
                    3),
  // Parking Brake
  new OnOffSwitch(new DigitalInputPullupPin(11),
                  new DxButton(10),
                  new DxButton(11),
                  3),
  // HMCS
  new SwitchingRotary(new AnalogInputPin(0),
                      DxAxis::XRotation(),
                      new DxButton(12),
                      new DxButton(13),
                      3,
                      0.05),
  // A/R Door
  new OnOffSwitch(new DigitalInputPullupPin(12),
                  new DxButton(14),
                  new DxButton(15),
                  3)
};

const int componentCount = sizeof(components)/sizeof(Component*);

void setup() {
  pinMode(pinLed, OUTPUT);

  for (int i = 0; i < componentCount; ++i) {
    components[i]->setup();
  }

  // Sends a clean report to the host. This is important on any Arduino type.
  // Make sure all desired USB functions are activated in USBAPI.h!
  Gamepad.begin();

#if DO_LOG
  Serial.begin(9600);
#endif

  LOGLN("Setup complete");
}

void loop() {

  for (int i = 0; i < componentCount; ++i) {
    components[i]->update();
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
/*  */
