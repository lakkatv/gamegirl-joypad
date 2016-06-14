#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <wiringPi.h>
#include <iostream>

#define die(str, args...) do { \
    perror(str); \
    exit(EXIT_FAILURE); \
  } while(0)

using namespace std;

int _button_pin1 = 10; // GPIO10
int _button_pin2 = 26; // GPIO26
int _button_pin3 = 11; // GPIO11
int _button_pin4 = 25; // GPIO25

int pinUpRead(int pin_to_pull_up, int pin_to_output) {
  // Set pins for Charlieplexing with diodes: http://www.mosaic-industries.com/embedded-systems/microcontroller-projects/electronic-circuits/matrix-keypad-scan-decode
  pinMode(pin_to_pull_up, INPUT);
  pullUpDnControl(pin_to_pull_up, PUD_UP);

  pinMode(pin_to_output, OUTPUT);
  digitalWrite(pin_to_output, LOW);

  // Wait that the pins stabilise and read
  delayMicroseconds(1);
  int value = !digitalRead(pin_to_pull_up);

  // Reset pin for next read
  pinMode(pin_to_output, INPUT);

  return value;
}

void set_key_bit(int fd, int button) {
  if (ioctl(fd, UI_SET_KEYBIT, button) < 0) {
    die("error: ioctl");
  }
}

void set_button_event(int fd, int button, int value) {
  struct input_event ev;
  ev.type = EV_KEY;
  ev.code = button;
  ev.value = value;
  if (write(fd, &ev, sizeof(struct input_event)) < 0) {
    die("error: write");
  }
}

int main() {
  // Initialise GPIO
  wiringPiSetupGpio();
  
  pinMode(_button_pin1, INPUT);
  pinMode(_button_pin2, INPUT);
  pinMode(_button_pin3, INPUT);
  pinMode(_button_pin4, INPUT);

  // Initialise udev
  struct uinput_user_dev uidev;
  struct input_event ev;

  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (fd < 0)
    die("error: open");

  if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
    die("error: ioctl");
  if (ioctl(fd, UI_SET_EVBIT, EV_REL) < 0)
    die("error: ioctl");
  if (ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0)
    die("error: ioctl");

  set_key_bit(fd, BTN_DPAD_UP);
  set_key_bit(fd, BTN_DPAD_DOWN);
  set_key_bit(fd, BTN_DPAD_LEFT);
  set_key_bit(fd, BTN_DPAD_RIGHT);
  set_key_bit(fd, BTN_NORTH);
  set_key_bit(fd, BTN_SOUTH);
  set_key_bit(fd, BTN_WEST);
  set_key_bit(fd, BTN_EAST);
  set_key_bit(fd, BTN_START);
  set_key_bit(fd, BTN_SELECT);
  set_key_bit(fd, BTN_TL);
  set_key_bit(fd, BTN_TR);

  ioctl(fd, UI_SET_EVBIT, EV_ABS);
  ioctl(fd, UI_SET_ABSBIT, ABS_X);
  ioctl(fd, UI_SET_ABSBIT, ABS_Y);
  uidev.absmin[ABS_X] = 0;
  uidev.absmax[ABS_X] = 4;
  uidev.absmin[ABS_Y] = 0;
  uidev.absmax[ABS_Y] = 4;

  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Gamegirl Controller");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor  = 1;
  uidev.id.product = 1;
  uidev.id.version = 1;

  if (write(fd, &uidev, sizeof(uidev)) < 0)
    die("error: write");

  if (ioctl(fd, UI_DEV_CREATE) < 0)
    die("error: ioctl");

  int up_button, up_button_old         = 0;
  int down_button, down_button_old     = 0;
  int left_button, left_button_old     = 0;
  int right_button, right_button_old   = 0;
  int x_button, x_button_old           = 0;
  int b_button, b_button_old           = 0;
  int y_button, y_button_old           = 0;
  int a_button, a_button_old           = 0;
  int start_button, start_button_old   = 0;
  int selec_button, selec_button_old   = 0;
  int l1_button, l1_button_old         = 0;
  int r1_button, r1_button_old         = 0;

  while (1) {
    // Read GPIO
    up_button_old     = up_button;
    down_button_old   = down_button;
    left_button_old   = left_button;
    right_button_old  = right_button;
    x_button_old      = x_button;
    b_button_old      = b_button;
    y_button_old      = y_button;
    a_button_old      = a_button;
    start_button_old  = start_button;
    selec_button_old  = selec_button;
    l1_button_old     = l1_button;
    r1_button_old     = r1_button;

    up_button    = pinUpRead(_button_pin1, _button_pin2); // S1
    down_button  = pinUpRead(_button_pin1, _button_pin3); // S2
    left_button  = pinUpRead(_button_pin1, _button_pin4); // S3
    right_button = pinUpRead(_button_pin2, _button_pin1); // S4
    x_button     = pinUpRead(_button_pin2, _button_pin3); // S5
    b_button     = pinUpRead(_button_pin2, _button_pin4); // S6
    y_button     = pinUpRead(_button_pin3, _button_pin1); // S7
    a_button     = pinUpRead(_button_pin3, _button_pin2); // S8
    start_button = pinUpRead(_button_pin3, _button_pin4); // S9
    selec_button = pinUpRead(_button_pin4, _button_pin1); // S10
    l1_button    = pinUpRead(_button_pin4, _button_pin2); // S11
    r1_button    = pinUpRead(_button_pin4, _button_pin3); // S12

    // Write to udev
    if (up_button != up_button_old) {
      set_button_event(fd, BTN_DPAD_UP, up_button != 0);
    }

    if (down_button != down_button_old) {
      set_button_event(fd, BTN_DPAD_DOWN, down_button != 0);
    }

    if (left_button != left_button_old) {
      set_button_event(fd, BTN_DPAD_LEFT, left_button != 0);
    }

    if (right_button != right_button_old) {
      set_button_event(fd, BTN_DPAD_RIGHT, right_button != 0);
    }

    if (x_button != x_button_old) {
      set_button_event(fd, BTN_NORTH, x_button != 0);
    }

    if (b_button != b_button_old) {
      set_button_event(fd, BTN_SOUTH, b_button != 0);
    }

    if (y_button != y_button_old) {
      set_button_event(fd, BTN_WEST, y_button != 0);
    }

    if (a_button != a_button_old) {
      set_button_event(fd, BTN_EAST, a_button != 0);
    }

    if (start_button != start_button_old) {
      set_button_event(fd, BTN_START, start_button != 0);
    }

    if (selec_button != selec_button_old) {
      set_button_event(fd, BTN_SELECT, selec_button != 0);
    }

    if (l1_button != l1_button_old) {
      set_button_event(fd, BTN_TL, l1_button != 0);
    }

    if (r1_button != r1_button_old) {
      set_button_event(fd, BTN_TR, r1_button != 0);
    }

    memset(&ev, 0, sizeof(struct input_event));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    if (write(fd, &ev, sizeof(struct input_event)) < 0)
      die("error: write");

    delay(1000/120);
  }

  // Destroy udev
  if (ioctl(fd, UI_DEV_DESTROY) < 0)
    die("error: ioctl");

  close(fd);
  
  return 0;
}
