# Motor Test Notes

## Motor 1

Status: preliminarily validated for bring-up and base-PWM characterization.

### Encoder / Mechanics

- Measured output pulses: `2682` pulses over `3` output-shaft revolutions
- Derived output pulses per revolution:
  - `2682 / 3 = 894`
- Working value chosen for framework testing:
  - `PPR_OUTPUT = 900`
- Implied gearbox ratio if encoder disk is `9` pulses per motor revolution:
  - `894 / 9 ≈ 99.3`
  - Practical conclusion: gearbox is approximately `100:1`

### Motor 1 bring-up observations

- Motor starts moving at about:
  - `PWM_start ≈ 110`
- Useful test range observed:
  - `120 .. 140`

### Sample results

All samples below were taken with:
- `dt_ms ≈ 100`
- framework output format:
  - `pwm_cmd,enc_count,delta_count,dt_ms,rpm`
- RPM formula used:
  - `rpm = delta_count / 900 * 600`
  - because `dt_ms = 100` and `PPR_OUTPUT = 900`

#### PWM 120

- Typical `delta_count`: `38 .. 50`
- Typical `rpm`: `25.3 .. 33.3`
- Practical center: about `29.3 rpm`

#### PWM 130

- Typical `delta_count`: `50 .. 60`
- Typical `rpm`: `33.3 .. 40.0`
- Practical center: about `36.7 rpm`

#### PWM 140

- Typical `delta_count`: `58 .. 71`
- Typical `rpm`: `38.7 .. 47.3`
- Practical center: about `43.0 rpm`

### Current conclusion

- Motor 1 encoder counting is usable for continued testing.
- `PPR_OUTPUT = 900` is a good working value for output-shaft RPM calculations.
- `PWM 110` is the approximate threshold where motion begins.
- A reasonable starting candidate for future `base_pwm` work is:
  - conservative: `120`
  - stronger / easier dual-motor matching: `130`

### Next recommended step

- Run the same characterization on Motor 2 with:
  - `PWM 120`
  - `PWM 130`
  - `PWM 140`
- Then compare Motor 1 vs Motor 2 to choose:
  - `base_pwm_left`
  - `base_pwm_right`
  - any fixed offset needed for straight driving

## Motor 2

Status: preliminarily validated and characterized against the same output-shaft
`PPR_OUTPUT = 900` assumption.

### Sample results

All samples below were taken with:
- `dt_ms ≈ 100`
- framework output format:
  - `pwm_cmd,enc_count,delta_count,dt_ms,rpm`
- RPM formula used:
  - `rpm = delta_count / 900 * 600`
  - because `dt_ms = 100` and `PPR_OUTPUT = 900`

#### PWM 120

- Typical `delta_count`: `46 .. 55`
- Typical `rpm`: `30.7 .. 36.7`
- Practical center: about `33.7 rpm`

#### PWM 130

- Typical `delta_count`: `62 .. 68`
- Typical `rpm`: `41.3 .. 45.3`
- Practical center: about `43.3 rpm`

#### PWM 140

- Typical `delta_count`: `75 .. 86`
- Typical `rpm`: `50.0 .. 57.3`
- Practical center: about `53.7 rpm`

### Comparison vs Motor 1

Motor 2 is clearly faster than Motor 1 at the same PWM.

Approximate comparison:

- `PWM 120`
  - Motor 1: about `29.3 rpm`
  - Motor 2: about `33.7 rpm`
- `PWM 130`
  - Motor 1: about `36.7 rpm`
  - Motor 2: about `43.3 rpm`
- `PWM 140`
  - Motor 1: about `43.0 rpm`
  - Motor 2: about `53.7 rpm`

### Current conclusion

- Motor 2 is stronger / faster than Motor 1 in the tested range.
- A shared PWM for both motors will not produce straight running.
- The next step should use different base values for left and right motors.

### Initial base PWM candidates for straight-driving experiments

- Motor 1 candidate: `130`
- Motor 2 candidate: `120`

These are not final PID numbers. They are only practical starting points for
the next sync test.

## Interpolated Base PWM

Important:

- The values below are **calculated base PWM estimates** from measured
  `PWM -> RPM` samples.
- They are **different from the hand-picked measured starting values** above.
- Earlier note versions had an RPM scale mistake of `x100`; the values below
  are the corrected ones using `PPR_OUTPUT = 900`.
- Keep both:
  - `measured base candidates`: useful for quick bring-up
  - `interpolated base estimates`: useful for more principled sync/PID setup

### Motor 1 linear estimate

Using approximate center points:

- `(120, 29.3 rpm)`
- `(130, 36.7 rpm)`
- `(140, 43.0 rpm)`

Approximate linear model:

- `rpm ≈ 0.685 * pwm - 52.9`
- inverse form:
  - `pwm ≈ (rpm + 52.9) / 0.685`

Example interpolated base PWM:

- target `35 rpm` -> `pwm ≈ 128.3`
- target `40 rpm` -> `pwm ≈ 135.6`
- target `45 rpm` -> `pwm ≈ 142.9`

### Motor 2 linear estimate

Using approximate center points:

- `(120, 33.7 rpm)`
- `(130, 43.3 rpm)`
- `(140, 53.7 rpm)`

Approximate linear model:

- `rpm ≈ 1.00 * pwm - 86.3`
- inverse form:
  - `pwm ≈ rpm + 86.3`

Example interpolated base PWM:

- target `35 rpm` -> `pwm ≈ 121.3`
- target `40 rpm` -> `pwm ≈ 126.3`
- target `45 rpm` -> `pwm ≈ 131.3`

### Practical sync starting pairs

If the goal is to make the two motors run close to each other before PID:

- around `35 rpm`
  - Motor 1: `128`
  - Motor 2: `121`
- around `40 rpm`
  - Motor 1: `136`
  - Motor 2: `126`
- around `45 rpm`
  - Motor 1: `143`
  - Motor 2: `131`

These are still estimates and should be verified with `SYNC_TEST`, but they
are better grounded than the first hand-picked base values.

## Sync Test Result

### Verified sync base pair

Tested pair:

- `baseL = 128`
- `baseR = 121`

Observed behavior:

- Left/right delta counts were close over repeated 100 ms windows
- Most `error = deltaL - deltaR` values stayed roughly within:
  - `-3 .. +6`
- A few larger excursions appeared:
  - about `-7`
  - about `-8`
- No strong one-sided bias dominated the whole run

### Practical conclusion

- `128 / 121` is a good **verified base pair** for two-motor bring-up
  before PID.
- This pair is smoother and better justified than using the same PWM on both
  motors.
- It is a suitable feedforward starting point for:
  - straight-driving experiments
  - future sync correction
  - future PID tuning

### Current recommended working values

- `PPR_OUTPUT = 900`
- `PWM_start ≈ 110`
- `baseL_verified = 128`
- `baseR_verified = 121`

## M7 Soft Sync PI Baseline

Status: verified as a gentle, usable first closed-loop sync mode.

### Goal

- Keep the two motors running close to each other
- Prefer smoothness over aggressive correction
- Avoid large PWM jumps and startup spikes

### Baseline parameters

- `baseL = 128`
- `baseR = 121`
- `kp = 0.30`
- `ki = 0.02`
- `kd = 0.00` (not used)
- `PPR_OUTPUT = 900`
- `dt_ms = 100`
- `EMA alpha = 0.20`
- `trim_limit = 15`
- `slew_step = 2`

### Control style

- Feedforward-first:
  - motors start from the verified base pair `128 / 121`
- PI sync only:
  - correction is based on `error = deltaL - deltaR`
- Soft behavior:
  - filtered error
  - small trim
  - very small PWM step changes

### Observed behavior

- `pwmL` stayed mostly around `127 .. 129`
- `pwmR` stayed mostly around `120 .. 122`
- `trim` stayed small, usually near zero
- Error still moved around, but the controller remained calm
- No strong violent correction or obvious oscillation was observed

### Practical conclusion

- This `m7` baseline is good enough to use as the first straight-driving sync layer.
- It is not a final tuned controller.
- It is intentionally conservative and smooth.
- `kp = 0.35` was tried and did not show a clearly better result than `kp = 0.30`
  for the current objective.

### Baseline to keep

- `baseL = 128`
- `baseR = 121`
- `kp = 0.30`
- `ki = 0.02`
