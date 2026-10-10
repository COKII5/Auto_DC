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

DEFAULT_PORT = 4210
FAILSAFE_TIMEOUT_S = 0.3
TELEMETRY_PERIOD_S = 0.1
POLL_TIMEOUT_S = 0.005
STATUS_PERIOD_S = 0.5
MAX_ACK_AGE_MS = 255
DISTANCE_UNKNOWN_MM = 0xFFFF
DEFAULT_DISTANCE_MM = 800
UINT32_MASK = 0xFFFFFFFF

DRIVE_FORMAT = "<BBHbbbBB"
TELEMETRY_BODY_FORMAT = "<BBHbbBBBIHB"

def crc8(data: bytes) -> int:
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
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
    difference = (candidate - reference) & 0xFFFF
    return 0 < difference < 0x8000

def encode_telemetry(command, mode: int, max_steer_deg: int, ack_age_ms: int, uptime_ms: int,
                     distance_mm: int, failsafe: bool) -> bytes:
    throttle = 0 if failsafe or command["brake"] else command["throttle"]
    steering = 0 if failsafe else command["steering"]
    body = struct.pack(TELEMETRY_BODY_FORMAT, PACKET_MAGIC, PACKET_TYPE_TELEMETRY, command["sequence"],
                       throttle, steering, mode, max_steer_deg, min(ack_age_ms, MAX_ACK_AGE_MS),
                       uptime_ms & UINT32_MASK, distance_mm, TELEMETRY_FLAG_FAILSAFE if failsafe else 0)
    return body + bytes([crc8(body)])

def run(host: str, port: int, mode: int, max_steer_deg: int, distance_mm: int) -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((host, port))
    except OSError as error:
        raise SystemExit(f"No se pudo abrir {host}:{port}: {error}")
    sock.setblocking(False)
    print(f"Carro simulado escuchando en {host}:{port}. Ctrl+C para salir.")

    start = time.monotonic()
    command = {"sequence": 0, "throttle": 0, "steering": 0, "trim": 0, "brake": False}
    client = None
    failsafe = True
    last_command_at = start
    last_telemetry_at = start
    last_status_at = start
    rejected = 0

    while True:
        readable, _, _ = select.select([sock], [], [], POLL_TIMEOUT_S)
        now = time.monotonic()
        if readable:
            try:
                datagram, sender = sock.recvfrom(64)
            except ConnectionResetError:
                continue
            parsed = parse_drive_command(datagram)
            if parsed is None:
                rejected += 1
            elif failsafe or is_newer_sequence(parsed["sequence"], command["sequence"]):
                command, client, failsafe, last_command_at = parsed, sender, False, now

        if not failsafe and now - last_command_at > FAILSAFE_TIMEOUT_S:
            failsafe = True
            print("\nFailsafe: sin comandos validos, motores detenidos.")

        if client is not None and now - last_telemetry_at >= TELEMETRY_PERIOD_S:
            last_telemetry_at = now
            ack_age_ms = int((now - last_command_at) * 1000)
            uptime_ms = int((now - start) * 1000)
            try:
                sock.sendto(encode_telemetry(command, mode, max_steer_deg, ack_age_ms, uptime_ms, distance_mm, failsafe),
                            client)
            except OSError as error:
                print(f"\nNo se pudo enviar telemetria: {error}")

        if now - last_status_at >= STATUS_PERIOD_S:
            last_status_at = now
            state = "FAILSAFE" if failsafe else "activo"
            print(f"\rseq={command['sequence']:5d} acel={command['throttle']:4d} giro={command['steering']:4d} "
                  f"trim={command['trim']:3d} freno={int(command['brake'])} {state:8s} rechazados={rejected}",
                  end="", flush=True)

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--mode", choices=sorted(STEERING_MODES), default="servo")
    parser.add_argument("--max-steer", type=int, default=30, help="angulo maximo del servo en grados")
    parser.add_argument("--distance-mm", type=int, default=DEFAULT_DISTANCE_MM,
                        help="distancia simulada del HC-SR04 en mm; 65535 = sin lectura")
    arguments = parser.parse_args()
    try:
        run(arguments.host, arguments.port, STEERING_MODES[arguments.mode], arguments.max_steer,
            arguments.distance_mm)
    except KeyboardInterrupt:
        print("\nCarro simulado detenido.")

if __name__ == "__main__":
    main()
