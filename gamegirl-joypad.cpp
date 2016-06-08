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

  set_key_bit(fd, BTN_START);
  set_key_bit(fd, BTN_WEST);
  set_key_bit(fd, BTN_EAST);
  set_key_bit(fd, BTN_A);

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

  int start_button, start_button_old = 0;
  int left_button, left_button_old   = 0;
  int right_button, right_button_old = 0;
  int a_button, a_button_old         = 0;

  while (1) {
    // Read GPIO
    start_button_old = start_button;
    left_button_old  = left_button;
    right_button_old = right_button;
    a_button_old     = a_button;

    start_button = pinUpRead(_button_pin1, _button_pin2); // S1
    left_button  = pinUpRead(_button_pin1, _button_pin4); // S2
    right_button = pinUpRead(_button_pin1, _button_pin3); // S3
    a_button     = pinUpRead(_button_pin2, _button_pin1); // S4
    pinUpRead(_button_pin2, _button_pin4); // S5
    pinUpRead(_button_pin2, _button_pin3); // S6
    pinUpRead(_button_pin4, _button_pin1); // S7
    pinUpRead(_button_pin4, _button_pin2); // S8

    // Write to udev
    if (start_button != start_button_old) {
      set_button_event(fd, BTN_START, start_button != 0);
    }

    if (left_button != left_button_old) {
      set_button_event(fd, BTN_WEST, left_button != 0);
    }

    if (right_button != right_button_old) {
      set_button_event(fd, BTN_EAST, right_button != 0);
    }

    if (a_button != a_button_old) {
      set_button_event(fd, BTN_A, a_button != 0);
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