#ifndef SLIDER_C
#define SLIDER_C

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_SLIDERS 256
#define SLIDER_WIDTH 4
#define KNOB_WIDTH 10
#define KNOB_LENGTH 30
#define SLIDER_COLOUR 0xAA, 0xAA, 0xAA, 0xFF
#define KNOB_COLOUR 0x11, 0x11, 0x11, 0xFF

typedef enum SliderOrientation {
    SLIDER_HORIZONTAL,
    SLIDER_VERTICAL,
} SliderOrientation;

typedef struct Slider {
    int x; int y;
    int length;
    float factor;
    float *var;
    bool enabled;
    bool is_held;
    SliderOrientation orient;
} Slider;

//
//  Functions
//

// Returns the ID of the created Slider
int New_Slider(int x, int y, int length, SliderOrientation orient, float *var, float factor);

Slider *Get_Slider(int id);

void Destroy_Sliders();

SDL_Rect *__calc_rect_knob(Slider *s, SDL_Rect *r);

void Draw_Sliders(SDL_Renderer *renderer);

void Sliders_HandleMouseButtonEvent(SDL_MouseButtonEvent event);

void Sliders_HandleMouseMotionEvent(SDL_MouseMotionEvent event);

#endif