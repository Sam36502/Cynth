// SDL2 Hello, World!

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>

#include "lib/synth.h"
#include "lib/slider.h"
#include "lib/button.h"
#include "lib/midi.h"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

#define CLR_RGBA(clr) clr.r, clr.g, clr.b, clr.a
#define CLR_WHITE (struct SDL_Colour){ 0xFF, 0xFF, 0xFF, 0xFF }
#define CLR_BLACK (struct SDL_Colour){ 0x00, 0x00, 0x00, 0xFF }
#define CLR_ORANGE (struct SDL_Colour){ 0xFF, 0x80, 0x40, 0xFF }
#define CLR_GREEN (struct SDL_Colour){ 0x40, 0xFF, 0x80, 0xFF }
#define CLR_BLUE (struct SDL_Colour){ 0x00, 0x74, 0xD9, 0xFF }
#define CLR_PINK (struct SDL_Colour){ 0xF0, 0x12, 0xBE, 0xFF }
#define CLR_LIGHT_GREY (struct SDL_Colour){ 0xDD, 0xDD, 0xDD, 0xFF }
#define CLR_DARK_GREY (struct SDL_Colour){ 0xAA, 0xAA, 0xAA, 0xFF }
#define CLR_TRANSPARENT (struct SDL_Colour){ 0x00, 0x00, 0x00, 0x00 }
#define CLR_TRANSPARENT_WHITE (struct SDL_Colour){ 0xFF, 0xFF, 0xFF, 0x40 }

#define NOTE_COUNT 25
#define SEMITONE 1.05946309436
#define START_FREQ 220.0
#define NUM_SYNTH_CHANNELS 16

void err_msg(const char *msg);
void cls();
void draw_bits(int x, int y, unsigned int n, unsigned int length);
void draw_text(int x, int y, char *str, SDL_Color clr);
void change_channel(int newchannel);
void release_all();

void cb_trigger_env(void *);
void cb_trigger_rel(void *);
void cb_change_channel(void *);

SDL_Window *g_window = NULL;
SDL_Renderer *g_renderer = NULL;
TTF_Font *g_font = NULL;
SND_Synth *g_synth = NULL;

int selected_channel = 0;
int slid_chfilt = -1;
int slid_chmod = -1;
int slid_chvol = -1;
int slid_chenv_a = -1;
int slid_chenv_d = -1;
int slid_chenv_s = -1;
int slid_chenv_r = -1;

bool sus_pedal = false;

float keyboard_frequencies[NOTE_COUNT];

int main(int argc, char* args[]) {

	// Initialisation
	if (SDL_Init(SDL_INIT_VIDEO) < 0) err_msg("Failed to initialise SDL");
	if (TTF_Init() < 0) err_msg("Failed to initialise SDL2-TTF");
	g_window = SDL_CreateWindow(
		"Sound-Generation Test",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN
	);
	if (g_window == NULL) err_msg("Failed to create Window");
	g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
	if (g_renderer == NULL) err_msg("Failed to create Renderer");
	g_font = TTF_OpenFont("C:\\WINDOWS\\FONTS\\PALA.TTF", 20);
	if (g_font == NULL) err_msg("Failed to open font");
	g_synth = SND_OpenSynth(NUM_SYNTH_CHANNELS, NULL);
	if (g_synth == NULL) err_msg("Failed to create synth");

	SND_CreateChannelOscilloscope(g_synth, 0, g_renderer,
		&(struct SDL_Rect){ SCREEN_WIDTH-200-200, 0, 200, 150 },
		CLR_GREEN, CLR_WHITE, CLR_BLACK
	);
	SND_CreateChannelOscilloscope(g_synth, 1, g_renderer,
		&(struct SDL_Rect){ SCREEN_WIDTH-200, 0, 200, 150 },
		CLR_GREEN, CLR_WHITE, CLR_BLACK
	);
	SND_CreateChannelOscilloscope(g_synth, 2, g_renderer,
		&(struct SDL_Rect){ SCREEN_WIDTH-200-200, 150, 200, 150 },
		CLR_GREEN, CLR_WHITE, CLR_BLACK
	);
	SND_CreateChannelOscilloscope(g_synth, 3, g_renderer,
		&(struct SDL_Rect){ SCREEN_WIDTH-200, 150, 200, 150 },
		CLR_GREEN, CLR_WHITE, CLR_BLACK
	);

	SND_CreateMixOscilloscope(g_synth, g_renderer,
		&(struct SDL_Rect){ SCREEN_WIDTH-400, 150+150, 400, 150 },
		CLR_GREEN, CLR_WHITE, CLR_BLACK
	);

	New_Slider(25, 25, 200, SLIDER_HORIZONTAL, &g_synth->volume, 1.0);

	// Calculate keyboard frequencies and add them to table
	// And create buttons for each note
	int note_width = SCREEN_WIDTH/NOTE_COUNT;
	for (int i=0; i<NOTE_COUNT; i++) {
		if (i==0) {
			keyboard_frequencies[i] = START_FREQ;
		} else {
			keyboard_frequencies[i] = keyboard_frequencies[i-1] * SEMITONE;
		}

		int degree = (i+9)%12;
		SDL_Colour keyclr = CLR_WHITE;
		if ((degree < 5 && (degree&1) == 1)
			|| (degree > 4 && (degree&1) == 0)
		) {
			keyclr = CLR_BLACK;
		}

		New_Button(
			(struct SDL_Rect){
				i*note_width+1, 452,
				note_width-1, SCREEN_HEIGHT-454
			},
			false,
			&cb_trigger_env,
			&cb_trigger_rel,
			&keyboard_frequencies[i],
			keyclr,
			CLR_ORANGE,
			CLR_PINK
		);
	}

	int channel_1 = 0;
	int channel_2 = 1;
	int channel_3 = 2;
	int channel_4 = 3;

	New_Button(
		(struct SDL_Rect){ SCREEN_WIDTH-200-200, 0, 200, 150 }, false,
		&cb_change_channel, NULL, &channel_1,
		CLR_TRANSPARENT, CLR_TRANSPARENT_WHITE, CLR_PINK
	);
	New_Button(
		(struct SDL_Rect){ SCREEN_WIDTH-200, 0, 200, 150 }, false,
		&cb_change_channel, NULL, &channel_2,
		CLR_TRANSPARENT, CLR_TRANSPARENT_WHITE, CLR_PINK
	);
	New_Button(
		(struct SDL_Rect){ SCREEN_WIDTH-200-200, 150, 200, 150 }, false,
		&cb_change_channel, NULL, &channel_3,
		CLR_TRANSPARENT, CLR_TRANSPARENT_WHITE, CLR_PINK
	);
	New_Button(
		(struct SDL_Rect){ SCREEN_WIDTH-200, 150, 200, 150 }, false,
		&cb_change_channel, NULL, &channel_4,
		CLR_TRANSPARENT, CLR_TRANSPARENT_WHITE, CLR_PINK
	);

	//SND_SetChannelFilter(g_synth, selected_channel, 0, SND_FILT_TIMECRUSH);
	//New_Slider(415, 75, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->filter_path[0]->amount, 1.0);
	//SND_SetChannelFilter(g_synth, selected_channel, 1, SND_FILT_BITCRUSH);
	//New_Slider(300, 75, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->filter_path[1]->amount, 1.0);
	//SND_SetChannelFilter(g_synth, selected_channel, 2, SND_FILT_LOW_PASS);
	//New_Slider(175, 75, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->filter_path[2]->amount, 1.0);
	//SND_SetChannelFilter(g_synth, selected_channel, 3, SND_FILT_HIGH_PASS);
	//New_Slider(50, 75, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->filter_path[3]->amount, 1.0);

	// Channel-Specific Slider
	slid_chmod = New_Slider(535, 75, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->wave_mod, 1.0);
	slid_chvol = New_Slider(535, 275, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->volume, 1.0);
	slid_chenv_a = New_Slider( 50, 275, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->env_values.attack, 1.0);
	slid_chenv_d = New_Slider(175, 275, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->env_values.decay, 1.0);
	slid_chenv_s = New_Slider(300, 275, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->env_values.sustain, 1.0);
	slid_chenv_r = New_Slider(415, 275, 150, SLIDER_VERTICAL, &g_synth->channels[selected_channel]->env_values.release, 1.0);

	/*
		// Open Test MIDI file
		MIDI_File *midi = MIDI_OpenFile("./midis/Song_of_Storms.mid");
		
		// MIDI Debug output
		if (midi != NULL) {
			printf("Parsed MIDI Header:\n");
			printf("              Format: %s\n", MIDI_GetFormatName(midi->header.format));
			printf("      Num. of Tracks: %i\n", midi->header.num_tracks);
			printf("  Ticks/Quarter-Note: %i\n\n", midi->header.ppq);
			fflush(stdout);
		}
	*/

	// Main Loop
	bool isRunning = true;
	bool redraw = true;
	SDL_Event curr_event;
	while (isRunning) {

		// Handle events
		int scode = SDL_WaitEvent(&curr_event);

		if (scode != 0) {
			switch (curr_event.type) {
				case SDL_QUIT:
					isRunning = false;
				continue;

				case SDL_KEYDOWN: {
					SDL_KeyCode kc = curr_event.key.keysym.sym;
					if (kc == SDLK_LSHIFT) {
						sus_pedal = true;
					}
				} break;

				case SDL_KEYUP: {
					SDL_KeyCode kc = curr_event.key.keysym.sym;
					int jump_size = 10;
					if (curr_event.key.keysym.mod & KMOD_SHIFT) jump_size = 100;
					switch (kc) {
						//case SDLK_F5: MIDI_PlayFile(midi, g_synth); break;
						case SDLK_KP_0: SND_SetChannelWaveform(g_synth, selected_channel, SND_WAVE_SINE, 0); break;
						case SDLK_KP_1: SND_SetChannelWaveform(g_synth, selected_channel, SND_WAVE_PULSE, 0); break;
						case SDLK_KP_2: SND_SetChannelWaveform(g_synth, selected_channel, SND_WAVE_TRI, 0); break;
						case SDLK_KP_3: SND_SetChannelWaveform(g_synth, selected_channel, SND_WAVE_SAW, 0); break;
						case SDLK_KP_4: SND_SetChannelWaveform(g_synth, selected_channel, SND_WAVE_NOISE, 0); break;
						case SDLK_KP_PLUS: SND_SetChannelFrequency(g_synth, selected_channel, SND_GetChannelFrequency(g_synth, selected_channel) + jump_size); break;
						case SDLK_KP_MINUS: SND_SetChannelFrequency(g_synth, selected_channel, SND_GetChannelFrequency(g_synth, selected_channel) - jump_size); break;
						case SDLK_ESCAPE: SND_StopAllChannels(g_synth); break;
						case SDLK_LSHIFT: sus_pedal = false; release_all(); break;
						default: break;
					};
					redraw = true;
				} break;

				case SDL_MOUSEBUTTONDOWN: 
				case SDL_MOUSEBUTTONUP:
					if (curr_event.button.button == SDL_BUTTON_MIDDLE) {
						printf("Cursor Position: (%i/%i)\n", curr_event.button.x, curr_event.button.y);
						fflush(stdout);
					}
					Sliders_HandleMouseButtonEvent(curr_event.button);
					Buttons_HandleMouseButtonEvent(curr_event.button);
				break;

				case SDL_MOUSEMOTION:
					Sliders_HandleMouseMotionEvent(curr_event.motion);
					Buttons_HandleMouseMotionEvent(curr_event.motion);
					redraw = true;
				break;

				case SDL_MOUSEWHEEL: {
					float vol = SND_GetChannelVolume(g_synth, selected_channel);
					SND_SetChannelVolume(g_synth, selected_channel, vol + curr_event.wheel.y * 0.1);
					redraw = true;
				} break;
			}
		}

		if (redraw) {
			cls();
		
			// Draw Borders
			SDL_SetRenderDrawColor(g_renderer, CLR_RGBA(CLR_DARK_GREY));
			SDL_RenderDrawRects(g_renderer,
				(struct SDL_Rect[8]){
					{	1+0,		1+0,		SCREEN_WIDTH-400,	50					},
					{	1+0,		1+50,		SCREEN_WIDTH-400,	200					},
					{	1+0,		1+250,	120,					200					},
					{	1+120,		1+250,	120,					200					},
					{	1+240,		1+250,	120,					200					},
					{	1+360,		1+250,	120,					200					},
					{	1+480,		1+250,	120,					200					},
					{	1+0,		1+450,	SCREEN_WIDTH,			SCREEN_HEIGHT-450	},
				}, 8
			);

			SDL_SetRenderDrawColor(g_renderer, CLR_RGBA(CLR_LIGHT_GREY));
			SDL_RenderDrawRects(g_renderer,
				(struct SDL_Rect[8]){
					{	0,		0,		SCREEN_WIDTH-400,	50					},
					{	0,		50,		SCREEN_WIDTH-400,	200					},
					{	0,		250,	120,					200				},
					{	120,	250,	120,					200				},
					{	240,	250,	120,					200				},
					{	360,	250,	120,					200				},
					{	480,	250,	120,					200				},
					{	0,		450,	SCREEN_WIDTH,		SCREEN_HEIGHT-450	},
				}, 8
			);

			redraw = false;
		}

		SND_DrawOscilloscopes(g_synth, g_renderer);
		Draw_Sliders(g_renderer);
		Draw_Buttons(g_renderer);

		// Update Screen
		SDL_RenderPresent(g_renderer);

	}

	//MIDI_CloseFile(midi);

	// Termination
	SND_CloseSynth(g_synth);
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	TTF_CloseFont(g_font);
	TTF_Quit();
	SDL_Quit();
	return 0;
}

void err_msg(const char *msg) {
	printf("[ERROR] %s: %s\n", msg, SDL_GetError());
	exit(1);
}

void cls() {
	SDL_SetRenderDrawColor(g_renderer, CLR_RGBA(CLR_BLUE));
	SDL_RenderClear(g_renderer);
}

void draw_bits(int x, int y, unsigned int n, unsigned int length) {
	SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderFillRect(g_renderer, &(struct SDL_Rect){ x, y, 16 * length, 16 });
	for (int i=0; i<length; i++) {
		struct SDL_Rect r = { x + (i*16) + 1, y + 1, 14, 14 };
		unsigned int mask = 1 << (length - i - 1);
		if ((n & mask) == 0) {
			SDL_SetRenderDrawColor(g_renderer, 0xFF, 0x41, 0x36, 0xFF);
		} else {
			SDL_SetRenderDrawColor(g_renderer, 0x2E, 0xCC, 0x40, 0xFF);
		}
		SDL_RenderFillRect(g_renderer, &r);
	}
}

void draw_text(int x, int y, char *str, SDL_Color clr) {
	SDL_Surface *textSurface = TTF_RenderText_Blended(g_font, str, clr);
	SDL_Rect destRect = {x, y, textSurface->w, textSurface->h};
	SDL_Texture *textTexture = SDL_CreateTextureFromSurface(g_renderer, textSurface);
	SDL_FreeSurface(textSurface);
	if (SDL_RenderCopy(
		g_renderer, textTexture,
		NULL, &destRect
	) < 0) err_msg("Failed to blit text surface");
}

void change_channel(int newchannel) {
	selected_channel = newchannel;

	Slider *slp;
	slp = Get_Slider(slid_chmod);
	slp->var = &g_synth->channels[newchannel]->wave_mod;
	slp = Get_Slider(slid_chvol);
	slp->var = &g_synth->channels[newchannel]->volume;
	slp = Get_Slider(slid_chenv_a);
	slp->var = &g_synth->channels[newchannel]->env_values.attack;
	slp = Get_Slider(slid_chenv_d);
	slp->var = &g_synth->channels[newchannel]->env_values.decay;
	slp = Get_Slider(slid_chenv_s);
	slp->var = &g_synth->channels[newchannel]->env_values.sustain;
	slp = Get_Slider(slid_chenv_r);
	slp->var = &g_synth->channels[newchannel]->env_values.release;
}

void cb_trigger_env(void *_freq) {
	float *freq = (float *) _freq;
	SND_SetChannelFrequency(g_synth, selected_channel, *freq);
	SND_TriggerChannelEnvelope(g_synth, selected_channel);
}

void cb_trigger_rel(void *_) {
	if (sus_pedal) return;
	SND_ReleaseChannelEnvelope(g_synth, selected_channel);
}

void cb_change_channel(void *_ch) {
	int *ch = (int *) _ch;
	change_channel(*ch);
}

void release_all() {
	for (int i=0; i<g_synth->num_channels; i++) {
		SND_ReleaseChannelEnvelope(g_synth, i);
	}
}