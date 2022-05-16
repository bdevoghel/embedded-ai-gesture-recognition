/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_LITE_MICRO_EXAMPLES_MAGIC_WAND_CONSTANTS_H_
#define TENSORFLOW_LITE_MICRO_EXAMPLES_MAGIC_WAND_CONSTANTS_H_

#define MAGIC_WAND_MODEL
#ifdef MAGIC_WAND_MODEL
#define ACCELEROMETER_FACTOR 1
#else
#define ACCELEROMETER_FACTOR -1
#endif

// The expected accelerometer data sample frequency
const float kTargetHz = 52;

// The size of the buffer on which the predictions are done
const int kAccellerometer_buffer_size = 600; // TODO : determine size of buffer


// What gestures are supported.
constexpr int kGestureCount = 3;
constexpr int kNoGesture = 0;
constexpr int kWalkingGesture = 1;
constexpr int kStairsGesture = 2;

// These control the sensitivity of the detection algorithm. If you're seeing
// too many false positives or not enough true positives, you can try tweaking
// these thresholds. Often, increasing the size of the training set will give
// more robust results though, so consider retraining if you are seeing poor
// predictions.
const float kDetectionThreshold = 0.4f;
const int kPredictionHistoryLength = 2; // buffer length of previous predictions (one prediction / tick)
const int kPredictionSuppressionDuration = 0; // ticks during which a detection will not result in a trigger

#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_MAGIC_WAND_CONSTANTS_H_
