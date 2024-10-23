import sys
import numpy as np
import os
import tempfile
from PyQt5.QtWidgets import QApplication, QMainWindow, QWidget
from PyQt5.QtGui import QPainter, QColor, QPen, QBrush
from PyQt5.QtCore import Qt, QRectF, QTimer, QPoint
from cpl_loader import get_components
import time

# Global scaling factor: 1 mm = 3.779527559 pixels (based on 96 dpi, 1 inch = 25.4 mm)
MM_TO_PIXELS = 3.779527559
FRAME_RATE_MS = 1000 // 30  # 30 Hz (33ms per frame)

class Canvas(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Canvas with Scaling Factor")
        # Canvas size in mm (100mm height x 140mm width)
        self.canvas_width_mm = 140
        self.canvas_height_mm = 100
        self.corner_radius_mm = 5

        # Convert dimensions to pixels using the scaling factor
        self.canvas_width_px = self.canvas_width_mm * MM_TO_PIXELS
        self.canvas_height_px = self.canvas_height_mm * MM_TO_PIXELS
        self.corner_radius_px = self.corner_radius_mm * MM_TO_PIXELS

        # Set canvas background color to dark gray
        self.setFixedSize(int(self.canvas_width_px), int(self.canvas_height_px))

        # List to store squares and their attributes (position, color, and orientation)
        self.squares = []

    def add_square(self, x_mm, y_mm, color, rotation=0):
        square_data = {
            'x': x_mm * MM_TO_PIXELS,
            'y': self.canvas_height_mm * MM_TO_PIXELS - y_mm * MM_TO_PIXELS,  # Flip y-axis
            'color': QColor(color),
            'rotation': rotation
        }
        self.squares.append(square_data)
        self.update()  # Trigger a repaint

    def update_squares(self, color):
        """ Update square colors in the animation. """
        offset = len(self.squares) - len(color)
        for i, c in enumerate(color):
            self.squares[i + offset]['color'] = QColor(c[0], c[1], c[2])

    def paintEvent(self, event):
        painter = QPainter(self)

        # Enable anti-aliasing for smooth edges
        painter.setRenderHint(QPainter.Antialiasing)

        # Draw the rounded rectangle for the canvas
        painter.setPen(QPen(Qt.NoPen))
        painter.setBrush(QBrush(QColor(50, 50, 50)))  # Use dark gray color for the canvas background
        rect = QRectF(0, 0, self.canvas_width_px, self.canvas_height_px)
        painter.drawRoundedRect(rect, self.corner_radius_px, self.corner_radius_px)

        # Draw the squares with their respective colors and rotations
        for square in self.squares:
            painter.save()
            painter.translate(square['x'], square['y'])
            painter.rotate(square['rotation'])

            painter.setBrush(QBrush(square['color']))
            # Drawing the square (1mm x 1mm) converted to pixels
            square_size_px = 1 * MM_TO_PIXELS
            # Use QRectF to allow float precision
            painter.drawRect(QRectF(-square_size_px / 2, -square_size_px / 2, square_size_px, square_size_px))

            painter.restore()


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.canvas = Canvas()
        self.setCentralWidget(self.canvas)
        self.setWindowTitle("PyQt Canvas with Scaling and Squares")

        # Restore window position from temp file
        self.load_window_position()

        # Fix the window size to prevent resizing
        self.setFixedSize(int(self.canvas.canvas_width_px), int(self.canvas.canvas_height_px))

        # Load components from CPL file
        LED_Matrix, LED_Sign = get_components()

        # Set LED_Matrix squares to black (off)
        for component in LED_Matrix:
            self.canvas.add_square(component['PosX'], component['PosY'], "#000000", component['Rot'])

        # Create an array of colors for the LED sign, starting with black
        self.color = np.zeros((len(LED_Sign), 3), dtype=np.uint8)

        # Add squares for the LED sign
        for i, component in enumerate(LED_Sign):
            self.canvas.add_square(component['PosX'], component['PosY'], "#000000", component['Rot'])

        # Timer for 30 Hz update (animation)
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_frame)
        self.timer.start(FRAME_RATE_MS)

        self.index = 0

    def update_frame(self):
        self.color *= 0  # Reset all colors to black

        self.index += 1
        for i in range(len(self.color)):
            if i < self.index:
                self.color[i] = [0xFC, 0x54, 0x00]


        self.canvas.update_squares(self.color)
        self.canvas.update()  # Repaint the canvas

    def closeEvent(self, event):
        """ Save the window position on close. """
        self.save_window_position()
        event.accept()

    def save_window_position(self):
        """ Save the window position to a temp file. """
        temp_file = os.path.join(tempfile.gettempdir(), "window_position.txt")
        with open(temp_file, "w") as f:
            pos = self.pos()
            f.write(f"{pos.x()},{pos.y()}")

    def load_window_position(self):
        """ Load the window position from the temp file. """
        temp_file = os.path.join(tempfile.gettempdir(), "window_position.txt")
        if os.path.exists(temp_file):
            with open(temp_file, "r") as f:
                data = f.read().split(',')
                if len(data) == 2:
                    try:
                        x, y = int(data[0]), int(data[1])
                        self.move(x, y)
                    except ValueError:
                        pass  # If there's an issue with the file contents, ignore


def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
