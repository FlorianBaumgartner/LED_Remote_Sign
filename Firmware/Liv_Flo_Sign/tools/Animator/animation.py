import numpy as np
import time
import random


class Animation:
    def __init__(self, squares, framerate):
        Animation.squares = squares
        Animation.framerate = framerate
        Animation.canvas_center_x = 64.35
        Animation.canvas_center_y = 70.5
        Animation.canvas_min_x = min([square['PosX'] for square in squares])
        Animation.canvas_max_x = max([square['PosX'] for square in squares])
        Animation.canvas_min_y = min([square['PosY'] for square in squares])
        Animation.canvas_max_y = max([square['PosY'] for square in squares])
        Animation.trigger = False
        Animation.radius = -1        # mm

        # Print a cpp arrays for the coordinates of the squares
        print(f"const float square_coordinates[{len(squares)}][2] = {{")
        for square in squares:
            print(f"  {{{square['PosX']}, {square['PosY']}}},")
        print("};")

        # Print the center of the canvas as cpp float
        print(f"const float canvas_center[2] = {{{Animation.canvas_center_x}, {Animation.canvas_center_y}}};")
        print(f"const float canvas_min_max_x[2] = {{{Animation.canvas_min_x}, {Animation.canvas_max_x}}};")
        print(f"const float canvas_min_max_y[2] = {{{Animation.canvas_min_y}, {Animation.canvas_max_y}}};")

    def map(value, in_min, in_max, out_min, out_max):
        return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

    def basic(framecount, color):
        print(Animation.squares[0])
        index = framecount % len(color)
        color *= 0
        for i in range(len(color)):
            if i < index:
                color[i] = [0xFC, 0x54, 0x00]
        return color

        
    @staticmethod
    def sine(framecount, color, trigger):
        speed = 1
        wavelength = 1  # Wavelength
        high_color = [0xFF, 0x08, 0x08]  # High color
        low_color = [0xFF, 0xFF, 0x00]   # Low color

        angle_offset = (framecount * -speed) % 360
        for i in range(len(color)):
            x = Animation.squares[i]['PosX']
            if x <= Animation.canvas_center_x:
                relative_x = abs(x - Animation.canvas_min_x)
                total_range = Animation.canvas_center_x - Animation.canvas_min_x
                normalized_position = -relative_x / total_range
            else:
                relative_x = abs(x - Animation.canvas_center_x)
                total_range = Animation.canvas_max_x - Animation.canvas_center_x
                normalized_position = relative_x / total_range

            angle = (normalized_position * 360 / wavelength + angle_offset) % 360
            val = np.cos(np.radians(angle))
            color[i] = [
                int(Animation.map(val, -1, 1, low_color[0], high_color[0])),
                int(Animation.map(val, -1, 1, low_color[1], high_color[1])),
                int(Animation.map(val, -1, 1, low_color[2], high_color[2]))
            ]
        return color


    @staticmethod
    def trigger_test(framecount, color, trigger):
        radius_rate = 3             # mm/frame
        high_color = [0xFF, 0x00, 0xFF]  # High color (Red)
        low_color = [0xFF, 0xA0, 0x00]   # Low color (Black)

        if trigger != Animation.trigger:
            Animation.trigger = trigger
            if trigger:
                spawn_anim = True
                Animation.radius = 0

        if Animation.radius >= 0:
            Animation.radius += radius_rate
            if Animation.radius >= 140:
                Animation.radius = -1
        
        if Animation.radius >= 0:
            for i in range(len(color)):
                x = Animation.squares[i]['PosX'] - Animation.canvas_center_x 
                val = 1 - 0.003 * (Animation.radius - np.abs(x))**2
                if val < 0:
                    val = 0
                color[i] = [
                    int(Animation.map(val, 0, 1, low_color[0], high_color[0])),
                    int(Animation.map(val, 0, 1, low_color[1], high_color[1])),
                    int(Animation.map(val, 0, 1, low_color[2], high_color[2]))
                ]
        return color


    @staticmethod
    def sprinkle(framecount, color, trigger):
        speed = 0.005            # one ramp per n frames
        number_of_groups = 9
        high_color = [0xFF, 0xA0, 0x00] 

        for i in range(len(color)):
            val = np.abs(np.mod(np.abs(i / number_of_groups - framecount * speed), 2) - 1)
            val *= np.abs(np.mod(np.abs(i / (number_of_groups - 1) + framecount * speed), 2) - 1)
            color[i] = [
                int(val * high_color[0]),
                int(val * high_color[1]),
                int(val * high_color[2])
            ]
        return color


    @staticmethod
    def circles(framecount, color, trigger):
        high_color = [0xFF, 0x00, 0x00]  # Primary color (red)

        # Initialize the circle radius and velocity
        if hasattr(Animation, 'acceleration') is False:
            Animation.radius = -1       # mm
            trigger = True
            Animation.acceleration = 0.02  # Acceleration rate

        # Initialize a new random spawn position if the radius is reset or trigger is active
        if Animation.radius < 0 or trigger:
            Animation.spawn_x = random.uniform(Animation.canvas_min_x, Animation.canvas_max_x)
            Animation.spawn_y = random.uniform(Animation.canvas_min_y, Animation.canvas_max_y)
            Animation.radius = 0  # Start the circle propagation
            Animation.velocity = 0.2 # Reset velocity for the new circle

        # Update velocity with acceleration
        Animation.velocity += Animation.acceleration

        # Update radius using the current velocity
        Animation.radius += Animation.velocity

        # Define the gradient fall-off range (how far it should fall to zero)
        gradient_width = 25  # mm

        gradient_width *= Animation.velocity

        # Update colors for all LEDs
        for i in range(len(color)):
            x = Animation.squares[i]['PosX']
            y = Animation.squares[i]['PosY']
            distance = ((x - Animation.spawn_x)**2 + (y - Animation.spawn_y)**2)**0.5

            # Compute the brightness based on the distance from the current radius
            if abs(distance - Animation.radius) <= gradient_width:
                # Calculate the brightness factor
                brightness = max(0, 1 - abs(distance - Animation.radius) / gradient_width)
                color[i] = [
                    int(high_color[0] * brightness),
                    int(high_color[1] * brightness),
                    int(high_color[2] * brightness)
                ]
            else:
                # Outside the gradient range, set to low color
                color[i] = [0, 0, 0]

        # Reset radius when it exceeds the canvas size
        max_distance = max(
            Animation.canvas_max_x - Animation.canvas_min_x,
            Animation.canvas_max_y - Animation.canvas_min_y
        )
        if Animation.radius > max_distance + 200:
            Animation.radius = -1  # Reset radius to spawn a new circle

        return color



    @staticmethod
    def newMessage(framecount, color, trigger):
        speed = 1.75           # Higher value = slower movement; lower value = faster
        wait_time = 3         # Seconds to wait after the tail completes
        tail_length = 10       # Maximum number of LEDs in the tail
        decay_factor = 0.8    # Exponential decay factor for tail brightness
        high_color = [0xFF, 0xFF, 0xFF]  # High color for the tail
        frame_rate = 30       # Assume a frame rate (replace with Animation.framerate)
        
        # Total frames to complete one cycle (moving across + wait time)
        cycle_frames = len(color) / speed + int(wait_time * frame_rate)
        
        # Calculate position within the cycle
        position_in_cycle = int(framecount % cycle_frames * speed)
        
        # Iterate over LEDs in the color array
        for i in range(len(color)):
            if position_in_cycle < len(color):  # Moving phase
                if i == position_in_cycle:  # Current position of the tail
                    color[i] = high_color
                elif position_in_cycle - i > 0 and position_in_cycle - i <= tail_length:  # Tail section
                    decay = decay_factor ** (position_in_cycle - i - 1)  # Exponential decay
                    brightness = max(int(255 * decay), 0)  # Ensure non-negative values
                    color[i] = [brightness] * 3
                else:  # LEDs not yet reached by the tail
                    color[i] = [0, 0, 0]
            else:  # Waiting phase
                color[i] = [0, 0, 0]

        return color


