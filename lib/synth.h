//
//
//      Basic SDL Sound Synthesizer
// 
//      By Samuel Pearce
//      2023
//

#ifndef SYNTH_H
#define SYNTH_H

//
//      INCLUDES
//

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

//
//      DEFINITIONS
//

#define MAX_CHANNELS 64
#define MAX_FILTERS 64
#define TAU 2 * M_PI

// See synth.c SND_OpenSynth() for default audio specifications

//
//      TYPES
//

typedef enum SND_EnvState {
    SND_ENV_HOLD,   // Doubles as waiting before triggering and sustain states
    SND_ENV_ATTACK,
    SND_ENV_DECAY,
    SND_ENV_RELEASE,
} SND_EnvState;

typedef enum SND_Wave {
    SND_WAVE_SINE,
    SND_WAVE_PULSE,
    SND_WAVE_TRI,
    SND_WAVE_SAW,
    SND_WAVE_NOISE,
} SND_Wave;

typedef enum SND_FilterType {
    SND_FILT_NONE,
    SND_FILT_BITCRUSH,
    SND_FILT_TIMECRUSH,
    SND_FILT_LOW_PASS,
    SND_FILT_HIGH_PASS,
} SND_FilterType;

typedef struct SND_Envelope {
    float attack;   // Attack time in seconds
    float decay;    // Decay time in seconds
    float sustain;  // Sustain volume in percent
    float release;  // Release time in seconds
} SND_Envelope;

typedef struct SND_Filter {
    float amount;
    float (* filter_func)(float sample, float amount, void *udata);
    void *udata;
} SND_Filter;

typedef struct SND_Oscilloscope {
    SDL_Color bg_colour;
    SDL_Color beam_colour;
    SDL_Color border_colour;
    SDL_Rect *rect;
    float *sample_buffer;
} SND_Oscilloscope;

typedef struct SND_Channel {
    float phase;
    float freq;
    float volume;
    float env_vol;
    float wave_mod;
    bool should_release;
    SND_Oscilloscope *osc;
    SND_Wave waveform;
    SND_Envelope env_values;
    SND_EnvState env_state;
    //SND_Filter *filter_path[MAX_FILTERS];
} SND_Channel;

typedef struct SND_Synth {
    int num_channels;
    float volume;
    SDL_AudioDeviceID dev_id;
    SDL_AudioSpec spec;
    SND_Oscilloscope *osc;
    SND_Channel *channels[MAX_CHANNELS];
} SND_Synth;

//
//      PUBLIC FUNCTIONS
//

//  Opens a new synth with a given audio device ID
//  
//  Pass `NULL` as dev_id to let SDL select a reasonable default device.
//  Returns `NULL` if it failed
SND_Synth *SND_OpenSynth(int num_channels, const char *dev_id);

//  Closes a previously opened synth
//
void SND_CloseSynth(SND_Synth *synth);

//  Returns a null-terminated name string for a given waveform
//
const char* SND_GetWaveformName(SND_Wave wave);


////    Synth Getters/Setters (Necessary?)


//  Sets the master volume for a synth
//  
//  The float volume value ranges from 0.0 to 1.0.
//  A synth's master volume is set to 0.5 by default.
void SND_SetMasterVolume(SND_Synth *synth, float volume);

//  Returns the current master volume of a synth
//
//  The float volume value ranges from 0.0 to 1.0.
float SND_GetMasterVolume(SND_Synth *synth);

//  Stops all channels from playing
//  
void SND_StopAllChannels(SND_Synth *synth);

//  Sets the volume of a channel
//  
//  The float volume value ranges from 0.0 to 1.0.
//  A channel's volume is set to 1.0 by default.
//  Set the channel's volume to 0.0 to disable it.
void SND_SetChannelVolume(SND_Synth *synth, int ch, float volume);

//  Returns the current volume of a given channel
//
//  The float volume value ranges from 0.0 to 1.0.
float SND_GetChannelVolume(SND_Synth *synth, int ch);

//  Sets the frequency of a channel
//  
//  A channel's frequency is set to 1000 by default.
void SND_SetChannelFrequency(SND_Synth *synth, int ch, float frequency);

//  Returns the current frequency of a given channel
//
float SND_GetChannelFrequency(SND_Synth *synth, int ch);

//  Sets the waveform of a channel
//  
//  The wave option selects which wave form the channel should use when
//  generating audio from a list of provided functions.
//  This is set to SND_WAVE_SINE by default.
//  The mod option is for fine-tuning wave properties, but most
//  wave functions ignore it. Here are the properties it alters:
//
//      SND_WAVE_PULSE  Sets the duty cycle in percent. 0.0 defaults to 0.5
//      SND_WAVE_SAW    Sets the saw to be a falling wave instead of rising if mod != 0.0
void SND_SetChannelWaveform(SND_Synth *synth, int ch, SND_Wave wave, float mod);

//  Returns the currently selected waveform of a channel
//
//  Passing `NULL` to mod is fine if you don't need it.
SND_Wave SND_GetChannelWaveform(SND_Synth *synth, int ch, float *mod);

//  Sets a filter in the filter path for a channel
//
//  If NULL is passed for the filter, the filter is reset and ignored.
//  A Channel's filter-path is all NULLs by default.
void SND_SetChannelFilter(SND_Synth *synth, int ch, int filter_id, SND_FilterType filter_type);


////    Envelope Functions


//  Sets the envelope values of a channel's envelope-generator
//  
//  The SND_Envelope struct has following values:
//      float attack;   // Attack time in seconds (Default 0.0)
//      float decay;    // Decay time in seconds (Default 0.0)
//      float sustain;  // Sustain volume in percent (Default 1.0)
//      float release;  // Release time in seconds (Default 0.0)
void SND_SetChannelEnvelope(SND_Synth *synth, int ch, SND_Envelope env);

//  Returns the envelope settings for a given channel
//
SND_Envelope SND_GetChannelEnvelope(SND_Synth *synth, int ch);

//  Triggers a channel envelope to begin
//
//  If the envelope was already in the middle of a cycle,
//  this resets it to the attack state and builds to max volume again.
void SND_TriggerChannelEnvelope(SND_Synth *synth, int ch);

//  Triggers a channel envelope to release
//
void SND_ReleaseChannelEnvelope(SND_Synth *synth, int ch);


////    Oscilloscope Functions


//  Creates an oscilloscope for one of the synth's channels to display on the screen
// 
//  This will automatically draw over itself as long as there is sound playing.
//  Rect is the box on the screen it should draw the oscilloscope inside of.
//  Set border's alpha to 0 for no border;
void SND_CreateChannelOscilloscope(
    SND_Synth *synth, int ch,
    SDL_Renderer *renderer, SDL_Rect *rect,
    SDL_Color beam, SDL_Color border, SDL_Color bg
);

//  Removes a previously created oscilloscope on a channel
//
void SND_RemoveChannelOscilloscope(SND_Synth *synth, int ch);

//  Creates an oscilloscope for the synth's main mix output
// 
//  This will automatically draw over itself as long as there is sound playing.
//  Rect is the box on the screen it should draw the oscilloscope inside of.
//  Set border's alpha to 0 for no border;
void SND_CreateMixOscilloscope(
    SND_Synth *synth,
    SDL_Renderer *renderer, SDL_Rect *rect,
    SDL_Color beam, SDL_Color border, SDL_Color bg
);

//  Removes a previously created oscilloscope for the synth
//
void SND_RemoveMixOscilloscope(SND_Synth *synth);

//  Draws all the oscilloscopes for a given synth
//
void SND_DrawOscilloscopes(SND_Synth *synth, SDL_Renderer *renderer);


////    Music File Functions


//
//      INTERNAL FUNCTIONS
//

//  Wave Functions
float __wave_sine(float phase);
float __wave_pulse(float phase, float mod);
float __wave_saw(float phase, float mod);
float __wave_triangle(float phase, float mod);
float __wave_noise(float phase, float mod);

//  Filter Functions
float __filter_bitcrush(float sample, float mod, void *udata);
float __filter_timecrush(float sample, float mod, void *udata);
float __filter_lowpass(float sample, float mod, void *udata);
float __filter_highpass(float sample, float mod, void *udata);

//  Filter Constructors
SND_Filter *__filter_create_bitcrush();
SND_Filter *__filter_create_timecrush();
SND_Filter *__filter_create_lowpass();
SND_Filter *__filter_create_highpass();

//  Called by SDL whenever the audio device needs more samples
//  
void __audio_callback(void *_synth, Uint8 *_buffer, int size);

//  Creates a new channel
//  
SND_Channel *__create_channel(SND_Synth *synth);

//  Called in the audio callback to progress the envelope one step
//
void __progress_envelope(SND_Synth *synth, SND_Channel *chan);

//  Renders a single Oscilloscope
//  
void __draw_oscilloscope(SND_Synth *synth, SDL_Renderer *renderer, SND_Oscilloscope *osc);

#endif