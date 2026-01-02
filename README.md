# Bionic Arm (EMG-Controlled Hand) — Open/Close via Variance-Based EMG Classification

This project is a **simple, robust EMG control loop** for a multi-servo prosthetic/bionic hand. It reads a single EMG channel from a muscle sensor, extracts a stable signal feature in real time, and drives **5 servos simultaneously** to perform two gestures:

- **Open**
- **Close**

Instead of trying to classify raw EMG amplitude (which drifts a lot), the controller uses **signal variability (standard deviation)** over a short time window as the decision feature.

---

## What this project demonstrates

- **EMG signal conditioning (software-side):** baseline subtraction + rectification + smoothing  
- **Feature extraction:** rolling **standard deviation** of the EMG envelope  
- **Calibration workflow:** per-user, per-session calibration for repeatability  
- **Real-time embedded control:** turning noisy biosignals into stable actuator commands  
- **Multi-actuator synchronization:** one gesture command drives **5 servo channels** together  

---

## Hardware / Stack

- **Microcontroller:** Arduino (or compatible)
- **EMG input:** single analog EMG channel into **A0**
- **Servo control:** PCA9685 via `Adafruit_PWMServoDriver` (I2C)
- **Actuators:** 5 hobby servos (one per finger, or mechanically linked)

---

## Core idea (why SD instead of raw EMG amplitude)

Raw EMG amplitude changes across:
- electrode placement
- sweat / contact quality
- fatigue
- baseline drift

This code uses **standard deviation of the smoothed EMG envelope** over a short window, because:
- “relaxed” EMG tends to be lower variance
- “activated / clenched” EMG tends to be higher variance
- SD is more stable than raw peak amplitude for simple two-gesture control

---

## How it works (pipeline)

### 1) Baseline calibration (relaxed muscle)
At startup, the program collects a few hundred samples while the user relaxes:
- Computes a baseline ADC level
- Subtracts it from future readings

Conceptually:
- `baseline = average(raw_samples_while_relaxed)`
- `corr = raw - baseline`

---

### 2) Rectification
EMG is effectively noisy and oscillatory; we keep the magnitude:
- `corr = max(raw - baseline, 0)`

---

### 3) Envelope smoothing (EMA)
To stabilize the signal, it applies an exponential moving average:
- `env = alpha * x + (1 - alpha) * env`

---

### 4) Rolling SD window (feature extraction)
It stores the last `STD_WINDOW` envelope samples and computes standard deviation:
- `liveSD = std(env[t-STD_WINDOW : t])`

---

### 5) Classification
A single threshold separates gestures:
- `liveSD <= threshold` → **Open**
- `liveSD > threshold` → **Close**

---

### 6) Motor output (5-channel synchronized actuation)
Each gesture maps to a servo angle, and the code sends the same PWM command to all 5 channels:
- **Open** → `angleOpen`
- **Close** → `angleClosed`

---

## Calibration routine (per user)

After baseline is set, the program calibrates SD for each gesture:

1. User holds **Open** → program records samples → computes `SD_open`  
2. User holds **Close** → program records samples → computes `SD_close`  
3. Threshold is set at the midpoint:

- `thresholdSD = (SD_open + SD_close) / 2`

This makes it easy to re-run calibration whenever electrode placement changes.

---

## Live tuning (Serial)

You can type a number into the Serial Monitor to update the threshold in real time.

Example:
- type `12.5` and press enter → sets `thresholdSD = 12.5`

Useful for quickly adapting to different sensors / muscles / environments.

---

## Key parameters (what they mean)

- `BASELINE_SAMPLES`  
  More samples = steadier baseline, slower startup.

- `CAL_STD_SAMPLES`  
  More samples = better SD estimate per gesture.

- `STD_WINDOW`  
  Feature window size. Bigger = more stable, slower response.

- `SMOOTH_ALPHA`  
  Smoothing strength. Lower = smoother but more lag.

- `GESTURE_HOLD_MS`  
  Delay between decisions. Lower = faster responsiveness.

---

## How to run

1. Wire EMG output → `A0` (power sensor appropriately).
2. Wire PCA9685 → I2C (SDA/SCL) and power servos.
3. Upload the sketch.
4. Open Serial Monitor (**115200 baud**).
5. Follow prompts:
   - relax for baseline
   - press a key to begin each gesture calibration
6. After calibration, the hand should switch **Open/Close** based on muscle activation.

---

## Why this matters for robotics / prosthetics

This project is a miniature version of a real biosignal control pipeline:

**noisy biological measurement → feature extraction → stable classification → actuator control**

Even with one EMG channel and two gestures, it demonstrates the fundamentals of:
- building reliable intent detection from noisy sensors
- calibrating per user
- making control stable enough to feel usable in real time
