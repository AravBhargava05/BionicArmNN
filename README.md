# EMG → hand control

A single muscle-signal channel controls five servos: relax to open the hand, activate to close it. The Arduino sketch calibrates a decision threshold for each session, then translates live EMG measurements into synchronized finger commands.

**Arduino / C++ · PCA9685 · biosignal processing**

[Hardware and demonstration ↗](https://aravsportfolio.com/?project=custom-analog-front-end-for-bionic-arm-control) · [Portfolio ↗](https://aravsportfolio.com)

## Signal to motion

```text
EMG on A0 → baseline subtraction → half-wave rectification
          → exponential smoothing → rolling standard deviation
          → open / close threshold → five servo channels
```

The threshold is the midpoint between two calibration measurements. Despite the repository’s historical name, the checked-in sketch uses a threshold classifier; it does not train a neural network or KNN model. The portfolio shows the broader bionic-arm hardware project and its other iterations.

## Run on hardware

1. Install the **Adafruit PWM Servo Driver Library** in the Arduino IDE.
2. Open [`bionic.ino`](bionic.ino). If the IDE asks to place it in a matching `bionic` sketch folder, accept.
3. Connect the conditioned EMG output to `A0` and the PCA9685 to the board’s I²C pins. Use a suitable servo supply and a shared ground.
4. Check the PWM limits and servo travel for your mechanism before powering the actuators. Channels `0–4` receive the same command.
5. Upload and open Serial Monitor at **115200 baud**. Relax during baseline calibration; send a character to start each prompted Open and Close calibration.
6. During operation, send a positive number to override the threshold.

| Setting | Default |
| :--- | :--- |
| Baseline samples | 200 |
| Calibration samples per gesture | 200 |
| Live feature window | 30 samples |
| Smoothing factor | 0.1 |
| Loop delay | 300 ms |
| Open / closed angles | 30° / 150° |

## Implementation notes

Calibration measures the standard deviation of rectified samples, while the live loop measures a smoothed envelope. Those feature distributions can differ, so the midpoint threshold may require manual adjustment. The 30-sample live window spans roughly nine seconds at the configured loop delay; it is not a low-latency controller.

This repository contains the control sketch. Hardware schematics, datasets, and quantitative classification results are not included.
