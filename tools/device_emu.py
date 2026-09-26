"""
FastAcq device emulator for end-to-end host testing without hardware.

Speaks the binary CMD/FRAME protocol on a com0com port (pair the host with
the other end). Implements ACK/STATUS/PONG, synthetic RAW chirp frames using
the SET_RAMP / START_CHIRP geometry, and TRACE frames (test mode) with a
realistic step timeline. Usage:  python device_emu.py COM9
"""
import sys, time, struct, math, random, threading
import serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM9"

MAGIC = 0xFACEDA7A
FS    = 60_000_000
CHUNK = 16384

# opcodes
CMD = {0x01:"START_CHIRP",0x02:"GET_FRAME",0x03:"PING",0x04:"SET_SAMPLES",0x05:"SET_MODE",
       0x06:"SET_DATA_MASK",0x07:"SET_INTERVAL",0x08:"TRIGGER",0x09:"GET_STATUS",
       0x0A:"SET_AMPLITUDE",0x0B:"SET_BURST",0x0C:"ABORT",0x0D:"SET_TRACE",0x0E:"SET_RAMP",0x0F:"GET_TRACE"}
FRAME_ID_PONG, FRAME_ID_STATUS, FRAME_ID_ACK, FRAME_ID_TRACE = 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFC
F_RAW, F_FFT, F_FFTV, F_STATUS, F_ACK, F_TRACE = 1, 2, 4, 8, 16, 32

def crc8(data):
    c = 0
    for b in data:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x07) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c

_tbl = []
for i in range(256):
    c = i
    for _ in range(8):
        c = (c >> 1) ^ 0xEDB88320 if c & 1 else c >> 1
    _tbl.append(c)
def crc32(data, crc=0xFFFFFFFF):
    for b in data:
        crc = _tbl[(crc ^ b) & 0xFF] ^ (crc >> 8)
    return crc

def header(frame_id, ts, n_samples, freq, flags, fft_size=0, peak_bin=0, peak_mag=0.0,
           res=0.0, raw_bytes=0, fft_bytes=0, reserved=b"\0"*8, srate=FS, reserved0=0):
    return struct.pack("<IIIIIHBBIIffII8s", MAGIC, frame_id, ts, n_samples, srate, freq,
                       flags, reserved0, fft_size, peak_bin, peak_mag, res, raw_bytes, fft_bytes, reserved)

class Dev:
    def __init__(self, ser):
        self.ser = ser
        self.lock = threading.Lock()
        self.mode = 0; self.mask = 3; self.interval = 30
        self.freq = 458; self.amp = 4095; self.burst = 1; self.samples_ovr = 0
        self.rise_us = 0; self.fall_us = 0
        self.trace_on = False
        self.frame_id = 0
        self.t0 = time.time()
        self.last_cycle = time.time()
        self.trace = []

    def tick(self): return int((time.time() - self.t0) * 1000) & 0xFFFFFFFF
    def us(self):   return int((time.time() - self.t0) * 1e6) & 0xFFFFFFFF

    def send(self, hdr, payload=b""):
        crc = crc32(payload, crc32(hdr)) ^ 0xFFFFFFFF
        with self.lock:
            self.ser.write(hdr + payload + struct.pack("<I", crc))

    def ack(self, cmd, status, value):
        self.send(header(FRAME_ID_ACK, self.tick(), value, 0, F_ACK, fft_size=cmd, peak_bin=status))

    def geometry(self):
        if self.rise_us and self.fall_us:
            rise, fall = self.rise_us, self.fall_us
        else:
            per = 1_000_000 / self.freq
            rise = fall = per / 2
        per_us = rise + fall
        freq_actual = int(round(1_000_000 / per_us))
        return int(rise), int(fall), freq_actual

    def status(self):
        rise, fall, _ = (self.rise_us, self.fall_us, 0)
        res = struct.pack("<HHBBH", self.amp, self.burst, 0, 0, rise)
        hdr = header(FRAME_ID_STATUS, self.tick(), self.samples_ovr, self.freq, F_STATUS,
                     fft_size=self.mode, peak_bin=self.mask, peak_mag=float(self.interval),
                     res=float(fall), reserved0=(1 if self.trace_on else 0) | 2, reserved=res)
        self.send(hdr)

    def capture(self, trig):
        rise, fall, f_act = self.geometry()
        rise_n = rise * FS // 1_000_000
        per_n  = (rise + fall) * FS // 1_000_000
        burst_n = per_n * self.burst
        target = -(-burst_n // CHUNK) * CHUNK
        target = min(target, (650000 // CHUNK) * CHUNK)
        n = self.samples_ovr if self.samples_ovr else target
        n = min(n, target)
        ts = self.tick()
        tr = []
        t = self.us()
        def ev(e, a=0, b=0, c=0, ctx=0, dt=5):
            nonlocal t
            t += dt
            tr.append(struct.pack("<IIHBBIII", t & 0xFFFFFFFF, ts + (t - tr_t0) // 1000 if tr else ts,
                                  e, ctx, len(tr) & 0xFF, a, b, c))
        tr_t0 = t
        # timeline
        ev(1, self.frame_id + 1, self.mode, trig)
        ev(2, self.freq, f_act * 1000, 64, dt=40)
        ev(3, 8188, 4094, self.amp, dt=30)
        ev(4, target, self.burst, 8188, dt=10)
        ev(5, target, CHUNK, 0x4D07, dt=60)
        ev(26, target // 60000 + 530, target, 1 if (self.freq == 458 and not self.rise_us) else 0, dt=5)
        ev(6, 63, target, 8188, ctx=1, dt=30000)          # VSYNC after 30 ms
        chunks = target // CHUNK
        for k in range(1, chunks + 1):
            if k <= 4 or k % 8 == 0 or k == chunks:
                ev(9, k, k * CHUNK, 0, ctx=1, dt=273 if k == 1 else 273 * (1 if k <= 5 else 8))
                ev(10, k * CHUNK, 160 * k, 1 if k == chunks else 0, ctx=1, dt=160)
        ev(8, 1, per_n, 1234, ctx=1, dt=50)
        ev(25, target, 1, 1, ctx=1, dt=2)
        ev(11, target, target // 60000 + 1, target, dt=120)
        ev(14, chunks, 0, 1, dt=3)
        ev(15, 4096, target, 1, dt=10)
        ev(16, 68, struct.unpack("<I", struct.pack("<f", 12345.6))[0], 1450, dt=1450)
        raw_bytes = n * 2
        ev(17, raw_bytes, 0, self.frame_id + 1, dt=20)
        # synth signal: 12-bit offset binary, UP beat 996 kHz, DOWN beat 1.2 MHz
        smp = bytearray(n * 2)
        A = 900.0 * self.amp / 4095.0
        for i in range(n):
            if i < rise_n:
                v = 2048 + A * math.sin(2 * math.pi * 996_000 * i / FS)
            elif i < per_n:
                v = 2048 + A * math.sin(2 * math.pi * 1_200_000 * i / FS)
            else:
                v = 2048
            v += random.gauss(0, 12)
            v = int(max(0, min(4095, v)))
            struct.pack_into("<H", smp, i * 2, v)
        res = struct.pack("<HHHH", self.amp, self.burst, min(rise, 65535), min(fall, 65535))
        self.frame_id += 1
        hdr = header(self.frame_id, ts, n, f_act, F_RAW, raw_bytes=raw_bytes, reserved=res)
        tx0 = time.time()
        self.send(hdr, bytes(smp))
        tx_ms = int((time.time() - tx0) * 1000)
        ev(18, 0xDEADBEEF, tx_ms, 0, dt=tx_ms * 1000)
        ev(20, t - tr_t0, self.frame_id, 0, dt=8)
        print(f"  -> DATA frame #{self.frame_id}: {n} samples ({raw_bytes} B) rise/fall {rise}/{fall} us, tx {tx_ms} ms")
        if self.trace_on:
            self.trace = tr
            self.send_trace(ts, f_act, res)

    def send_trace(self, ts, f_act, res):
        if not self.trace: return
        payload = b"".join(self.trace)
        hdr = header(FRAME_ID_TRACE, ts, len(self.trace), f_act, F_TRACE, fft_size=24, peak_bin=0,
                     raw_bytes=len(payload), reserved=res, srate=1_000_000)
        self.send(hdr, payload)
        print(f"  -> TRACE frame: {len(self.trace)} records")

    def handle(self, pkt):
        cmd, a1, a2, a3, c8 = struct.unpack("<BHHHB", pkt)
        if crc8(pkt[:7]) != c8:
            print(f"  !! CRC8 mismatch cmd=0x{cmd:02X}"); self.ack(0, 1, 0); return
        name = CMD.get(cmd, "?")
        print(f"RX {name} (0x{cmd:02X}) a1={a1} a2={a2}")
        if cmd == 0x03:
            self.send(header(FRAME_ID_PONG, self.tick(), 0, 0, 0)); return
        if cmd == 0x01:
            if 100 <= a1 <= 24000: self.freq = a1; self.rise_us = self.fall_us = 0; self.ack(cmd, 0, a1)
            else: self.ack(cmd, 1, self.freq)
        elif cmd in (0x02, 0x08):
            self.ack(cmd, 0, 1)
            if self.mode == 1: print("  (TRIGGER ignored: CONTINUOUS)")
            else: self.capture(1)
        elif cmd == 0x04: self.samples_ovr = a1 | (a2 << 16); self.ack(cmd, 0, self.samples_ovr)
        elif cmd == 0x05: self.mode = a1; self.ack(cmd, 0, a1); self.last_cycle = time.time()
        elif cmd == 0x06: self.mask = a1 & 3; self.ack(cmd, 0, self.mask)
        elif cmd == 0x07: self.interval = a1; self.ack(cmd, 0, a1)
        elif cmd == 0x09: self.status()
        elif cmd == 0x0A: self.amp = a1; self.ack(cmd, 0, a1)
        elif cmd == 0x0B: self.burst = a1; self.ack(cmd, 0, a1)
        elif cmd == 0x0C: self.ack(cmd, 0, 0)
        elif cmd == 0x0D: self.trace_on = bool(a1); self.ack(cmd, 0, a1)
        elif cmd == 0x0E:
            per = (a1 + a2) * 240
            if a1 >= 1 and a2 >= 1 and 10000 <= per <= 2_400_000:
                self.rise_us, self.fall_us = a1, a2; self.freq = 240_000_000 // per
                self.ack(cmd, 0, (a2 << 16) | a1)
            else: self.ack(cmd, 1, 0)
        elif cmd == 0x0F:
            self.ack(cmd, 0, len(self.trace)); self.send_trace(self.tick(), self.freq, b"\0"*8)
        else: self.ack(cmd, 1, 0)

def main():
    ser = serial.Serial(PORT, 921600, timeout=0.05)
    dev = Dev(ser)
    print(f"emulator on {PORT}; waiting for commands")
    buf = b""
    while True:
        data = ser.read(64)
        if data:
            buf += data
            while len(buf) >= 8:
                dev.handle(buf[:8]); buf = buf[8:]
        if dev.mode == 1 and (time.time() - dev.last_cycle) * 1000 >= dev.interval:
            dev.capture(2); dev.last_cycle = time.time()

if __name__ == "__main__":
    main()
