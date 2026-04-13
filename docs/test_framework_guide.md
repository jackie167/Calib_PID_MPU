# Test Framework Guide

File firmware hiện tại chạy theo framework test mode-based trong:

- `src/test_framework.h`

Mục tiêu:

- test encoder từ đầu
- đo xung / vòng
- tìm PWM bắt đầu quay
- đo `delta_count`
- đo `rpm`
- tìm base PWM cho từng motor
- test sync 2 motor
- sau đó mới dùng `m7` soft sync PI

## 1. Upload firmware

```bash
cd "/Users/dinhthi/Test dc encoder"
. .venv/bin/activate
pio run -e seeed_xiao_esp32c3 -t upload
```

## 2. Mở serial monitor

```bash
cd "/Users/dinhthi/Test dc encoder"
. .venv/bin/activate
pio device monitor
```

Khi boot xong, framework sẽ in danh sách command.

## 3. Nguyên tắc dùng

- Gõ lệnh trong serial monitor rồi nhấn Enter
- Mỗi lần đổi mode, framework tự:
  - dừng motor
  - reset state
  - reset timer đo
- Nếu muốn reset lại encoder và state thủ công:

```text
r
```

## 4. Các mode

### `m0` - Idle

Trạng thái nghỉ:

- motor dừng
- chỉ in encoder count hiện tại

### `m1` - Count Pulse

Dùng để kiểm tra encoder bằng quay tay.

Log:

```text
enc_count,delta_count,dt_ms
```

Mục tiêu:

- encoder có đếm không
- có bị nhảy loạn không
- đếm có dừng hẳn khi không quay không

### `m2` - PWM Hold

Giữ 1 PWM cố định cho 1 motor.

Set PWM trước:

```text
p120
```

Vào mode:

```text
m2
```

Log:

```text
pwm_cmd,enc_count,delta_count,dt_ms,rpm
```

Mục tiêu:

- xem motor có quay không
- `delta_count` có ổn định theo thời gian không

### `m3` - PWM Sweep

Quét nhiều mức PWM tự động.

Vào mode:

```text
m3
```

Log:

```text
pwm_cmd,enc_count,delta_count,dt_ms,rpm
```

Mục tiêu:

- tìm vùng chết
- tìm vùng PWM hữu ích
- xem delta/rpm có tăng theo PWM không

### `m4` - RPM Test

Đo `rpm` với 1 PWM cố định sau khi đã chốt được `PPR_OUTPUT`.

Ví dụ:

```text
k900
p120
m4
```

Log:

```text
pwm_cmd,enc_count,delta_count,dt_ms,rpm
```

### `m5` - Base PWM Test

Dùng để dò base PWM sơ bộ.

Ví dụ:

```text
k900
t35
p128
m5
```

Trong đó:

- `t35` là target tham khảo
- mode này chưa tự điều khiển theo target, chỉ in để bạn đối chiếu

Log:

```text
pwm_cmd,enc_count,delta_count,dt_ms,rpm
```

### `m6` - Sync Test

Chạy 2 motor cùng lúc với 2 base PWM cố định.

Ví dụ:

```text
r
kl900
kr900
pl128
pr121
m6
```

Log:

```text
baseL,encL,deltaL,baseR,encR,deltaR,dt_ms,rpmL,rpmR,error
```

Ý nghĩa:

- `error = deltaL - deltaR`
- nếu `error > 0`: motor trái nhanh hơn
- nếu `error < 0`: motor phải nhanh hơn

### `m7` - Soft Sync PI

Đây là mode sync kín vòng nhẹ, ưu tiên mượt.

Mặc định đang dùng:

- `baseL = 128`
- `baseR = 121`
- `kp = 0.30`
- `ki = 0.02`
- `PPR = 900`

Chạy nhanh:

```text
r
m7
```

Log:

```text
baseL,baseR,pwmL,pwmR,deltaL,deltaR,error,trim,dt_ms
```

Ý nghĩa:

- `error = deltaL - deltaR`
- `trim` là phần hiệu chỉnh PI rất nhẹ
- `pwmL/pwmR` sẽ chỉ dao động nhỏ quanh base

## 5. Command list

### Chọn motor đơn

```text
s1
s2
```

- `s1`: chọn motor 1 cho các mode `m1..m5`
- `s2`: chọn motor 2 cho các mode `m1..m5`

### Set PWM cho mode đơn

```text
p120
```

### Set base cho sync test

```text
pl128
pr121
```

### Set base cho m7

```text
bl128
br121
```

### Set PPR

```text
k900
kl900
kr900
```

- `k900`: cho mode đơn
- `kl900`: PPR trái
- `kr900`: PPR phải

### Set target tham khảo

```text
t35
```

### Set gain cho m7

```text
kp0.30
ki0.02
```

### Reset

```text
r
```

### Help

```text
h
```

## 6. Quy trình test khuyến nghị

### Bước 1: encoder

```text
r
m1
```

Quay tay để xác nhận encoder sống và đếm sạch.

### Bước 2: xung / vòng đầu ra

Trong `m1`:

- reset count
- quay đúng số vòng đầu ra
- tính `PPR_OUTPUT`

Giá trị hiện đã chốt:

- `PPR_OUTPUT = 900`

### Bước 3: tìm PWM bắt đầu quay

```text
r
k900
p110
m2
```

Giá trị hiện đã chốt:

- `PWM_start ≈ 110`

### Bước 4: đo vài mức PWM

Đo ở:

- `120`
- `130`
- `140`

Với:

```text
k900
p120
m4
```

sau đó đổi `p130`, `p140`.

### Bước 5: sync 2 motor base cố định

```text
r
kl900
kr900
pl128
pr121
m6
```

Giá trị hiện đã verify:

- `baseL = 128`
- `baseR = 121`

### Bước 6: soft sync PI

```text
r
m7
```

Baseline hiện đã chốt:

- `baseL = 128`
- `baseR = 121`
- `kp = 0.30`
- `ki = 0.02`

## 7. Giá trị hiện tại đã chốt

- `PPR_OUTPUT = 900`
- `PWM_start ≈ 110`
- `baseL_verified = 128`
- `baseR_verified = 121`
- `m7 kp = 0.30`
- `m7 ki = 0.02`

## 8. Ghi chú

- Nếu chỉ cần debug nhanh, cứ đi theo thứ tự:
  - `m1` -> `m2` -> `m4` -> `m6` -> `m7`
- Nếu thấy số liệu lạ:
  - reset bằng `r`
  - vào lại mode
- Không cần dùng tất cả mode mỗi lần; chọn đúng mode theo mục tiêu đang test.
