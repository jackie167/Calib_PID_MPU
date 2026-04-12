#!/usr/bin/env python3
import argparse
import csv
import sys
import time
import select
import socket
import termios
import tty

try:
    import serial
except ImportError:
    serial = None

try:
    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation
except ImportError:
    plt = None


def parse_args():
    p = argparse.ArgumentParser(description="Live plot CSV data from serial or UDP.")
    p.add_argument("--port", default="/dev/cu.usbmodem1101", help="Serial port")
    p.add_argument("--baud", type=int, default=115200, help="Baud rate")
    p.add_argument("--max-points", type=int, default=300, help="Max points to keep")
    p.add_argument("--send-keys", action="store_true", help="Send single-key commands to serial")
    p.add_argument("--udp", action="store_true", help="Receive data via UDP instead of serial")
    p.add_argument("--udp-port", type=int, default=4210, help="UDP port to listen on")
    return p.parse_args()


def main():
    args = parse_args()
    if serial is None or plt is None:
        print("Missing dependencies. Install with:", file=sys.stderr)
        print("  python3 -m pip install pyserial matplotlib", file=sys.stderr)
        sys.exit(1)

    # --- input source: UDP or Serial ---
    udp_sock = None
    ser = None
    if args.udp:
        udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_sock.bind(("0.0.0.0", args.udp_port))
        udp_sock.setblocking(False)
        print(f"[UDP] Listening on port {args.udp_port} ...")
    else:
        if serial is None:
            print("pyserial not installed", file=sys.stderr)
            sys.exit(1)
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
        time.sleep(0.2)

    headers = None
    data = {}
    x = []
    start = time.time()

    fig, ax = plt.subplots()
    lines = {}

    def ensure_line(name):
        if name not in lines:
            (line,) = ax.plot([], [], label=name)
            lines[name] = line
            ax.legend(loc="upper right")

    # optional raw keyboard
    old_term = None
    if args.send_keys and sys.stdin.isatty():
        old_term = termios.tcgetattr(sys.stdin)
        tty.setcbreak(sys.stdin.fileno())

    def read_key():
        if not args.send_keys or not sys.stdin.isatty():
            return None
        r, _, _ = select.select([sys.stdin], [], [], 0)
        if r:
            return sys.stdin.read(1)
        return None

    def update(_):
        nonlocal headers
        k = read_key()
        if k:
            if k == "q":
                plt.close("all")
                return []
            if ser:
                ser.write(k.encode("utf-8"))
        # read lines from UDP or serial
        lines_in = []
        if udp_sock:
            try:
                while True:
                    data, _ = udp_sock.recvfrom(4096)
                    for l in data.decode(errors="ignore").splitlines():
                        l = l.strip()
                        if l:
                            lines_in.append(l)
            except BlockingIOError:
                pass
        else:
            while True:
                line = ser.readline().decode(errors="ignore").strip()
                if not line:
                    break
                lines_in.append(line)
        while True:
            if not lines_in:
                break
            line = lines_in.pop(0)
            # Header
            if headers is None:
                if "," in line and "rpm" in line:
                    headers = [h.strip() for h in line.split(",")]
                    for h in headers:
                        data[h] = []
                    continue
                else:
                    continue
            # Data row
            row = next(csv.reader([line]))
            if len(row) != len(headers):
                continue
            t = time.time() - start
            x.append(t)
            for h, v in zip(headers, row):
                try:
                    data[h].append(float(v))
                except ValueError:
                    data[h].append(float("nan"))

            # trim
            if len(x) > args.max_points:
                x.pop(0)
                for h in headers:
                    data[h].pop(0)

        if headers:
            for h in headers:
                ensure_line(h)
                lines[h].set_data(x, data[h])
            ax.relim()
            ax.autoscale_view()
        return list(lines.values())

    ani = FuncAnimation(fig, update, interval=200)
    ax.set_title("Serial Plot")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Value")
    plt.show()

    if old_term is not None:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_term)


if __name__ == "__main__":
    main()
