# Embedded AI - Gesture Recognition
This is a branch of the Tensorflow library, aimed at developping a personalized variation of the Magic Wand example project.

## Environment
For this project, a Conda environment with `pycrypto` and `pyserial` is used.
```
conda create -n embedded_ai
conda activate embedded_ai
pip3 install pycrypto pyserial --user
```

Please note the used branch of the Tensorflow repositiry is `r2.4` (set as default in this repository).

## MCU development board
For this project we use the Sparkfun Edge micro-controller unit development board.

## Building and Flashing the MCU
The [Makefile](/Makefile) at the root should allow to build and flash the MCU with the `build` and `flash` targets respectively.
Big insiration was taken from [Google's Codelabs tutorial](https://codelabs.developers.google.com/codelabs/sparkfun-tensorflow).
