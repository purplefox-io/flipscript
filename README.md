# FlipScript: Rapid Prototyping for Flipper Zero in a Python-like Language

## What is FlipScript?

FlipScript is a high-level, compiled programming language designed to make creating applications for the Flipper Zero faster and more intuitive. It uses a clean, Python-like syntax that transpiles directly into a complete, self-contained Flipper Zero application source file in C.

The goal of FlipScript is to lower the barrier to entry for Flipper Zero development, allowing hobbyists, students, and developers to rapidly prototype ideas without needing to write extensive C boilerplate code. You can focus on your application's logic, and FlipScript handles the rest.

### How it Works

FlipScript uses a custom-built compiler with a classic pipeline:

1. **Lexer**: Scans your `.fs` script and breaks it down into a stream of tokens (keywords, identifiers, numbers, etc.).

2. **Parser**: Takes the token stream and builds an Abstract Syntax Tree (AST), which is a structured representation of your code's logic. It also automatically injects bindings to native Flipper SDK functions when it sees an `import` statement.

3. **Code Generator**: Traverses the AST and translates it into a fully-functional C source file (`.c`), complete with the required Flipper application entry point (`app_main`), event loop, and callback functions.

## Current Status: In Development

FlipScript is currently in the early stages of development. The core compiler is functional, and it successfully supports the creation of basic GUI applications using the `render`, `input`, and `main` function structure. However, it should be considered **alpha software**.

## Future Development & Roadmap

The vision for FlipScript is to provide a comprehensive and easy-to-use interface for all of the Flipper Zero's powerful hardware capabilities. Future releases will focus on adding support for:

* **GPIO Control**: Directly manipulate the Flipper's GPIO pins for custom hardware projects.

* **Sub-GHz Radio**: Add functions to simplify sending and receiving signals with the radio module.

* **NFC & Infrared**: Create high-level bindings for reading/writing NFC tags and transmitting infrared signals.

* **An Expanded Standard Library**: Introduce more built-in functions for common tasks and data manipulation.

* **Improved Error Handling**: Provide more descriptive error messages during compilation.

## Supported Functions

When you use the `import` statement in FlipScript, the compiler automatically makes a set of native Flipper SDK functions available to you. Here is the complete list of currently supported functions:

| FlipScript Function | Native Flipper SDK Function | Description | 
|:---|:---|:---|
| `canvas_clear()` | `canvas_clear()` | Clears the entire screen. | 
| `canvas_draw_str()` | `canvas_draw_str()` | Draws a string of text at a specific (x, y) coordinate. | 
| `canvas_draw_frame()` | `canvas_draw_frame()` | Draws the outline of a box (a frame) at a given position and size. | 
| `canvas_draw_box()` | `canvas_draw_box()` | Draws a solid, filled rectangle at a given position and size. | 
| `canvas_draw_circle()` | `canvas_draw_circle()` | Draws the outline of a circle at a given center point and radius. | 
| `canvas_draw_disc()` | `canvas_draw_disc()` | Draws a solid, filled circle at a given center point and radius. | 
| `canvas_flush()` | `canvas_flush()` | (Not typically needed) Pushes the canvas buffer to the display hardware. | 
| `delay_ms()` | `furi_delay_ms()` | Pauses the execution of the script for a specified number of milliseconds. | 
| `notification_message()` | `notification_message()` | Triggers a built-in Flipper notification (e.g., LED flash, vibration). | 

## How to Compile and Run Your FlipScript App
Here is the complete workflow for turning your `.fs` file into a running Flipper Zero application.

### **Step** 1: Write Your FlipScript (.fs) File
Create a file, for example my_app.fs, and write your application logic using the Python-like syntax.

### **Step 2**: Compile FlipScript to C
Use your flipscript compiler to transpile the `.fs` file into a C source file. The `-c` flag enables C code generation, and `-o` specifies the output file name.

```bash
./flipscript -c -o output.c my_app.fs
```
After running this command, you will have a new file named `output.c` in your directory. This file contains the complete, self-contained Flipper application code.

### **Step 3: Configure the Flipper Build**
For the Flipper build system (ufbt) to know which file to compile for your application, you must specify it in the `application.fam` file. This is a critical step. If you don't do this, ufbt will try to compile all `.c` files in your project, which will cause errors.

Your application.fam should look like this:

```python
# Flipper Application Manifest
App(
    # The App ID is a unique name for your application.
    appid="my_flipscript_app",

    # The entry point for your app's main function.
    entry_point="app_main",

    # List of all C source files that make up your Flipper application.
    # By specifying only "output.c", you are telling the build system
    # to ignore all other .c files in the directory.
    sources=[
        "output.c",
    ],

    # The category under which your app will appear on the Flipper.
    fap_category="Misc",
)
```

### **Step 4: Build and Launch on Flipper**
With your `output.c` and `application.fam` files in place, you can now use the Flipper build tool (ufbt) to build the application and launch it on your device.

```bash
ufbt launch
```

This command will compile `output.c` into a `.fap` file and automatically deploy and run it on your connected Flipper Zero. You should now see your FlipScript application running on the device!

**Contributions are welcome!**
