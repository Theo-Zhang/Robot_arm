#include "defs.h"
#include "sine.h"
#include "pid.h"

// this is intended for running on an Arduino Mega.

void tick(); // run this every ms.
Every e(1000, tick);

SineGen sine_j456;
SineGen sine_j3;
SineGen sine_j2;
SineGen sine_j1;

AD pot(A0); // pot input (optional).

// motor controllers.
MC j6(A1, 2, 3);
MC j5(A2, 4, 5);
MC j4(A3, 6, 7);

MC j3(A4, 8, 9);
MC j2(A5, 10, 11);
MC j1(A6, 44, 45);

MC* j[6] = {&j1, &j2, &j3, &j4, &j5, &j6};

#define jmap(x) rangemap(0,6,[](int i){j[i]->x;});

void setup() {
  Serial.begin(9600);

  sine_j3.scale = -1;
  sine_j3.bias = -350;
  sine_j3.nsplit = 16;

  sine_j2.scale = -2;
  sine_j2.bias = 0;
  sine_j2.nsplit = 16;
  sine_j2.cntr = 256 * 4;

  sine_j1.scale = -2;

  sine_j1.bias = 320 - 512;
  sine_j1.nsplit = 16;
  sine_j1.cntr = 256 * 2;

  sine_j456.nsplit = 8;

  jmap(init());

  j5.pidc.kp = 3;
  j5.pidc.ki = -7;
  j6.pidc.kp = 3;

  j3.pidc.kp = 6;
  j2.pidc.kp = 5;
}

void loop() {
  e.update();
}

int sine_wave = 0;
int cntr = 0;

PID pidc;
LPF lpf_sumerr(7, 3);
LPF lpf_sumerr2(7, 3);

void command_callback(uc whichaxis, int value) {
  Serial.print("cmd");
  Serial.print((int)whichaxis);
  Serial.print(" ");
  Serial.println(value);

  j[whichaxis - 1]->target = value;
}

SerialCommandHandler sch(command_callback);

void tick() {
  cntr = (cntr + 1) % 1000;

  /* command(target) */
# define SINE
# ifdef SINE
  int target_456 = sine_j456.next(); // sinewave command
  int target_3 = sine_j3.next();
  int target_2 = sine_j2.next();
  int target_1 = sine_j1.next();
# else
  int target = pot.read(); // potiontiometer command
# endif

  jmap(update());

  sch.update();
#define jp j3

  if (cntr % 200 == 0) { // debug info
    //    Serial.print("cmd");
    //    Serial.print(jp.target);
    //
    //    Serial.print(" mea");
    //    Serial.print(jp.measured);
    //
    //    Serial.print(" err");
    //    Serial.print(jp.pidc.lasterr);
    //
    //    Serial.print(" out");
    //    Serial.print(jp.out);
    //
    //    Serial.print(" dir");
    //    Serial.print(jp.dir);
    //
    //    Serial.print(" p");
    //    Serial.print(jp.pidc.pout);
    //
    //    Serial.print(" i");
    //    Serial.print(jp.pidc.iout);
    //
    //    Serial.print(" d");
    //    Serial.print(jp.pidc.dout);
    //
    //    Serial.print(" se");
    //    Serial.print(lpf_sumerr.update(lpf_sumerr.update(jp.pidc.sumerr)));
    //    jp.pidc.sumerr = 0;
    //
    //    Serial.println();
  }
  if (cntr == 0) { // joint readings
    rangemap(0, 6, [](int i) {
      Serial.print(" j");
      Serial.print((int)(i + 1));
      Serial.print("_");
      Serial.print(j[i]->measured);
    });
    Serial.println();
  }
}
