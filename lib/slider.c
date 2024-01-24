#include "slider.h"
/*
 *      Super-basic UI Slider implementation
 */

static Slider *__g_sliders[MAX_SLIDERS];
static unsigned int __g_slidercount = 0;

int New_Slider(int x, int y, int length, SliderOrientation orient, float *var, float factor) {
    Slider *s = malloc(sizeof(Slider));
    s->x = x;
    s->y = y;
    s->length = length;
    s->orient = orient;
    s->factor = factor;
    s->var = var;
    s->is_held = false;
    s->enabled = true;

    __g_sliders[__g_slidercount] = s;
    return __g_slidercount++;
}

Slider *Get_Slider(int id) {
    if (id < 0 || id >= __g_slidercount) return NULL;
    return __g_sliders[id];
}

// Removes all Sliders
void Destroy_Sliders() {
    for (int i=0; i<__g_slidercount; i++) {
        free(__g_sliders[i]);
    }
}

SDL_Rect *__calc_rect_knob(Slider *s, SDL_Rect *r) {
    int val = *s->var * s->length * s->factor;
    if (s->orient == SLIDER_HORIZONTAL) {
        r->x = s->x + val - KNOB_WIDTH/2;
        r->y = s->y - KNOB_LENGTH/2;
        r->w = KNOB_WIDTH;
        r->h = KNOB_LENGTH;
        return r;
    } else {
        r->x = s->x - KNOB_LENGTH/2;
        r->y = s->y + s->length - val - KNOB_WIDTH/2;
        r->w = KNOB_LENGTH;
        r->h = KNOB_WIDTH;
        return r;
    }
}

void Draw_Sliders(SDL_Renderer *renderer) {
    for (int i=0; i<__g_slidercount; i++) {
        Slider *s = __g_sliders[i];

        SDL_SetRenderDrawColor(renderer, SLIDER_COLOUR);
        if (s->orient == SLIDER_HORIZONTAL) {
            SDL_RenderFillRect(renderer, &(struct SDL_Rect){ s->x, s->y-SLIDER_WIDTH/2, s->length, SLIDER_WIDTH });
        } else {
            SDL_RenderFillRect(renderer, &(struct SDL_Rect){ s->x-SLIDER_WIDTH/2, s->y, SLIDER_WIDTH, s->length });
        }

        SDL_SetRenderDrawColor(renderer, KNOB_COLOUR);
        SDL_Rect r;
        SDL_RenderFillRect(renderer, __calc_rect_knob(s, &r));
    }
}

void Sliders_HandleMouseButtonEvent(SDL_MouseButtonEvent event) {
    if (event.button != SDL_BUTTON_LEFT) return;

    for (int i=0; i<__g_slidercount; i++) {
        Slider *s = __g_sliders[i];
        if (!s->enabled) continue;
        
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            SDL_Rect knob;
            __calc_rect_knob(s, &knob);
            if (event.x >= knob.x
                && event.y >= knob.y
                && event.x <= knob.x+knob.w
                && event.y <= knob.y+knob.h
            ) {
                s->is_held = true;
            }
        }

        if (event.type == SDL_MOUSEBUTTONUP && s->is_held) s->is_held = false;
    }
}

void Sliders_HandleMouseMotionEvent(SDL_MouseMotionEvent event) {
    for (int i=0; i<__g_slidercount; i++) {
        Slider *s = __g_sliders[i];
        if (!s->enabled) continue;

        if (s->is_held) {
            if (s->orient == SLIDER_HORIZONTAL) {
                int newx = event.x;
                if (newx < s->x) newx = s->x;
                if (newx > s->x+s->length) newx = s->x+s->length;
                *s->var = ((float) newx - s->x)/s->length;
            } else {
                int newy = event.y;
                if (newy < s->y) newy = s->y;
                if (newy > s->y+s->length) newy = s->y+s->length;
                *s->var = (s->length - ((float) newy - s->y))/s->length;
            }
        }
    }
}