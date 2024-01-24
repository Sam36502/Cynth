#include "synth.h"

//
//      PUBLIC FUNCTIONS
//

//  Opens a new synth with a given audio device ID
//  
//  Pass `NULL` as dev_id to let SDL select a reasonable default device.
//  Returns `NULL` if it failed
SND_Synth *SND_OpenSynth(int num_channels, const char *device) {
    srand(time(NULL));

    // Initialise SDL Audio if not already done
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    // Create synth object
    SND_Synth *synth = malloc(sizeof(SND_Synth));
    synth->num_channels = num_channels;
    synth->volume = 0.1; // Your ears will thank me
    synth->osc = NULL;

    for (int i=0; i<num_channels; i++) {
        synth->channels[i] = __create_channel(synth);
    }

    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = AUDIO_F32SYS;
    spec.channels = 1;
    spec.samples = 441;
    spec.callback = __audio_callback;
    spec.userdata = synth;

    // Open Audio Device
    SDL_AudioSpec *obtained = NULL;
    int device_id = SDL_OpenAudioDevice(
        device, 0,
        &spec, obtained,
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE
        | SDL_AUDIO_ALLOW_SAMPLES_CHANGE
    );
    if (device_id <= 0) return NULL;
    if (obtained != NULL) spec = *obtained;

    synth->dev_id = device_id;
    synth->spec = spec;

    SDL_PauseAudioDevice(device_id, 0);

    return synth;
}

//  Closes a previously opened synth
//
void SND_CloseSynth(SND_Synth *synth) {
    SDL_PauseAudioDevice(synth->dev_id, 1);
    
    for (int i=0; i<synth->num_channels; i++) {
        free(synth->channels[i]);
    }

    SDL_CloseAudioDevice(synth->dev_id);
    free(synth);
}

//  Sets the master volume for a synth
//  
//  The float volume value ranges from 0.0 to 1.0.
//  A synth's master volume is set to 0.5 by default.
void SND_SetMasterVolume(SND_Synth *synth, float volume) {
    synth->volume = volume;
}

//  Returns the current master volume of a synth
//
//  The float volume value ranges from 0.0 to 1.0.
float SND_GetMasterVolume(SND_Synth *synth) {
    return synth->volume;
}

//  Stops all channels from playing
//  
void SND_StopAllChannels(SND_Synth *synth) {
    for (int i=0; i<synth->num_channels; i++) {
        SND_SetChannelVolume(synth, i, 0.0);
        SND_ReleaseChannelEnvelope(synth, i);
    }
}

//  Sets the volume of a channel
//  
//  The float volume value ranges from 0.0 to 1.0.
//  A channel's volume is set to 1.0 by default.
//  Set the channel's volume to 0.0 to disable it.
void SND_SetChannelVolume(SND_Synth *synth, int ch, float volume) {
    if (ch < 0 || ch > synth->num_channels) return;
    synth->channels[ch]->volume = volume;
}

//  Returns the current volume of a given channel
//
//  The float volume value ranges from 0.0 to 1.0.
//  Returns -1.0 if an invalid channel ID was provided
float SND_GetChannelVolume(SND_Synth *synth, int ch) {
    if (ch < 0 || ch > synth->num_channels) return -1;
    return synth->channels[ch]->volume;
}

//  Sets the frequency of a channel
//  
//  A channel's frequency is set to 1000 by default.
void SND_SetChannelFrequency(SND_Synth *synth, int ch, float frequency) {
    if (ch < 0 || ch > synth->num_channels) return;
    synth->channels[ch]->freq = frequency;
}

//  Returns the current frequency of a given channel
//
//  Returns -1.0 if an invalid channel ID was provided
float SND_GetChannelFrequency(SND_Synth *synth, int ch) {
    if (ch < 0 || ch > synth->num_channels) return -1;
    return synth->channels[ch]->freq;
}

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
void SND_SetChannelWaveform(SND_Synth *synth, int ch, SND_Wave wave, float mod) {
    if (ch < 0 || ch > synth->num_channels) return;
    synth->channels[ch]->waveform = wave;
    synth->channels[ch]->wave_mod = mod;
}

//  Returns the currently selected waveform of a channel
//
//  Passing `NULL` to mod is fine if you don't need it.
//  Returns SND_WAVE_SINE if an invalid channel ID was provided
SND_Wave SND_GetChannelWaveform(SND_Synth *synth, int ch, float *mod) {
    if (ch < 0 || ch > synth->num_channels) return SND_WAVE_SINE;
    if (mod != NULL) *mod = synth->channels[ch]->wave_mod;
    return synth->channels[ch]->waveform;
}

//  Sets a filter in the filter path for a channel
//
//  If SND_FILT_NONE is passed for the filter, the filter is reset and ignored.
//  A Channel's filter-path is all SND_FILT_NONE by default.
void SND_SetChannelFilter(SND_Synth *synth, int ch, int filter_id, SND_FilterType filter_type) {
    if (ch < 0 || ch > synth->num_channels) return;

    //SND_Filter *newfilt;
    //switch (filter_type) {
    //    case SND_FILT_BITCRUSH: newfilt = __filter_create_bitcrush(); break;
    //    case SND_FILT_TIMECRUSH: newfilt = __filter_create_timecrush(); break;
    //    case SND_FILT_LOW_PASS: newfilt = __filter_create_lowpass(); break;
    //    case SND_FILT_HIGH_PASS: newfilt = __filter_create_highpass(); break;
    //    case SND_FILT_NONE: {
    //        free(synth->channels[ch]->filter_path[filter_id]);
    //        newfilt = NULL;
    //    } break;
    //}
    //synth->channels[ch]->filter_path[filter_id] = newfilt;
}

//  Returns a capitalised name string for a given waveform
//
//  E.g.: SND_GetWaveformName(SND_WAVE_SINE) => "Sine"
const char* SND_GetWaveformName(SND_Wave wave) {
    switch (wave) {
        case SND_WAVE_SINE: return "Sine";
        case SND_WAVE_PULSE: return "Pulse";
        case SND_WAVE_TRI: return "Triangle";
        case SND_WAVE_SAW: return "Sawtooth";
        case SND_WAVE_NOISE: return "Noise";
    }
    return "";
}

//  Sets the envelope values of a channel's envelope-generator
//  
//  The SND_Envelope struct has following values:
//      float attack;   // Attack time in seconds (Default 0.0)
//      float decay;    // Decay time in seconds (Default 0.0)
//      float sustain;  // Sustain volume in percent (Default 1.0)
//      float release;  // Release time in seconds (Default 0.0)
void SND_SetChannelEnvelope(SND_Synth *synth, int ch, SND_Envelope env) {
    if (ch < 0 || ch > synth->num_channels) return;
    synth->channels[ch]->env_values = env;
}

//  Returns the envelope settings for a given channel
//
//  Returns all -1.0s if an invalid channel ID was provided
SND_Envelope SND_GetChannelEnvelope(SND_Synth *synth, int ch) {
    if (ch < 0 || ch > synth->num_channels) return (struct SND_Envelope){-1.0, -1.0, -1.0, -1.0};
    return synth->channels[ch]->env_values;
}

//  Triggers a channel envelope to begin
//
//  If the envelope was already in the middle of a cycle,
//  this resets it to the attack state and builds to max volume again.
void SND_TriggerChannelEnvelope(SND_Synth *synth, int ch) {
    if (ch < 0 || ch > synth->num_channels) return;
    synth->channels[ch]->env_state = SND_ENV_ATTACK;
}

//  Triggers a channel envelope to release
//
void SND_ReleaseChannelEnvelope(SND_Synth *synth, int ch) {
    if (ch < 0 || ch > synth->num_channels) return;
    synth->channels[ch]->should_release = true;
    synth->channels[ch]->env_state = SND_ENV_RELEASE;
}

//  Creates an oscilloscope for one of the synth's channels to display on the screen
// 
//  This will automatically draw over itself as long as there is sound playing.
//  Rect is the box on the screen it should draw the oscilloscope inside of.
//  Set border's alpha to 0 for no border;
//  Will limit rectangle width to the number of synth samples per chunk.
void SND_CreateChannelOscilloscope(
    SND_Synth *synth, int ch,
    SDL_Renderer *renderer, SDL_Rect *rect,
    SDL_Color beam, SDL_Color border, SDL_Color bg
) {
    if (ch < 0 || ch > synth->num_channels) return;
    SND_Channel *chan = synth->channels[ch];
    if (chan->osc != NULL) return;
    if (rect->w > synth->spec.samples) rect->w = synth->spec.samples;

    SND_Oscilloscope *osc = malloc(sizeof(SND_Oscilloscope));
    osc->bg_colour = bg;
    osc->beam_colour = beam;
    osc->border_colour = border;
    osc->rect = rect;
    osc->sample_buffer = malloc(sizeof(float) * rect->w);

    synth->channels[ch]->osc = osc;
}

//  Removes a previously created oscilloscope on a channel
//
void SND_RemoveChannelOscilloscope(SND_Synth *synth, int ch) {
    if (ch < 0 || ch > synth->num_channels) return;
    SND_Channel *chan = synth->channels[ch];
    if (chan->osc != NULL) return;

    free(chan->osc->sample_buffer);
    free(chan->osc);
    chan->osc = NULL;
}

//  Creates an oscilloscope for the synth's main mix output
// 
//  This will automatically draw over itself as long as there is sound playing.
//  Rect is the box on the screen it should draw the oscilloscope inside of.
//  Set border's alpha to 0 for no border;
void SND_CreateMixOscilloscope(
    SND_Synth *synth,
    SDL_Renderer *renderer, SDL_Rect *rect,
    SDL_Color beam, SDL_Color border, SDL_Color bg
) {
    if (synth->osc != NULL) return;
    if (rect->w > synth->spec.samples) rect->w = synth->spec.samples;

    SND_Oscilloscope *osc = malloc(sizeof(SND_Oscilloscope));
    osc->bg_colour = bg;
    osc->beam_colour = beam;
    osc->border_colour = border;
    osc->rect = rect;
    osc->sample_buffer = malloc(sizeof(float) * rect->w);

    synth->osc = osc;
}

//  Removes a previously created oscilloscope for the synth
//
void SND_RemoveMixOscilloscope(SND_Synth *synth) {
    if (synth->osc != NULL) return;

    free(synth->osc->sample_buffer);
    free(synth->osc);
    synth->osc = NULL;
}


//  Draws all the oscilloscopes for a given synth
//
void SND_DrawOscilloscopes(SND_Synth *synth, SDL_Renderer *renderer) {
    for (int i=0; i<synth->num_channels; i++) {
        SND_Oscilloscope *osc = synth->channels[i]->osc;
        if (osc != NULL)
            __draw_oscilloscope(synth, renderer, osc);
    }

    SND_Oscilloscope *mixosc = synth->osc;
    if (mixosc != NULL) __draw_oscilloscope(synth, renderer, mixosc);
}

//
//      INTERNAL FUNCTIONS
//

//  Wave Functions
float __wave_sine(float phase) {
    return sin(phase * TAU);
}

float __wave_pulse(float phase, float mod) {
    if (mod == 0.0) mod = 0.5;
    if (phase > mod) {
        return 1.0;
    } else {
        return -1.0;
    }
}

float __wave_saw(float phase, float mod) {
    if (mod > 0.0) {
        return -2.0 * phase + 1.0;
    } else {
        return 2.0 * phase - 1.0;
    }
}

float __wave_triangle(float phase, float mod) {
    if (mod <= 0.0) mod = 0.5;
    if (phase > mod) {
        return 2.0 - (4.0 * phase - 1.0);
    } else {
        return 4.0 * phase - 1.0;
    }
}

float __wave_noise(float phase, float mod) {
    return ((float) rand()) / (RAND_MAX / 2) - 1;
}

float __filter_bitcrush(float sample, float mod, void *udata) {
    if (mod == 0.0) return sample;
    int mask = 0;
    for (int i=0; i<(int)((1-mod)*7+1); i++) {
        mask |= 1 << i;
    }
    Uint8 crumch = (Uint8) ((sample + 1.0) / 2.0 * mask);
    crumch &= mask;
    sample = ((float)crumch) / mask * 2.0 - 1.0;
    return sample;
}

typedef struct __timestep_t { float sample; Uint8 step; } __timestep_t;
float __filter_timecrush(float sample, float mod, void *udata) {
    if (mod == 0.0) return sample;
    __timestep_t *ts = (__timestep_t *) udata;

    Uint8 steps = (Uint8) (mod * 16);
    if (ts->step >= steps) {
        ts->step = 0;
        ts->sample = sample;
    }

    ts->step++;
    return ts->sample;
}

float __filter_lowpass(float sample, float mod, void *udata) {
    float *prev = ((float *) udata);

    sample = *prev + mod * (sample - *prev);

    *prev = sample;
    return sample;
}

float __filter_highpass(float sample, float mod, void *udata) {
    float lowpassd = __filter_lowpass(sample, mod, udata);

    sample = sample - lowpassd;

    return sample;
}

SND_Filter *__filter_create_bitcrush() {
    SND_Filter *f = malloc(sizeof(SND_Filter));
    f->amount = 0.0;
    f->filter_func = &__filter_bitcrush;
    f->udata = NULL;
    return f;
}

SND_Filter *__filter_create_timecrush() {
    __timestep_t *prev = malloc(sizeof(__timestep_t));
    prev->sample = 0.0;
    prev->step = 0;
    SND_Filter *f = malloc(sizeof(SND_Filter));
    f->amount = 0.0;
    f->filter_func = &__filter_timecrush;
    f->udata = prev;
    return f;
}

SND_Filter *__filter_create_lowpass() {
    float *prev = malloc(sizeof(float));
    *prev = 0.0;
    SND_Filter *f = malloc(sizeof(SND_Filter));
    f->amount = 1.0;
    f->filter_func = &__filter_lowpass;
    f->udata = prev;
    return f;
}

SND_Filter *__filter_create_highpass() {
    float *prev = malloc(sizeof(float));
    *prev = 0.0;
    SND_Filter *f = malloc(sizeof(SND_Filter));
    f->amount = 0.0;
    f->filter_func = &__filter_highpass;
    f->udata = prev;
    return f;
}

//  Called by SDL whenever the audio device needs more samples
//  
void __audio_callback(void *_synth, Uint8 *_buffer, int size) {
    SND_Synth *synth = (SND_Synth *) _synth;
    float *buffer = (float *) _buffer;

    for (int i=0; i<synth->spec.samples; i++) {
        float mix_sample = 0.0;
        for (int ch=0; ch<synth->num_channels; ch++) {
            SND_Channel *chan = synth->channels[ch];
            if (chan->volume != 0.0) {

                float sample = chan->volume * chan->env_vol;
                switch(chan->waveform) {
                    case SND_WAVE_SINE:     sample *= __wave_sine(chan->phase); break;
                    case SND_WAVE_PULSE:    sample *= __wave_pulse(chan->phase, chan->wave_mod); break;
                    case SND_WAVE_TRI:      sample *= __wave_triangle(chan->phase, chan->wave_mod); break;
                    case SND_WAVE_SAW:      sample *= __wave_saw(chan->phase, chan->wave_mod); break;
                    case SND_WAVE_NOISE:    sample *= __wave_noise(chan->phase, chan->wave_mod); break;
                }

                //// Pass sample through filter path
                //for (int i=0; i<MAX_FILTERS; i++) {
                //    SND_Filter *filt = chan->filter_path[i];
                //    if (filt == NULL) continue;
                //    sample = filt->filter_func(sample, filt->amount, filt->udata);
                //}

                mix_sample += sample;

                // Store sample to oscilloscope
                if (chan->osc != NULL && i < chan->osc->rect->w) {
                    chan->osc->sample_buffer[i] = sample;
                }
            
                // Wrap phase over 1.0
                chan->phase += chan->freq / synth->spec.freq;
                if (chan->phase >= 1.0) chan->phase -= 1.0;
            }

            __progress_envelope(synth, chan);
        }
        buffer[i] = mix_sample * synth->volume;
        if (synth->osc != NULL && i < synth->osc->rect->w) synth->osc->sample_buffer[i] = mix_sample;
    }
}

//  Creates a new channel
//  
SND_Channel *__create_channel(SND_Synth *synth) {
    SND_Channel *ch = malloc(sizeof(SND_Channel));

    ch->phase = 0.0;
    ch->freq = 1000.0; // Hz
    ch->volume = 1.0;
    ch->env_vol = 0.0;
    ch->wave_mod = 0.0;
    ch->should_release = false;
    ch->waveform = SND_WAVE_SINE;
    ch->env_values = (struct SND_Envelope){0.0, 0.0, 1.0, 0.0};
    ch->env_state = SND_ENV_HOLD;
    ch->osc = NULL;
    
    return ch;
}

//  Called in the audio callback to progress the envelope one step
//
void __progress_envelope(SND_Synth *synth, SND_Channel *chan) {
    if (chan->should_release) {
        chan->env_state = SND_ENV_RELEASE;
        chan->should_release = false;
    }
    SND_Envelope env = chan->env_values;
    switch (chan->env_state) {
        case SND_ENV_HOLD: break;
         
        case SND_ENV_ATTACK: {
            if (env.attack <= 0.0) chan->env_state = SND_ENV_DECAY;
            chan->env_vol += 1.0 / (synth->spec.freq * env.attack);
            if (chan->env_vol >= 1.0) {
                chan->env_vol = 1.0;
                chan->env_state = SND_ENV_DECAY;
            }
        } break;

        case SND_ENV_DECAY: {
            if (env.decay <= 0.0) chan->env_state = SND_ENV_HOLD;
            chan->env_vol -= (1.1 - env.sustain) / (synth->spec.freq * env.decay);
            if (chan->env_vol <= env.sustain) {
                chan->env_vol = env.sustain;
                chan->env_state = SND_ENV_HOLD;
            }
        } break;

        case SND_ENV_RELEASE: {
            if (chan->env_vol >= 0.0 && env.release > 0.0) {
                chan->env_vol -= env.sustain / (synth->spec.freq * env.release);
            } else {
                chan->env_vol = 0.0;
                chan->env_state = SND_ENV_HOLD;
            }
        } break;
    }
}

//  Render an oscilloscope
//
void __draw_oscilloscope(SND_Synth *synth, SDL_Renderer *renderer, SND_Oscilloscope *osc) {
    SDL_Rect *rect = osc->rect;

    // Clear Oscilloscope
    SDL_SetRenderDrawColor(
        renderer,
        osc->bg_colour.r,
        osc->bg_colour.g,
        osc->bg_colour.b,
        osc->bg_colour.a
    );
    SDL_RenderFillRect(renderer, osc->rect);


    SDL_SetRenderDrawColor(
        renderer,
        osc->beam_colour.r,
        osc->beam_colour.g,
        osc->beam_colour.b,
        osc->beam_colour.a
    );
    float padding = 0.1;
    float sample_amp = (1.0 - padding) * (rect->h/2);
    int centreline = rect->y + rect->h/2;
    float last_sample = 0.0;
    for (int i=0; i<rect->w; i++) {
        float curr = osc->sample_buffer[i] * sample_amp;
    
        if (curr > rect->h/2) curr = rect->h/2;
        if (curr < -rect->h/2) curr = -rect->h/2;
    
        SDL_RenderDrawLine(renderer,
            rect->x + i, centreline - last_sample,
            rect->x + i, centreline - curr
        );

        last_sample = curr;
    }

    if (osc->border_colour.a > 0) {
        SDL_SetRenderDrawColor(
            renderer,
            osc->border_colour.r,
            osc->border_colour.g,
            osc->border_colour.b,
            osc->border_colour.a
        );
        SDL_RenderDrawRect(renderer, osc->rect);
    }
}
