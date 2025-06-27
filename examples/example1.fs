# Example FlipScript Code (example1.fs)

import gui
import furi

# Define app state structure
class AppState:
    x = 0
    y = 0
    counter = 0
    is_running = True

# Render function - called when the screen needs to be updated.
# Needed to use the canvas.
# This is where you put code if you want to use the display.
def render(canvas, app):
    # Clear the screen
    # FIX: Use the 'display_' prefix for GUI functions
    canvas_clear(canvas)
    
    # Draw a frame around the edge of the screen
    # FIX: Use the 'display_' prefix for GUI functions
    canvas_draw_frame(canvas, 0, 0, 128, 64)
    
    # Draw some text
    # FIX: Use the 'display_' prefix for GUI functions
    canvas_draw_str(canvas, 10, 10, "Hello from FlipScript!")
    
    # Draw the counter
    canvas_draw_str(canvas, 10, 30, "Counter: ")
    canvas_draw_str(canvas, 70, 30, app.counter)
    canvas_draw_str(canvas, 10, 40, "x: ")
    canvas_draw_str(canvas, 70, 40, app.x)
    canvas_draw_str(canvas, 10, 50, "y: ")
    canvas_draw_str(canvas, 70, 50, app.y)

# Input function - called when a button is pressed
def input(key, type, app):
    if type == InputTypePress:
        if key == InputKeyUp:
            app.y = app.y + 1
        elif key == InputKeyDown:
            app.y = app.y - 1
        elif key == InputKeyLeft:
            app.x = app.x - 1
        elif key == InputKeyRight:
            app.x = app.x + 1
        elif key == InputKeyOk:
            app.counter = app.counter + 1
        

# The main function is executed on every loop of the application
def main(app):
    app.is_running = True
    delay_ms(10)

# Initialize the app
print("Starting FlipperScript application")