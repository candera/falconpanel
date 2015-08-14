#ifndef _COMPONENTS_H
#define _COMPONENTS_H

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

/* Abstracts the concept of a DirectX button. */
class Button : public Updateable {
 public:
  virtual void press() = 0;
  virtual void release() = 0;
};

/* I tried making this a method of the Button class, but I got a weird
   error. Don't hate the player, hate the game. */
void setButton(Button* button, bool state) {
  if (state) {
    button->press();
  }
  else {
    button->release();
  }
}

/* An actual button on the Gamepad. There are 32 available on the
   Gamepad, numbered from 1-32. At some point, I might extend this to
   cover the hat.*/
class DxButton : public Button {
 private:
  int _num;

 public:
  DxButton(int num) {
    _num = num;
  }

  virtual void press() {
    Gamepad.press(_num);
  }

  virtual void release() {
    Gamepad.release(_num);
  }

  virtual void update() { }
};

/* Adapts a DirectX button to be one that will be pressed momentarily.
   That is, when it is pressed, after `duration` updates, it will be
   released even without an explicit call to release. */
class MomentaryButton : public Button {
 private:
  Button* _inner;
  int _duration;
  int _countdown;

 public:
  MomentaryButton(Button* inner, int duration = 3) {
    _inner = inner;
    _duration = duration;
  }

  virtual void press() {
    _inner->press();
    _countdown = _duration;
  }

  virtual void release() {
    _inner->release();
    _countdown = 0;
  }

  virtual void update() {
    if (_countdown > 0) {
      --_countdown;
      if (_countdown == 0) {
        _inner->release();
      }
    }
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
   Includes things like swpitches and knobs, but also things like
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
   corresponding DX button is pressed and released when the physical
   one is. */
class PushButton : public Component {
 private:
  DigitalInput* _in;
  Button* _button;

 public:
  PushButton(DigitalInput* in, Button* button) {
    _in = in;
    _button = button;
  }

  virtual void setup() {
    _in->setup();
  }

  virtual void update() {
    _button->update();
    if (_in->read()) {
      _button->release();
    }
    else {
      _button->press();
    }
  }
};

const int UP = 0;
const int MIDDLE = 1;
const int DOWN = 2;
const int NONE = -1;

/* A physical, non-momentary, three-position switch, which will
   generate presses of three buttons corresponding to each
   position. */
class OnOffOnSwitch : public Component {
 private:
  DigitalInput* _inUp;
  DigitalInput* _inDown;
  Button* _buttonUp;
  Button* _buttonMiddle;
  Button* _buttonDown;
  int _last = NONE;

 public:
  OnOffOnSwitch(DigitalInput* inUp, DigitalInput* inDown,
                Button* buttonUp, Button* buttonMiddle, Button* buttonDown) {
    _inUp = inUp;
    _inDown = inDown;
    _buttonUp = buttonUp;
    _buttonMiddle = buttonMiddle;
    _buttonDown = buttonDown;
  }

  virtual void setup() {
    _inUp->setup();
    _inDown->setup();
  }

  virtual void update() {
    int current;
    _buttonUp->update();
    _buttonMiddle->update();
    _buttonDown->update();

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
      setButton(_buttonUp, current == UP);
      setButton(_buttonMiddle, current == MIDDLE);
      setButton(_buttonDown, current == DOWN);
      _last = current;
    }
  }
};

/* A physical, on-off, non-momentary switch that will generate presses
   of two different buttons. */
class OnOffSwitch : public Component {
 private:
  DigitalInput* _in;
  Button* _buttonUp;
  Button* _buttonDown;
  int _last = NONE;

 public:
  OnOffSwitch(DigitalInput* in, Button* buttonUp, Button* buttonDown) {
    _in = in;
    _buttonUp = buttonUp;
    _buttonDown = buttonDown;
  }

  virtual void setup() {
    _in->setup();
  }

  virtual void update() {
    int current;

    _buttonUp->update();
    _buttonDown->update();

    if (!_in->read()) {
      current = UP;
    }
    else {
      current = DOWN;
    }

    if (current != _last) {
      setButton(_buttonUp, current == UP);
      setButton(_buttonDown, current == DOWN);
      _last = current;
    }
  }
};

/* Adapts a simple potentiometer into a DX axis and two DirectX
   buttons. The buttons will be pressed as the potentiometer passes
   through a configurable threshold in the on and off directions. */
class SwitchingRotary : public Component {
 private:
  float _last;
  AnalogInput* _in;
  Button* _buttonOn;
  Button* _buttonOff;
  DxAxis* _dxAxis;
  float _threshold;

 public:
  SwitchingRotary(AnalogInput* in,
                  DxAxis* dxAxis, Button* buttonOn, Button* buttonOff,
                  float threshold) {
    _in = in;
    _buttonOn = buttonOn;
    _buttonOff = buttonOff;
    _last = -1;
    _threshold = threshold;
    _dxAxis = dxAxis;
  }

  virtual void setup() {
    _in->setup();
  }

  virtual void update() {
    _buttonOn->update();
    _buttonOff->update();
    float val = _in->read();

    if ((val >= _threshold) && (_last < _threshold)) {
      _buttonOn->press();
      _buttonOff->release();
    }
    else if ((val <= _threshold) && (_last > _threshold)) {
      _buttonOn->release();
      _buttonOff->press();
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

    _last = val;
  }
};

/* Adapts a 360-degree potentiometer into two DX buttons that will
 * fire as the knob is turned (one will pulse when turned clockwise,
 * one for counterclockwise) */
class PulseRotary : public Component {
 private:
  float _last;
  float _nextUpLow1;
  float _nextUpHigh1;
  float _nextDownLow1;
  float _nextDownHigh1;
  float _nextUpLow2;
  float _nextUpHigh2;
  float _nextDownLow2;
  float _nextDownHigh2;
  AnalogInput* _in;
  Button* _buttonUp;
  Button* _buttonDown;
  float _stepSize;
  int _pendingUp;
  int _pendingDown;
  bool _pressed;

 public:
  PulseRotary(AnalogInput* in,
              Button* buttonUp,
              Button* buttonDown,
              int divisions) {
    _in = in;
    _buttonUp = buttonUp;
    _buttonDown = buttonDown;
    _last = 0;
    _stepSize = 1.0 / divisions;
    updateThresholds();

    _pendingUp = 0;
    _pendingDown = 0;
    _pressed = false;
  }

  virtual void setup() {
    _in->setup();
  }

  bool between(float val, float low, float high) {
    return (val >= low) && (val <= high);
  }

  void updateThresholds() {
    float opposite = _last + 0.5;

    if (opposite >= 1.0) {
      opposite -= 1.0;
    }

    // _last just above zero
    if (between(_last, 0, _stepSize)) {
      _nextUpLow1 = _last + _stepSize;
      _nextUpHigh1 = opposite;
      _nextUpLow2 = -1.0;
      _nextUpHigh2 = -1.0;

      _nextDownLow1 = opposite;
      _nextDownHigh1 = _last - _stepSize + 1.0;
      _nextDownLow2 = -1.0;
      _nextDownHigh2 = -1.0;
    }
    // _last just below one
    else if (between(_last, 1.0 - _stepSize, 1.0)) {
      _nextUpLow1 = _last + _stepSize - 1.0;
      _nextUpHigh1 = opposite;
      _nextUpLow2 = -1.0;
      _nextUpHigh2 = -1.0;

      _nextDownLow1 = opposite;
      _nextDownHigh1 = _last - _stepSize;
      _nextDownLow2 = -1.0;
      _nextDownHigh2 = -1.0;
    }
    // _last just before middle
    else if (between(_last, 0.5 - _stepSize, 0.5)) {
      _nextUpLow1 = _last + _stepSize;
      _nextUpHigh1 = opposite;
      _nextUpLow2 = -1.0;
      _nextUpHigh2 = -1.0;

      _nextDownLow1 = 0.0;
      _nextDownHigh1 = _last - _stepSize;
      _nextDownLow2 = opposite;
      _nextDownHigh2 = 1.0;
    }
    // _last just after middle
    else if (between(_last, 0.5, 0.5 + _stepSize)) {
      _nextUpLow1 = _last + _stepSize;
      _nextUpHigh1 = 1.0;
      _nextUpLow2 = 0.0;
      _nextUpHigh2 = opposite;

      _nextDownLow1 = opposite;
      _nextDownHigh1 = _last - _stepSize;
      _nextDownLow2 = -1.0;
      _nextDownHigh2 = -1.0;
    }
    // _last in first half
    else if (between(_last, _stepSize, 0.5)) {
      _nextUpLow1 = _last + _stepSize;
      _nextUpHigh1 = opposite;
      _nextUpLow2 = -1.0;
      _nextUpHigh2 = -1.0;

      _nextDownLow1 = 0;
      _nextDownHigh1 = _last - _stepSize;
      _nextDownLow2 = opposite;
      _nextDownHigh2 = 1.0;
    }
    // _last in second half
    else {
      _nextUpLow1 = _last + _stepSize;
      _nextUpHigh1 = 1.0;
      _nextUpLow2 = 0.0;
      _nextUpHigh2 = opposite;

      _nextDownLow1 = opposite;
      _nextDownHigh1 = _last - _stepSize;
      _nextDownLow2 = -1.0;
      _nextDownHigh2 = -1.0;
    }
  }

  virtual void update() {
    _buttonUp->update();
    _buttonDown->update();

    float val = _in->read();

    if (between(val, _nextDownLow1, _nextDownHigh1) ||
        between(val, _nextDownLow2, _nextDownHigh2)) {
      _pendingUp = 0;
      _pendingDown++;
      _buttonUp->release();
      _last = val;
      updateThresholds();
    }
    else if (between(val, _nextUpLow1, _nextUpHigh1) ||
             between(val, _nextUpLow2, _nextUpHigh2)) {
      _pendingUp++;
      _pendingDown = 0;
      _buttonDown->release();
      _last = val;
      updateThresholds();
    }

    if (_pendingUp > 0) {
      if (_pressed) {
        _buttonUp->release();
        _pressed = false;
        _pendingUp--;
      }
      else {
        _buttonUp->press();
        _pressed = true;
      }
    }
    else if (_pendingDown > 0) {
      if (_pressed) {
        _buttonDown->release();
        _pressed = false;
        _pendingDown--;
      }
      else {
        _buttonDown->press();
        _pressed = true;
      }
    }
    else {
      _pressed = false;
    }
  }
};

enum Edge : byte {
  None, Rising, Falling
};

/* Adapts a rotary encoder into two DirectX buttons - one for each
 * direction of rotation */
class RotaryEncoder : public Component {
 private:
  DigitalInput* _in1;
  DigitalInput* _in2;
  Button* _buttonForward;
  Button* _buttonBackward;
  int _pendingForward;
  int _pendingBackward;
  bool _pressed;
  bool _last1;
  bool _last2;
  int _lastEvent;
  int _penultimateEvent;
  int _queueLimit;

 public:
  RotaryEncoder(DigitalInput* in1, DigitalInput* in2,
                Button* buttonForward, Button* buttonBackward,
                int queueLimit = 2) {
    _in1 = in1;
    _in2 = in2;
    _buttonForward = buttonForward;
    _buttonBackward = buttonBackward;
    _pendingForward = 0;
    _pendingBackward = 0;
    _pressed = false;
    _last1 = false;
    _last2 = false;
    _lastEvent = 0;
    _penultimateEvent = 0;
    _queueLimit = queueLimit;
  }

  virtual void setup() {
    _in1->setup();
    _in2->setup();
  }

  virtual void update() {
    bool val1 = _in1->read();
    bool val2 = _in2->read();

    if (val1 && !_last1) {
      _penultimateEvent = _lastEvent;
      _lastEvent = 1;
    }
    else if (!val1 && _last1) {
      _penultimateEvent = _lastEvent;
      _lastEvent = -1;
    }

    if (val2 && !_last2) {
      _penultimateEvent = _lastEvent;
      _lastEvent = 2;
    }
    else if (!val2 && _last2) {
      _penultimateEvent = _lastEvent;
      _lastEvent = -2;
    }

    if (_lastEvent == -2 && _penultimateEvent == -1) {
      if (_pendingForward < _queueLimit) {
        _pendingForward++;
      }
      _lastEvent = 0;
      _penultimateEvent = 0;
    }
    else if (_lastEvent == -1 && _penultimateEvent == -2) {
      if (_pendingBackward < _queueLimit) {
        _pendingBackward++;
      }
      _lastEvent = 0;
      _penultimateEvent = 0;
    }

    _last1 = val1;
    _last2 = val2;
      
    if (_pendingForward > 0) {
      if (_pressed) {
        _buttonForward->release();
        _pressed = false;
        _pendingForward--;
      }
      else {
        _buttonForward->press();
        _pressed = true;
      }
    }
    else if (_pendingBackward > 0) {
      if (_pressed) {
        _buttonBackward->release();
        _pressed = false;
        _pendingBackward--;
      }
      else {
        _buttonBackward->press();
        _pressed = true;
      }
    }
    else {
      _pressed = false;
    }
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
#endif
