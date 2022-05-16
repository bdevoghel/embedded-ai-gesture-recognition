# Embedded AI - Gesture Recognition
This is a branch of the Tensorflow library, aimed at developping a personalized variation of the Magic Wand example project.

## Environment
For this project, a Conda environment with `pycrypto` and `pyserial` is used.
```bash
conda create -n embedded_ai python=3.6
conda activate embedded_ai
pip3 install pycrypto pyserial --user
```

Please note the used branch of the Tensorflow repositiry is `r2.4` (set as default in this repository).

The application code is located in [/tensorflow/lite/micro/examples/magic_wand](/tensorflow/lite/micro/examples/magic_wand) (README there is not updated).

## MCU development board
For this project we use the Sparkfun Edge micro-controller unit development board.

## Building and Flashing the MCU
The [Makefile](/Makefile) at the root should allow to build and flash the MCU with the `build` and `flash` targets respectively.
Big inspiration was taken from [Google's Codelabs tutorial](https://codelabs.developers.google.com/codelabs/sparkfun-tensorflow).

## Writing debug logs
To log data in the application code, the `error_reporter->Report()` method can be called. It supports the standard `printf` tokens for string interpolation, which you can use to include important information in logs:
```cpp
error_reporter->Report("Heard %s (%d) @%dms", found_command, score, current_time);
```

## Model training
The dataset described in [Casale et. al. 2011](https://www.researchgate.net/publication/221258784_Human_Activity_Recognition_from_Accelerometer_Data_Using_a_Wearable_Device) is used and a Tensorflow model is build to fit this data. The model is then converted to a TF Lite model and embedded on the Sparkfun Edge. More details available in the notebook. 

Note : to be able to run the notebook, is may be needed to move it out of the Tensorflow library together with the dataset. Also the following libraries are needed to be added the the `embedded_ai` environment created earlier:
```
pip install tensorflow tensorflow-model-optimization jupyter matplotlib pandas tqdm
```
