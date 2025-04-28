import numpy as np

def degps_to_raw(degps: float) -> int:
    steps_per_deg = 4096.0 / 360.0
    speed_in_steps = degps * steps_per_deg
    speed_int = int(round(speed_in_steps))
    if speed_int > 0x7FFF:
            speed_int = 0x7FFF
    return speed_int

def body_to_wheel_raw(
    self,
    x_cmd: float,
    y_cmd: float,
    theta_cmd: float,
    wheel_radius: float = 0.05,
    base_radius: float = 0.125,
    max_raw: int = 3000,
) -> dict:
    """
    Convert desired body-frame velocities into wheel raw commands.

    Parameters:
        x_cmd      : Linear velocity in x (m/s).
        y_cmd      : Linear velocity in y (m/s).
        theta_cmd  : Rotational velocity (deg/s).
        wheel_radius: Radius of each wheel (meters).
        base_radius : Distance from the center of rotation to each wheel (meters).
        max_raw    : Maximum allowed raw command (ticks) per wheel.

    Returns:
        A dictionary with wheel raw commands:
            {"left_wheel": value, "back_wheel": value, "right_wheel": value}.

    Notes:
        - Internally, the method converts theta_cmd to rad/s for the kinematics.
        - The raw command is computed from the wheels angular speed in deg/s
        using degps_to_raw(). If any command exceeds max_raw, all commands
        are scaled down proportionally.
    """
    # Convert rotational velocity from deg/s to rad/s.
    theta_rad = theta_cmd * (np.pi / 180.0)
    # Create the body velocity vector [x, y, theta_rad].
    velocity_vector = np.array([x_cmd, y_cmd, theta_rad])

    # Define the wheel mounting angles (defined from y axis cw)
    angles = np.radians(np.array([300, 180, 60]))
    # Build the kinematic matrix: each row maps body velocities to a wheel’s linear speed.
    # The third column (base_radius) accounts for the effect of rotation.
    m = np.array([[np.cos(a), np.sin(a), base_radius] for a in angles])

    # Compute each wheel’s linear speed (m/s) and then its angular speed (rad/s).
    wheel_linear_speeds = m.dot(velocity_vector)
    wheel_angular_speeds = wheel_linear_speeds / wheel_radius

    # Convert wheel angular speeds from rad/s to deg/s.
    wheel_degps = wheel_angular_speeds * (180.0 / np.pi)

    # Scaling
    steps_per_deg = 4096.0 / 360.0
    # 每旋转一度，电机编码器会产生多少个原始计数
    raw_floats = [abs(degps) * steps_per_deg for degps in wheel_degps]
    max_raw_computed = max(raw_floats)
    if max_raw_computed > max_raw:
        scale = max_raw / max_raw_computed
        wheel_degps = wheel_degps * scale

    # arduino 可以直接用负数
    # Convert each wheel’s angular speed (deg/s) to a raw integer.
    wheel_raw = [degps_to_raw(deg) for deg in wheel_degps]

    return {"left_wheel": wheel_raw[0], "back_wheel": wheel_raw[1], "right_wheel": wheel_raw[2]}


if __name__ == "__main__":
    # Example usage
    x_cmd = 0.0  # m/s
    y_cmd = 0.0  # m/s
    theta_cmd = 1  # deg/s
    wheel_radius = 0.05  # meters
    base_radius = 0.125  # meters

    result = body_to_wheel_raw(x_cmd, y_cmd, theta_cmd, wheel_radius, base_radius)
    print(result)