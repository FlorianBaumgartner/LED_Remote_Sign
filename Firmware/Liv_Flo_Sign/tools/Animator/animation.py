import numpy as np
import time


class Animation:
    def __init__(self, squares):
        Animation.squares = squares
        Animation.canvas_center_x = 64.35
        Animation.canvas_center_y = 70.5
        Animation.trigger = False
        Animation.radius = -1        # mm


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
        speed = 0.05
        wavelength_mm = 80  # Wavelength for the cosine wave
        high_color = [0xFF, 0x00, 0x00]  # High color (Red)
        low_color = [0xFF, 0xA0, 0x00]   # Low color (Black)

        # Go through all squares, calculate the color based on the cosine of the x position
        for i in range(len(color)):
            x = Animation.squares[i]['PosX'] - Animation.canvas_center_x
            val = np.cos((-np.abs(x) / wavelength_mm) * 2 * np.pi + framecount * speed)
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
        speed = 0.03            # one ramp per n frames
        number_of_groups = 4
        high_color = [0xFF, 0xFF, 0xFF] 
        low_color = [0xFF, 0xA0, 0x00] 

        for i in range(len(color)):
            group = i % number_of_groups
            phaseshift = (1.0 / number_of_groups) * group

            # \left|\operatorname{mod}\left(\left|\frac{x-p}{3}\right|,2\right)-1\right|
            val = np.abs(np.mod(np.abs(i - framecount * speed - phaseshift), 2) - 1)
            color[i] = [
                int(Animation.map(val, 0, 1, low_color[0], high_color[0])),
                int(Animation.map(val, 0, 1, low_color[1], high_color[1])),
                int(Animation.map(val, 0, 1, low_color[2], high_color[2]))
            ]
        return color


    @staticmethod
    def slow_sine(framecount, color, trigger):
        speed = 5            # one ramp per n frames
        number_of_groups = 3
        high_color = [0xFF, 0x00, 0xFF]
        low_color = [0xFF, 0xA0, 0x00]

        for i in range(len(color)):
            group = i % number_of_groups
            val = np.sin((framecount + i * speed) * 0.05)
            color[i] = [
                int(Animation.map(val, -1, 1, low_color[0], high_color[0])),
                int(Animation.map(val, -1, 1, low_color[1], high_color[1])),
                int(Animation.map(val, -1, 1, low_color[2], high_color[2]))
            ]
        return color