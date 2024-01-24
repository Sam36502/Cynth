#ifndef BUTTON_C
#define BUTTON_C

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_BUTTON 256
#define SLIDER_WIDTH 4
#define KNOB_WIDTH 10
#define KNOB_LENGTH 30
#define SLIDER_COLOUR 0xAA, 0xAA, 0xAA, 0xFF
#define KNOB_COLOUR 0x11, 0x11, 0x11, 0xFF

typedef struct Button {
    SDL_Rect rect;
    void (* callback_on)(void *udata);
    void (* callback_off)(void *udata);
    void *udata;
    bool is_toggle;
    bool is_held;
    bool is_on;
    SDL_Colour clr_off;
    SDL_Colour clr_on;
    SDL_Colour clr_held;
} Button;

//
//  Functions
//

//  Returns the ID of the created Button.
int New_Button(
    SDL_Rect rect,
    bool is_toggleable,
    void (* callback_on)(void *udata),
    void (* callback_off)(void *udata),
    void *udata,
    SDL_Colour clr_off, SDL_Colour clr_held, SDL_Colour clr_on
);

Button *Get_Button(int id);

void Destroy_Buttons();

void Draw_Buttons(SDL_Renderer *renderer);

void Buttons_HandleMouseButtonEvent(SDL_MouseButtonEvent event);

void Buttons_HandleMouseMotionEvent(SDL_MouseMotionEvent event);

#endif