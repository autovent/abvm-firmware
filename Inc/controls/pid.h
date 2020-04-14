#ifndef PID_H
#define PID_H

class PID {
public:
  struct Params {
    float Kp;
    float Ki;
    float Kd;
  };

  PID(float Kp, float Ki, float Kd, float dt);

  PID(Params params, float dt);

  void reset();

  float update(float target, float meas);

  Params params;

private:
  float err_acc_;
  float period_;
  float err_last_;
  bool is_init_;
};

#endif