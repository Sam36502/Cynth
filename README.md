# Cynth

*Pronounced "Synth" (/sɪnθ/)*

This is a basic library using pure C and SDL2 to generate
tones on the fly for basic 8-bit style music.

## Project Structure

The main library for sound generation is a single pair of c files
in the `lib` directory: `synth.h` & `synth.c`.
This library lets you create a synth object for managing multiple
single-oscillator "channels" with an ADSR envelope for each one,
that are then mixed and output via SDL's audio callback function.

The included extra libraries `button.c`, `midi.c` and `slider.c`, are
small utilities I threw together to test extra features with the synth.

# Demo Program

A basic demonstration of the synth's capabilities is included with the
`main.c` file, which is built and run when calling `make`.



**Note:** This project is still a work in progress!