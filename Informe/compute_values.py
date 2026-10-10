import math
from pathlib import Path

MASS_KG = 0.317
WHEEL_RADIUS_M = 0.024
BATTERY_NO_LOAD_MEASURED_V = 7.5
BATTERY_LOAD_MEASURED_V = 6.9

ROLLING_COEFFICIENT = 0.02
GRAVITY = 9.81
DRIVEN_WHEELS = 2

GEAR_RATIO = 50
NOMINAL_VOLTAGE = 6.0
NO_LOAD_RPM = 500.0
NO_LOAD_CURRENT_A = 0.07
RATED_RPM = 400.0
RATED_CURRENT_A = 0.15
RATED_TORQUE_KGF_CM = 0.5
STALL_CURRENT_A = 0.55
STALL_TORQUE_KGF_CM = 2.0

BATTERY_NOMINAL_V = 7.4
BATTERY_FULL_V = 8.4
EFFECTIVE_VOLTAGE_LIMIT_V = 5.0
BRIDGE_CONTINUOUS_A = 2.0
BRIDGE_REPETITIVE_PEAK_A = 2.5

REPORT_FILE = Path(__file__).with_name("informe_carro.tex")
BEGIN_MARKER = "% BEGIN VALUES (generado por compute_values.py; no editar a mano)"
END_MARKER = "% END VALUES"

def rpm_to_rad_s(rpm: float) -> float:
    return rpm * 2.0 * math.pi / 60.0

def compute_values() -> dict:
    rolling_force_n = ROLLING_COEFFICIENT * MASS_KG * GRAVITY
    required_torque_nm = rolling_force_n / DRIVEN_WHEELS * WHEEL_RADIUS_M
    required_torque_kgf_cm = required_torque_nm / GRAVITY * 100.0

    no_load_rad_s = rpm_to_rad_s(NO_LOAD_RPM)
    rated_rad_s = rpm_to_rad_s(RATED_RPM)
    duty_adopted = EFFECTIVE_VOLTAGE_LIMIT_V / BATTERY_NOMINAL_V
    armature_resistance = NOMINAL_VOLTAGE / STALL_CURRENT_A
    stall_full_a = BATTERY_FULL_V / armature_resistance
    stall_limited_a = EFFECTIVE_VOLTAGE_LIMIT_V / armature_resistance
    no_load_error_v = abs(BATTERY_NO_LOAD_MEASURED_V - BATTERY_NOMINAL_V)
    load_error_v = abs(BATTERY_LOAD_MEASURED_V - BATTERY_NOMINAL_V)

    return {
        "massKg": MASS_KG,
        "wheelRadiusM": WHEEL_RADIUS_M,
        "wheelRadiusCm": WHEEL_RADIUS_M * 100.0,
        "rollingForceN": rolling_force_n,
        "requiredTorqueNm": required_torque_nm,
        "requiredTorqueKgfCm": required_torque_kgf_cm,
        "safetyRated": RATED_TORQUE_KGF_CM / required_torque_kgf_cm,
        "safetyStall": STALL_TORQUE_KGF_CM / required_torque_kgf_cm,
        "noLoadRadS": no_load_rad_s,
        "motorShaftRpm": NO_LOAD_RPM * GEAR_RATIO,
        "motorShaftRadS": no_load_rad_s * GEAR_RATIO,
        "speedNoLoad": no_load_rad_s * WHEEL_RADIUS_M,
        "speedRated": rated_rad_s * WHEEL_RADIUS_M,
        "speedLimited": no_load_rad_s * WHEEL_RADIUS_M * EFFECTIVE_VOLTAGE_LIMIT_V / NOMINAL_VOLTAGE,
        "dutyFullCharge": NOMINAL_VOLTAGE / BATTERY_FULL_V,
        "dutyAdopted": duty_adopted,
        "effectiveAtFull": duty_adopted * BATTERY_FULL_V,
        "overvoltageNominalPct": (BATTERY_NOMINAL_V / NOMINAL_VOLTAGE - 1.0) * 100.0,
        "overvoltageFullPct": (BATTERY_FULL_V / NOMINAL_VOLTAGE - 1.0) * 100.0,
        "armatureResistance": armature_resistance,
        "stallFullA": stall_full_a,
        "stallLimitedA": stall_limited_a,
        "stallTotalFullA": stall_full_a * DRIVEN_WHEELS,
        "noLoadUsePct": NO_LOAD_CURRENT_A / BRIDGE_CONTINUOUS_A * 100.0,
        "ratedUsePct": RATED_CURRENT_A / BRIDGE_CONTINUOUS_A * 100.0,
        "stallUsePct": STALL_CURRENT_A / BRIDGE_CONTINUOUS_A * 100.0,
        "stallFullUsePct": stall_full_a / BRIDGE_CONTINUOUS_A * 100.0,
        "stallLimitedUsePct": stall_limited_a / BRIDGE_CONTINUOUS_A * 100.0,
        "batteryNoLoadMeasured": BATTERY_NO_LOAD_MEASURED_V,
        "batteryLoadMeasured": BATTERY_LOAD_MEASURED_V,
        "batteryNoLoadAbsError": no_load_error_v,
        "batteryNoLoadRelErrorPct": no_load_error_v / BATTERY_NOMINAL_V * 100.0,
        "batteryLoadAbsError": load_error_v,
        "batteryLoadRelErrorPct": load_error_v / BATTERY_NOMINAL_V * 100.0,
    }

def format_value(value: float) -> str:
    if abs(value) < 0.01:
        return f"{value:.3e}"
    if abs(value) >= 10000:
        return f"{value:.0f}"
    return f"{value:.4g}"

def write_macros(values: dict) -> None:
    lines = [BEGIN_MARKER]
    for name, value in values.items():
        lines.append(f"\\newcommand{{\\val{name[0].upper()}{name[1:]}}}{{{format_value(value)}}}")
    lines.append(END_MARKER)
    text = REPORT_FILE.read_text(encoding="utf-8")
    start = text.index(BEGIN_MARKER)
    end = text.index(END_MARKER) + len(END_MARKER)
    REPORT_FILE.write_text(text[:start] + "\n".join(lines) + text[end:], encoding="utf-8")

if __name__ == "__main__":
    computed = compute_values()
    write_macros(computed)
    for key, number in computed.items():
        print(f"{key:24s} {format_value(number)}")
