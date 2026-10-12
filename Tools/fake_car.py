"""Carro simulado por UDP: se comporta como el firmware AutoDcController.

Sirve para probar Unity sin el ESP32. En Unity se usa la IP 127.0.0.1.

    python Tools/fake_car.py
    python Tools/fake_car.py --mode differential --max-steer 25
    python Tools/fake_car.py --distance-mm 150     # obstaculo cercano
    python Tools/fake_car.py --distance-mm 65535   # sin lectura del HC-SR04
"""

import argparse
import select
import socket
import struct
import time

PACKET_MAGIC = 0xAD
PACKET_TYPE_DRIVE = 0x01
PACKET_TYPE_TELEMETRY = 0x81
DRIVE_PACKET_SIZE = 9
DRIVE_FLAG_BRAKE = 0x01
TELEMETRY_FLAG_FAILSAFE = 0x01
COMMAND_LIMIT = 100
STEERING_MODES = {"servo": 0, "differential": 1}

CRC8_POLYNOMIAL = 0x07
CRC8_HIGH_BIT = 0x80
BYTE_MASK = 0xFF
BITS_PER_BYTE = 8
SEQUENCE_MASK = 0xFFFF
SEQUENCE_HALF_RANGE = 0x8000
UINT32_MASK = 0xFFFFFFFF
MAX_DATAGRAM_SIZE = 64
MILLISECONDS_PER_SECOND = 1000

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 4210
MIN_PORT = 1
MAX_PORT = 65535
DEFAULT_MAX_STEER_DEG = 30
MAX_STEER_LIMIT_DEG = 90
FAILSAFE_TIMEOUT_S = 0.3
TELEMETRY_PERIOD_S = 0.1
POLL_TIMEOUT_S = 0.005
STATUS_PERIOD_S = 0.5
MAX_ACK_AGE_MS = 255
DISTANCE_UNKNOWN_MM = 0xFFFF
DEFAULT_DISTANCE_MM = 800

DRIVE_FORMAT = "<BBHbbbBB"
TELEMETRY_BODY_FORMAT = "<BBHbbBBBIHB"

def crc8(data: bytes) -> int:
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(BITS_PER_BYTE):
            crc = ((crc << 1) ^ CRC8_POLYNOMIAL) & BYTE_MASK if crc & CRC8_HIGH_BIT else (crc << 1) & BYTE_MASK
    return crc

def parse_drive_command(datagram: bytes):
    if len(datagram) != DRIVE_PACKET_SIZE or crc8(datagram[:-1]) != datagram[-1]:
        return None
    magic, packet_type, sequence, throttle, steering, trim, flags, _ = struct.unpack(DRIVE_FORMAT, datagram)
    if magic != PACKET_MAGIC or packet_type != PACKET_TYPE_DRIVE:
        return None
    if abs(throttle) > COMMAND_LIMIT or abs(steering) > COMMAND_LIMIT:
        return None
    return {"sequence": sequence, "throttle": throttle, "steering": steering,
            "trim": trim, "brake": bool(flags & DRIVE_FLAG_BRAKE)}

def is_newer_sequence(candidate: int, reference: int) -> bool:
    difference = (candidate - reference) & SEQUENCE_MASK
    return 0 < difference < SEQUENCE_HALF_RANGE

def encode_telemetry(command, mode: int, max_steer_deg: int, ack_age_ms: int, uptime_ms: int,
                     distance_mm: int, failsafe: bool) -> bytes:
    throttle = 0 if failsafe or command["brake"] else command["throttle"]
    steering = 0 if failsafe else command["steering"]
    body = struct.pack(TELEMETRY_BODY_FORMAT, PACKET_MAGIC, PACKET_TYPE_TELEMETRY, command["sequence"],
                       throttle, steering, mode, max_steer_deg, min(ack_age_ms, MAX_ACK_AGE_MS),
                       uptime_ms & UINT32_MASK, distance_mm, TELEMETRY_FLAG_FAILSAFE if failsafe else 0)
    return body + bytes([crc8(body)])

def to_milliseconds(seconds: float) -> int:
    return int(seconds * MILLISECONDS_PER_SECOND)

class FakeCar:
    def __init__(self, sock: socket.socket, mode: int, max_steer_deg: int, distance_mm: int):
        self.sock = sock
        self.mode = mode
        self.max_steer_deg = max_steer_deg
        self.distance_mm = distance_mm
        self.start = time.monotonic()
        self.command = {"sequence": 0, "throttle": 0, "steering": 0, "trim": 0, "brake": False}
        self.client = None
        self.failsafe = True
        self.last_command_at = self.start
        self.last_telemetry_at = self.start
        self.last_status_at = self.start
        self.rejected = 0

    def step(self) -> None:
        readable, _, _ = select.select([self.sock], [], [], POLL_TIMEOUT_S)
        now = time.monotonic()
        if readable:
            self.receive_command(now)
        self.update_failsafe(now)
        self.send_telemetry_if_due(now)
        self.print_status_if_due(now)

    def receive_command(self, now: float) -> None:
        try:
            datagram, sender = self.sock.recvfrom(MAX_DATAGRAM_SIZE)
        except ConnectionResetError:
            return
        parsed = parse_drive_command(datagram)
        if parsed is None:
            self.rejected += 1
        elif self.failsafe or is_newer_sequence(parsed["sequence"], self.command["sequence"]):
            self.command, self.client, self.failsafe, self.last_command_at = parsed, sender, False, now

    def update_failsafe(self, now: float) -> None:
        if not self.failsafe and now - self.last_command_at > FAILSAFE_TIMEOUT_S:
            self.failsafe = True
            print("\nFailsafe: sin comandos validos, motores detenidos.")

    def send_telemetry_if_due(self, now: float) -> None:
        if self.client is None or now - self.last_telemetry_at < TELEMETRY_PERIOD_S:
            return
        self.last_telemetry_at = now
        packet = encode_telemetry(self.command, self.mode, self.max_steer_deg,
                                  to_milliseconds(now - self.last_command_at), to_milliseconds(now - self.start),
                                  self.distance_mm, self.failsafe)
        try:
            self.sock.sendto(packet, self.client)
        except OSError as error:
            print(f"\nNo se pudo enviar telemetria: {error}")

    def print_status_if_due(self, now: float) -> None:
        if now - self.last_status_at < STATUS_PERIOD_S:
            return
        self.last_status_at = now
        state = "FAILSAFE" if self.failsafe else "activo"
        print(f"\rseq={self.command['sequence']:5d} acel={self.command['throttle']:4d} "
              f"giro={self.command['steering']:4d} trim={self.command['trim']:3d} "
              f"freno={int(self.command['brake'])} {state:8s} rechazados={self.rejected}",
              end="", flush=True)

def open_socket(host: str, port: int) -> socket.socket:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((host, port))
    except OSError as error:
        sock.close()
        raise SystemExit(f"No se pudo abrir {host}:{port}: {error}")
    sock.setblocking(False)
    return sock

def bounded_int(minimum: int, maximum: int):
    def parse(text: str) -> int:
        try:
            value = int(text)
        except ValueError:
            raise argparse.ArgumentTypeError(f"'{text}' no es un numero entero")
        if not minimum <= value <= maximum:
            raise argparse.ArgumentTypeError(f"{value} esta fuera del rango {minimum} a {maximum}")
        return value
    return parse

def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=bounded_int(MIN_PORT, MAX_PORT), default=DEFAULT_PORT)
    parser.add_argument("--mode", choices=sorted(STEERING_MODES), default="servo")
    parser.add_argument("--max-steer", type=bounded_int(0, MAX_STEER_LIMIT_DEG), default=DEFAULT_MAX_STEER_DEG,
                        help="angulo maximo del servo en grados")
    parser.add_argument("--distance-mm", type=bounded_int(0, DISTANCE_UNKNOWN_MM), default=DEFAULT_DISTANCE_MM,
                        help=f"distancia simulada del HC-SR04 en mm; {DISTANCE_UNKNOWN_MM} = sin lectura")
    return parser.parse_args()

def main() -> None:
    arguments = parse_arguments()
    sock = open_socket(arguments.host, arguments.port)
    car = FakeCar(sock, STEERING_MODES[arguments.mode], arguments.max_steer, arguments.distance_mm)
    print(f"Carro simulado escuchando en {arguments.host}:{arguments.port}. Ctrl+C para salir.")
    try:
        while True:
            car.step()
    except KeyboardInterrupt:
        print("\nCarro simulado detenido.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
