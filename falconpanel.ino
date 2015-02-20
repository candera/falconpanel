/*
  Copyright (c) 2015 Craig Andera

  Falconpanel - a set of functions for turning an Arduino into a
  DirectX game controller.

  Modify the components array below to match your setup.

*/

const int pinLed = LED_BUILTIN;

#include "components.h"

// I've got a 74LS151 3-to-8 mux with its address pins connected to
// Arduino pins 2-4, and with its output pin connected to Arduino pin
// 5. We have to declare this outside the components array below
// because we reference it in there.
IC74LS151* mux1 = new IC74LS151(new DigitalOutputPin(2),
                                new DigitalOutputPin(3),
                                new DigitalOutputPin(4),
                                new DigitalInputPullupPin(5));

// Keep track of the button number so I don't have to keep looking at
// what I used.
int dxButton = 1;

// Configure our particular setup. Change this to match what you have.
Component* components[] =   {
  // List the mux here so its setup gets called
  mux1,
  // FACK
  new PushButton(mux1->input(0), new DxButton(dxButton++)),
  // Master Caution
  new PushButton(mux1->input(1), new DxButton(dxButton++)),
  // Laser Arm
  new OnOffSwitch(mux1->input(2),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // Master Arm
  new OnOffOnSwitch(mux1->input(3),
                    mux1->input(4),
                    new MomentaryButton(new DxButton(dxButton++)),
                    new MomentaryButton(new DxButton(dxButton++)),
                    new MomentaryButton(new DxButton(dxButton++))),
  // Emergency Stores Jettison
  new PushButton(mux1->input(5), new DxButton(dxButton++)),
  // Parking Brake
  new OnOffSwitch(new DigitalInputPullupPin(6),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // Landing Gear
  new OnOffSwitch(mux1->input(6),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // Taxi Lights
  new OnOffSwitch(mux1->input(7),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // Stores config
  new OnOffSwitch(new DigitalInputPullupPin(7),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // HMCS
  new SwitchingRotary(new AnalogInputPin(0),
                      DxAxis::XRotation(),
                      new MomentaryButton(new DxButton(dxButton++)),
                      new MomentaryButton(new DxButton(dxButton++)),
                      0.05),
  // RWR
  new OnOffSwitch(new DigitalInputPullupPin(8),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // Chaff
  new OnOffSwitch(new DigitalInputPullupPin(9),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // Flares
  new OnOffSwitch(new DigitalInputPullupPin(10),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // Jammer
  new OnOffSwitch(new DigitalInputPullupPin(11),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // A/R Door
  new OnOffSwitch(new DigitalInputPullupPin(12),
                  new MomentaryButton(new DxButton(dxButton++)),
                  new MomentaryButton(new DxButton(dxButton++))),
  // RWR Power
  new PushButton(new DigitalInputPullupPin(13), new DxButton(dxButton++))
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
