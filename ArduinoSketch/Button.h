class Button {
public:

  Button(int pin) : mPin(pin), mLastState(true), mStateChanged(false) { }

  void initialise(){
    pinMode(mPin, INPUT_PULLUP);
  }

  void update() {
    bool newState = digitalRead(mPin);
    mStateChanged = mLastState != newState;
    mLastState = newState;    
  }

  bool changed() {
    return mStateChanged;
  }

  bool pressed() {
    return changed() && mLastState == false;
  }

  bool released() {
    return changed() && mLastState == true;
  }

  int mPin;
  bool mLastState;
  bool mStateChanged;
};
