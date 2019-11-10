AVR simulator
=============

get the simulator:
 git clone https://github.com/Traumflug/simulavr.git

place it in the parent directory of this project

To simulator atmega328(p):

- set the project CONFIG_AVR_SIMU=y option in the config file

- set the device in the project C code:
  SIMINFO_DEVICE("atmega328");

- then run the simulator like this:
  ../simulavr/src/simulavr  -f alarm -d atmega328 -F 16000000
