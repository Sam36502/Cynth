#include "button.h"
#include <stdio.h>

static Button *__g_buttons[MAX_BUTTON];
static int __g_buttoncount = 0;

//  Returns the ID of the created Button.
//  Leave either bool_var or int_var as null
//  if you won't use them
int New_Button(
    SDL_Rect rect,
    bool is_toggleable,
    void (* callback_on)(void *udata),
    void (* callback_off)(void *udata),
    void *udata,
    SDL_Colour clr_off, SDL_Colour clr_held, SDL_Colour clr_on
) {
    Button *b = malloc(sizeof(Button));
    b->rect = rect;
    b->callback_on = callback_on;
    b->callback_off = callback_off;
    b->udata = udata;
    b->is_toggle = is_toggleable;
    b->clr_off = clr_off;
    b->clr_held = clr_held;
    b->clr_on = clr_on;

    b->is_on = false;
    b->is_held = false;

    __g_buttons[__g_buttoncount] = b;
    return __g_buttoncount++;
}

Button *Get_Button(int id) {
    return __g_buttons[id];
}

void Destroy_Buttons() {
    for (int i=0; i<__g_buttoncount; i++) {
        Button *b = __g_buttons[i];
        free(b);
    }
}

void Draw_Buttons(SDL_Renderer *renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i=0; i<__g_buttoncount; i++) {
        Button *b = __g_buttons[i];

        if (b->is_held) {
            SDL_SetRenderDrawColor(renderer,
                b->clr_held.r,
                b->clr_held.g,
                b->clr_held.b,
                b->clr_held.a
            );
        } else if (b->is_on) {
            SDL_SetRenderDrawColor(renderer,
                b->clr_on.r,
                b->clr_on.g,
                b->clr_on.b,
                b->clr_on.a
            );
        } else {
            SDL_SetRenderDrawColor(renderer,
                b->clr_off.r,
                b->clr_off.g,
                b->clr_off.b,
                b->clr_off.a
            );
        }

        SDL_RenderFillRect(renderer, &b->rect);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Buttons_HandleMouseButtonEvent(SDL_MouseButtonEvent event) {
    if (event.button != SDL_BUTTON_LEFT) return;

    for (int i=__g_buttoncount-1; i>=0; i--) {
        Button *b = __g_buttons[i];

        if (
            event.x >= b->rect.x
            && event.x <= b->rect.x+b->rect.w
            && event.y >= b->rect.y
            && event.y <= b->rect.y+b->rect.h
        ) {

            // Mouse clicked on button
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                b->is_held = true;

                if (b->is_toggle) {
                    b->is_on = !b->is_on;

                    if (b->callback_on != NULL && b->is_on) (*b->callback_on)(b->udata);
                    if (b->callback_off != NULL && !b->is_on) (*b->callback_off)(b->udata);
                } else {
                    b->is_on = true;
                    if (b->callback_on != NULL) (*b->callback_on)(b->udata);
                }
            
            // Mouse button lifted on button
            } else {
                b->is_held = false;

                if (!b->is_toggle && b->is_on) {
                    b->is_on = false;
                    if (b->callback_off != NULL) (*b->callback_off)(b->udata);
                }
            }

            return;
        }
    }
}

void Buttons_HandleMouseMotionEvent(SDL_MouseMotionEvent event) {
    for (int i=0; i<__g_buttoncount; i++) {
        Button *b = __g_buttons[i];

        if (b->is_held) {
            if (
                event.x < b->rect.x
                || event.x > b->rect.x+b->rect.w
                || event.y < b->rect.y
                || event.y > b->rect.y+b->rect.h
            ) {
                b->is_held = false;
                if (!b->is_toggle) {
                    if (b->callback_off != NULL) (*b->callback_off)(b->udata);
                    b->is_on = false;
                }
            }
        }
    }
}