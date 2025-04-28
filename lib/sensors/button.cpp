

// listen for pin to flip flop
//

#ifndef C_BUT
#define C_BUT

class button
{
private:
  int last_trigger; // timestamp of last trigger
  int debounce = 1000;

public:
  button(); // callback func here
  void check(); // check if button is pressed and then trigger callback
};

#endif
