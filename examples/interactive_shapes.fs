# interactive_shapes.fs
# An example showing functions and state management for drawing.

import gui
import furi

# Define the application's state. This is where we will store
# all the variables that need to persist between frames.
class AppState:
    x = 25
    y = 25
    # 0 for box, 1 for circle
    shape_mode = 0
    is_filled = False
    is_running = True

# This is a user-defined function for drawing a box.
# It checks the app's state to decide whether to draw a
# filled box or just an outline.
def draw_my_box(canvas, app):
    if app.is_filled:
        # NOTE: For this to work, you must add a binding for
        # 'canvas_draw_box' in your parser.c file.
        # Example: {"display_draw_box", "canvas_draw_box", 5}
        canvas_draw_box(canvas, app.x, app.y, 30, 15)
    else:
        # This will work with your current compiler.
        canvas_draw_frame(canvas, app.x, app.y, 30, 15)

# This is a user-defined function for drawing a circle.
def draw_my_circle(canvas, app):
    if app.is_filled:
        # NOTE: For this to work, you must add a binding for
        canvas_draw_disc(canvas, app.x, app.y, 10)
    else:
        # NOTE: For this to work, you must add a binding for
        canvas_draw_circle(canvas, app.x, app.y, 10)

# The render function is called on every frame to draw to the screen.
def render(canvas, app):
    canvas_clear(canvas)
    canvas_draw_str(canvas, 2, 12, "Shape Drawer")
    
    # Decide which shape to draw based on the app's state.
    if app.shape_mode == 0:
        draw_my_box(canvas, app)
    else:
        draw_my_circle(canvas, app)

    # Display instructions
    canvas_draw_str(canvas, 2, 60, "L/R: Toggle Fill | U/D: Shape")

# The input function is called when a button is pressed.
def input(key, type, app):
    if type == InputTypePress:
        if key == InputKeyUp:
            app.shape_mode = 1
        elif key == InputKeyDown:
            app.shape_mode = 0
        elif key == InputKeyLeft:
            app.is_filled = False
        elif key == InputKeyRight:
            app.is_filled = True

# The main function is called repeatedly in the application loop.
# We don't need to do anything here for this example.
def main(app):
    app.is_running = True
    # This function is required by the template but can be empty.
    delay_ms(50)

# This code runs once at the start of the application.
print("FlipScript Shape Drawer Initialized")

