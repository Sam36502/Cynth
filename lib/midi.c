#include "midi.h"

//
//  Internal Functions
//

static void _err_msg(const char *filename, const char *msg) {
    #ifdef MIDI_PRINT_ERR_MSGS
        printf("[ERROR] Failed to parse midi file '%s':\n  %s\n", filename, msg);
        fflush(stdout);
    #endif
}

static float _midi_to_freq(Uint8 note_num) {
    float freq = MIDI_TUNING / 16; // 440 (A4) / 2^4 -> (A0)
    for (int i=21; i<note_num; i++) {
        freq *= MIDI_SEMITONE;
    }
    return freq;
}

// Parse Variable-Length-Quantities
static Uint32 _parse_vlq(FILE *f, Uint8 byte) {
    Uint32 num = (byte & 0b01111111);
    while ((byte & 0b10000000) && !feof(f)) {
        num <<= 7;
        byte = fgetc(f);
        num |= (byte & 0b01111111);
    }
    return num;
}

static int num_voices;
static int *voice_assign;

static void _handle_midi_event(MIDI_MidiEvent event, SND_Synth *synth) {
    Uint8 note = event.data[0];
    Uint8 vel = event.data[1];

    int voice_index = -1;
    int first_free = -1;
    for (int i=0; i<num_voices; i++) {
        if (voice_assign[i] == -1) {
            if (first_free == -1) first_free = i;
            continue;
        }
        if (voice_assign[i] == note) {
            voice_index = i;
            break;
        }
    }
    if (voice_index == -1 && first_free != -1) voice_index = first_free;

    switch (event.status) {
        case MIDI_STATUS_NOTE_OFF: {
            #ifdef MIDI_PRINT_INFO_MSGS
            printf("[MIDI](V-%02i) Note Off Event: Note %i, Velocity %i\n", voice_index, note, vel);
            #endif
            if (voice_index != -1) {
                SND_ReleaseChannelEnvelope(synth, voice_index);
                voice_assign[voice_index] = -1;
            }
        } break;

        case MIDI_STATUS_NOTE_ON: {
            if (voice_index != -1) {
                if (vel > 0) {
                    #ifdef MIDI_PRINT_INFO_MSGS
                    printf("[MIDI](V-%02i) Note On  Event: Note %i, Velocity %i\n", voice_index, note, vel);
                    #endif
                    SND_SetChannelFrequency(synth, voice_index, _midi_to_freq(note));
                    SND_SetChannelVolume(synth, voice_index, ((float) vel) / 128);
                    SND_TriggerChannelEnvelope(synth, voice_index);
                    voice_assign[voice_index] = note;
                } else {
                    #ifdef MIDI_PRINT_INFO_MSGS
                    printf("[MIDI](V-%02i) Note Off Event: Note %i, Velocity %i\n", voice_index, note, vel);
                    #endif
                    SND_ReleaseChannelEnvelope(synth, event.channel);
                    voice_assign[voice_index] = -1;
                }
            }
        } break;

        case MIDI_STATUS_POLY_PRESS: break; // Unimplemented
        case MIDI_STATUS_CC: break; // Unimplemented
        case MIDI_STATUS_PROG_CHANGE: break; // Unimplemented
        case MIDI_STATUS_CHAN_PRESS: break; // Unimplemented
        case MIDI_STATUS_PITCH_BEND: break; // Unimplemented
    }

    fflush(stdout);
}

static int _play_track_thread(void *_data) {
    struct {
        MIDI_File *midi;
        SND_Synth *synth;
        Uint16 track_nr;
    } *data = _data;
    MIDI_File *midi = data->midi;
    SND_Synth *synth = data->synth;
    Uint16 track_nr = data->track_nr;
    free(_data);

    Uint8 event_index;

    #define CURR_TRACK midi->tracks[track_nr]
    #define CURR_EVENT CURR_TRACK.events[event_index]

    for (event_index=0; event_index<CURR_TRACK.count; event_index++) {
        if (CURR_EVENT.delta_time > 0) {
            SDL_Delay(
                CURR_EVENT.delta_time * (midi->uspq / midi->header.ppq) / 1000
            );
        }

        if (CURR_EVENT.type == MIDI_EVENT_TYPE_MIDI)
            _handle_midi_event(CURR_EVENT.midi, synth);
        
        if (CURR_EVENT.type == MIDI_EVENT_TYPE_META) {
            char *buf = malloc(sizeof(char) * (CURR_EVENT.meta.length + 1));
            buf[sizeof(char) * CURR_EVENT.meta.length] = '\0';
            SDL_memcpy(buf, CURR_EVENT.meta.data, sizeof(char) * CURR_EVENT.meta.length);

            switch (CURR_EVENT.meta.type) {
                case MIDI_META_SEQ_NUM: break; // Unimplemented / TODO ?

                #ifdef MIDI_PRINT_INFO_MSGS
                case MIDI_META_TXT: printf("[META] Text Event: \"%s\"\n", buf); break;
                case MIDI_META_COPYRIGHT: printf("[META] Copyright Notice: \"%s\"\n", buf); break;
                case MIDI_META_SEQ_NAME: printf("[META] Sequence Name: \"%s\"\n", buf); break;
                case MIDI_META_INS_NAME: printf("[META] Instrument Name: \"%s\"\n", buf); break;
                case MIDI_META_LYRIC: printf("[META] Lyric: \"%s\"\n", buf); break;
                case MIDI_META_MARKER: printf("[META] Marker: \"%s\"\n", buf); break;
                case MIDI_META_CUE_PT: printf("[META] Cue Point: \"%s\"\n", buf); break;
                #endif

                case MIDI_META_CH_PREFIX: break; // Unimplemented
                case MIDI_META_EOT: break; // Already handled in parsing
                case MIDI_META_SET_TEMPO: {
                    midi->uspq = 0;
                    midi->uspq |= CURR_EVENT.meta.data[0]; midi->uspq <<= 8;
                    midi->uspq |= CURR_EVENT.meta.data[1]; midi->uspq <<= 8;
                    midi->uspq |= CURR_EVENT.meta.data[2];
                    float tempo = 1 / (((float) midi->uspq) / 1000000) * 60;
                    #ifdef MIDI_PRINT_INFO_MSGS
                    printf("[META] Changed tempo to %0.2f BPM\n", tempo);
                    #endif
                } break;
                case MIDI_META_SMPTE_OFFS: break; // Unimplemented
                case MIDI_META_TIME_SIG: break; // Unimplemented
                case MIDI_META_KEY_SIG: break; // Unimplemented
                case MIDI_META_SEQSPEC: break; // Unimplemented / TODO?
            }

            fflush(stdout);
            free(buf);
        }
    }

    #undef CURR_TRACK
    #undef CURR_EVENT

    return 0;
}

static int _play_file_thread(void *_data) {
    MIDI_Play_Thread_Data *data = (MIDI_Play_Thread_Data *) _data;
    MIDI_File *midi = data->midi;
    SND_Synth *synth = data->synth;
    free(_data);

    num_voices = 16;
    voice_assign = malloc(sizeof(Uint8) * num_voices);
    for (int i=0; i<num_voices; i++) voice_assign[i] = -1;

    // TODO: Make a thread for each track if format is simultaneous
    if (midi->header.format == MIDI_FMT_SIMULTANEOUS) {
        for (int i=0; i<midi->header.num_tracks; i++) {
            struct {
                MIDI_File *midi;
                SND_Synth *synth;
                Uint16 track_nr;
            } *track_data = malloc(sizeof(MIDI_File *) + sizeof(SND_Synth *) + sizeof(Uint16));
            track_data->midi = midi;
            track_data->synth = synth;
            track_data->track_nr = i;
        
            SDL_CreateThread(_play_track_thread, "", track_data);
        }
    } else {
        // TODO or L + Ratio + No Implementation
    }

    free(voice_assign);
    
    #ifdef MIDI_PRINT_INFO_MSGS
    puts("[MIDI] Finished playing.");
    fflush(stdout);
    #endif

    return 0;
}

//
//  Public Functions
//

MIDI_File *MIDI_OpenFile(const char *filename) {
    MIDI_File *midi = malloc(sizeof(MIDI_File));
    FILE *file = fopen(filename, "r");
    int index = 0;

    enum parse_state {
        TYPE,
        SIZE,
        FORMAT,
        N_TRACKS,
        DIVISION,
        TRACK_TYPE,
        TRACK_LENGTH,
        DELTA_TIME,
        EVENT_STATUS,
        META_EVENT,
        MIDI_EVENT,
        DONE,
    } state = TYPE;

    int track_index = -1;
    int event_index = -1;
    Uint8 running_status = 0x00;

    #define CURR_TRACK midi->tracks[track_index]
    #define CURR_EVENT CURR_TRACK.events[event_index]

    while (state != DONE && !feof(file)) {
        Uint8 byte = fgetc(file);

        switch (state) {

            case TYPE: {
                if (byte != MIDI_CHTP_HEADER[index]) {
                    _err_msg(filename, 
                        "Header-Type bytes are invalid. Is this a MIDI file?"
                    );
                    fclose(file);
                    return NULL;
                }
                if (index == 3) {
                    state = SIZE;
                    index = -1;
                }
            } break;

            case SIZE: {
                if (byte != MIDI_CHSZ_HEADER[index]) {
                    _err_msg(filename, 
                        "Header size is invalid."
                    );
                    fclose(file);
                    free(midi);
                    return NULL;
                }
                if (index == 3) {
                    state = FORMAT;
                    index = -1;
                }
            } break;

            case FORMAT: {
                if (index == 0) break;
                midi->header.format = byte;
                state = N_TRACKS;
                index = -1;
            } break;

            case N_TRACKS: {
                if (index == 0) {
                    midi->header.num_tracks = byte;
                    midi->header.num_tracks <<= 8;
                } else {
                    midi->header.num_tracks |= byte;
                    midi->tracks = malloc(sizeof(MIDI_Track) * midi->header.num_tracks);
                    state = DIVISION;
                    index = -1;
                }
            } break;

            case DIVISION: {
                if (index == 0) {
                    if (byte & 0b10000000) {
                        _err_msg(filename,
                            "Provided delta-time format is not supported, sorry"
                        );
                        fclose(file);
                        free(midi);
                        return NULL;
                    }
                    midi->header.ppq = byte;
                    midi->header.ppq <<= 8;
                } else {
                    midi->header.ppq |= byte;
                    state = TRACK_TYPE;
                    index = -1;
                }
            } break;

            case TRACK_TYPE: {
                if (byte != MIDI_CHTP_TRACK[index]) {
                    _err_msg(filename, 
                        "Invalid Track-Type"
                    );
                    return NULL;
                }
                if (index == 3) {
                    track_index++;
                    CURR_TRACK.count = 0;
                    CURR_TRACK.length = 0;
                    CURR_TRACK.capacity = sizeof(MIDI_Event) * MIDI_EVENT_BUFFER_SIZE;
                    CURR_TRACK.events = malloc(CURR_TRACK.capacity);
                    event_index = -1;
                    state = TRACK_LENGTH;
                    index = -1;
                }
            } break;

            case TRACK_LENGTH: {
                CURR_TRACK.length |= byte;

                if (index < 3) {
                    CURR_TRACK.length <<= 8;
                } else {
                    state = DELTA_TIME;
                    index = -1;
                }
            } break;

            case DELTA_TIME: {
                // Expand event buffer if required
                if (event_index == MIDI_EVENT_BUFFER_SIZE - 1) {
                    CURR_TRACK.capacity += sizeof(MIDI_Event) * MIDI_EVENT_BUFFER_SIZE;
                    CURR_TRACK.events = realloc(CURR_TRACK.events, CURR_TRACK.capacity);
                }

                event_index++;
                CURR_TRACK.count++;
                CURR_EVENT.delta_time = _parse_vlq(file, byte);
                state = EVENT_STATUS;
                index = -1;
            } break;

            case EVENT_STATUS: {
                if (byte & 0b10000000) running_status = byte;

                if ((running_status & 0b11110000) == 0xF0) {
                    if (running_status == 0xFF) {
                        CURR_EVENT.type = MIDI_EVENT_TYPE_META;
                        state = MIDI_EVENT;
                    } else {
                        CURR_EVENT.type = MIDI_EVENT_TYPE_SYSEX;
                        CURR_EVENT.meta.type = running_status;
                        state = META_EVENT;
                    }
                } else {
                    CURR_EVENT.type = MIDI_EVENT_TYPE_MIDI;
                    CURR_EVENT.midi.channel = (running_status & 0b00001111);
                    CURR_EVENT.midi.status = (running_status & 0b11110000);
                    state = MIDI_EVENT;
                }

                if (byte & 0b10000000) {
                    index = -1;
                    break;
                }
                index = 0;
            } // Fallthrough to parse data if using running status
            case MIDI_EVENT: {
                if (CURR_EVENT.type == MIDI_EVENT_TYPE_MIDI) {
                
                    int length = 2;
                    if (CURR_EVENT.midi.status == 0xC0 || CURR_EVENT.midi.status == 0xD0) {
                        length = 1;
                    }

                    CURR_EVENT.midi.data[index] = byte;
                    if (index == length - 1) {
                        state = DELTA_TIME;
                        index = -1;
                    }
                }
            }
            case META_EVENT: {
                if (CURR_EVENT.type == MIDI_EVENT_TYPE_META) {
                    if (index == 0) {
                        CURR_EVENT.meta.type = byte;
                    }
                    if (index == 1) {
                        CURR_EVENT.meta.length = _parse_vlq(file, byte);

                        // Handle End-of-Track
                        if (CURR_EVENT.meta.type == MIDI_META_EOT) {
                            CURR_TRACK.count = event_index + 1;
                            
                            if (track_index == midi->header.num_tracks - 1) {
                                state = DONE;
                                break;
                            } else state = TRACK_TYPE;
                            index = -1;
                        }

                        CURR_EVENT.meta.data = malloc(sizeof(Uint8) * CURR_EVENT.meta.length);
                    }
                    if (index > 1) {
                        int i = index - 2;
                        CURR_EVENT.meta.data[i] = byte;
                        if (i == CURR_EVENT.meta.length - 1) {
                            state = DELTA_TIME;
                            index = -1;
                        }
                    }
                }
            } break;

            case DONE: break;
        }
        index++;
    }

    #undef CURR_TRACK
    #undef CURR_EVENT

    // Set default usec/quarter-note (120 BPM -> 500000);
    midi->uspq = 500000;

    fclose(file);

    return midi;
}

void MIDI_CloseFile(MIDI_File *midi) {
    if (midi == NULL) return;
    for (int i=0; i<midi->header.num_tracks; i++) {
        for (int j=0; j<midi->tracks[i].count; j++) {
            if (
                midi->tracks[i].events[j].type == MIDI_EVENT_TYPE_META
                && midi->tracks[i].events[j].meta.length > 0
            ) free(midi->tracks[i].events[j].meta.data);
        }
        free(midi->tracks[i].events);
    }
    free(midi->tracks);
    free(midi);
}

void MIDI_PlayFile(MIDI_File *midi, SND_Synth *synth) {
    MIDI_Play_Thread_Data *data = malloc(sizeof(MIDI_Play_Thread_Data));
    data->midi = midi;
    data->synth = synth;

    void *res = SDL_CreateThread(&_play_file_thread, "MIDI_Thread", data);
    if (res == NULL) {
        printf("[ERROR] Failed to start MIDI-Playing Thread: %s\n", SDL_GetError());
    }
}

const char *MIDI_GetFormatName(MIDI_Format fmt) {
    switch (fmt) {
        case MIDI_FMT_SINGLE: return "Single Track";
        case MIDI_FMT_SIMULTANEOUS: return "Simultaneous Tracks";
        case MIDI_FMT_SEQUENTIAL: return "Sequential Tracks";
    }
    return "";
}