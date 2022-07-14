# ifndef _DEFS_
# define _DEFS_

typedef unsigned long ul;
typedef unsigned char uc;
typedef unsigned int ui;

void rangemap(int start, int len, void (*cb)(int)) {
  for (int i = start; i < start + len; i++) {
    cb(i);
  }
}

class Every {
    ul last_t;
  public:
    const ul interval; void(*callback)();
    Every(ul itvl, void(*cb)()) : interval(itvl), callback(cb) {}
    // interval(microseconds) should be an unsigned long (uint32) less than or equals 2^30.
    void update() {
      auto m = micros();
      if ((ul)(m - last_t) >= interval) {
        callback(); last_t += interval;
      }
    }
};

int clip(int input, int low, int high) {
  if (input > high) {
    return high;
  } else if (input < low) {
    return low;
  } else {
    return input;
  }
}

int clipp(int input, int value) {
  return clip(input, -value, value - 1);
}

int shift(int input, char places) {
  if (places > 0) {
    return input << places;
  } else if (places == 0) {
    return input;
  } else {
    return input >> (-places);
  }
}
# endif
