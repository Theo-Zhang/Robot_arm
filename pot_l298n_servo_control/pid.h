
#include "defs.h"

class LPF {
  public:
    int lastout = 0;
    char multiply = 3, shift_right = 2;
    LPF() {}
    LPF(char m, char s) : multiply(m), shift_right(s) {}
    int update(int input) {
      int out = ((input + lastout * multiply ) >> shift_right);
      lastout = out;
      return out;
    }
};

class PID {
  public:
    LPF lpf1;
    LPF lpf2;

    int lasterr = 0;
    unsigned char initial = 1;

    int intg = 0;
    int target = 0;

    int pout, iout, dout;

    unsigned int sumerr = 0;

    void set_target(int t) {
      target = t;
    }

    char kp = 4, ki = -6, kd = 7;

    int update(int measurement) {
      /* Error */
      int err = target - measurement;

      /* Integration */
# define intg_clip 4096
      intg = clipp(intg + err, intg_clip);

      /* Derivative */
      if (initial) {
        initial = 0;
        lasterr = err;
      }
      int deri = err - lasterr;
      lasterr = err;

      deri = clipp(deri, 32);

      /* Multiply the Factors */
      // int kp = 4, ki = -3, kd = 8;
      pout = shift(err, kp);
      iout = shift(intg, ki);
      dout = shift(deri, kd);

      int thisout = pout + iout + dout;

      /* Low-Pass Filter */
      int out = lpf1.update(thisout);
      out = lpf2.update(out);

      /* error accumulator, to judge performance */
      if (sumerr < 60000) {
        sumerr += abs(err);
      }

      return out;
    }

    void zero_integrator() {
      intg = 0;
    }
};

class AD {
  public:
    //    const uc pin;
    uc pin;

    AD(uc p): pin(p) {}
    AD() {}
    int read() {
      return analogRead(pin);
    }
};

class MC {
  public:
    PID pidc;
    AD adc;
    int target = 512;
    int measurement = 0;
    uc pwm0, pwm1;

    int out;
    int dir;
    int measured;

    MC (uc ap, uc p0, uc p1) {
      pwm0 = p0;
      pwm1 = p1;
      adc = AD(ap);
      // init();
    }
    void init() {
      analogWrite(pwm0, 0);
      analogWrite(pwm1, 0);
      pinMode (pwm0, OUTPUT);
      pinMode (pwm1, OUTPUT);
      delay(1);
      target = adc.read();
    }

    void update() {
      /* read joint angle*/
      measured = adc.read();

      /* set target */
      pidc.set_target(target);

      /* PID loop */
      /* get output */
      out = pidc.update(measured);

      /* convert output to magnitude and direction */
      dir = 0;
      if (out < 0) {
        out = -out;
        dir = 1;
      }
      if (out > 511) {
        //    pidc.zero_integrator();
      }
      if (out > 255) {
        out = 255;
      }

      if (dir == 0) {
        analogWrite(pwm0, out);
        analogWrite(pwm1, 0);
      } else {
        analogWrite(pwm0, 0);
        analogWrite(pwm1, out);
      }
    }
};

class SerialCommandHandler {
  public:
    /* format: j1233 j2199\n */
    enum State {
      WAITING,
      GOTJ,
      GOTWHICH,
      GOTNUMBER,
    };

    State s = WAITING;
    uc cntr = 0;
    uc whichaxis = 0;

    int buf = 0;

    void (*callback)(uc, int);

    void append(char c) {
      buf = buf * 10 + (c - '0');
      if (cntr > 4) { // expecting 4 digits as of right now
        s = WAITING; // cancel parsing
      }
    }
    bool isnumber(char c) {
      return c >= '0' and  c <= '9';
    }
    void eat(char c) {
      switch (s) {
        case WAITING:
          if (c == 'j' or c == 'J') {
            s = GOTJ;
          }
          break;
        case GOTJ:
          if (isnumber(c)) {
            whichaxis = c - '0';
            if (whichaxis >= 1 and whichaxis <= 6) {
              s = GOTWHICH;
              buf = 0;
            } else {
              s = WAITING;
            }
          } else if (c == ' ') {
            // do nothing
          } else {
            s = WAITING;
          }
          break;

        case GOTWHICH:
          if (isnumber(c)) {
            s = GOTNUMBER;
            append(c);
          } else if (c == ' ') {
            // do nothing
          } else {
            s = WAITING;
          }
          break;

        case GOTNUMBER:
          if (isnumber(c)) {
            append(c);
          } else {
            callback(whichaxis, buf);
            s = WAITING;
          }
      }

    }
    void update() {
      while (Serial.available() > 0) {
        eat(Serial.read());
      }
    }

    SerialCommandHandler(void cb(uc, int)): callback(cb) {}
};
