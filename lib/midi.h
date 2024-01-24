#ifndef MIDI_H
#define MIDI_H

/*
 *      Basic MIDI 1.0 Implementation
 *      for my synth library
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "synth.h"

//
//  Constant Definitions
//

#define MIDI_TUNING 440.0   // Tuning of A4 in Hertz
#define MIDI_A0 21          // MIDI Note Number of A0 (lowest supported note)
#define MIDI_SEMITONE 1.05946309436
#define MIDI_DEFAULT_TEMPO 120
#define MIDI_EVENT_BUFFER_SIZE 0xFF

#define MIDI_PRINT_ERR_MSGS // Comment out to hide error messages
#define MIDI_PRINT_INFO_MSGS // Comment out to hide info/debug messages

static const Uint8 MIDI_CHTP_HEADER[4] = { 'M', 'T', 'h', 'd' };
static const Uint8 MIDI_CHTP_TRACK[4]  = { 'M', 'T', 'r', 'k' };
static const Uint8 MIDI_CHSZ_HEADER[4] = { 0x00, 0x00, 0x00, 0x06 };


//
//  Type Definitions
//

typedef enum MIDI_Format {
    MIDI_FMT_SINGLE,        // File contains a single multi-channel track
    MIDI_FMT_SIMULTANEOUS,  // File contains one or more simultaneous tracks of a sequence
    MIDI_FMT_SEQUENTIAL,    // file contains one or more sequential single-track patterns
} MIDI_Format;

typedef enum MIDI_EventType {
    MIDI_EVENT_TYPE_MIDI,
    MIDI_EVENT_TYPE_SYSEX,
    MIDI_EVENT_TYPE_META,
} MIDI_EventType;

typedef enum MIDI_VoiceStatus {
    MIDI_STATUS_NOTE_OFF    = 0x80,
    MIDI_STATUS_NOTE_ON     = 0x90,
    MIDI_STATUS_POLY_PRESS  = 0xA0,
    MIDI_STATUS_CC          = 0xB0,
    MIDI_STATUS_PROG_CHANGE = 0xC0,
    MIDI_STATUS_CHAN_PRESS  = 0xD0,
    MIDI_STATUS_PITCH_BEND  = 0xE0,
} MIDI_VoiceStatus;

typedef enum MIDI_MetaEventType {
    MIDI_META_SEQ_NUM       = 0x00,
    MIDI_META_TXT           = 0x01,
    MIDI_META_COPYRIGHT     = 0x02,
    MIDI_META_SEQ_NAME      = 0x03,
    MIDI_META_INS_NAME      = 0x04,
    MIDI_META_LYRIC         = 0x05,
    MIDI_META_MARKER        = 0x06,
    MIDI_META_CUE_PT        = 0x07,
    MIDI_META_CH_PREFIX     = 0x20,
    MIDI_META_EOT           = 0x2F,
    MIDI_META_SET_TEMPO     = 0x51,
    MIDI_META_SMPTE_OFFS    = 0x54, // SMPTE is not currently supported
    MIDI_META_TIME_SIG      = 0x58,
    MIDI_META_KEY_SIG       = 0x59,
    MIDI_META_SEQSPEC       = 0x7F, // Sequencer-Specific Meta event. Can basically be anything
} MIDI_MetaEventType;

typedef struct MIDI_MidiEvent {
    MIDI_VoiceStatus status;
    Uint8 channel;
    Uint8 data[2];
} MIDI_MidiEvent;

typedef struct MIDI_MetaEvent {
    MIDI_MetaEventType type;
    Uint32 length;
    Uint8 *data;
} MIDI_MetaEvent;

typedef struct MIDI_Event {
    MIDI_EventType type;
    Uint32 delta_time;
    union {
        MIDI_MidiEvent midi;
        MIDI_MetaEvent meta;
    };
} MIDI_Event;

typedef struct MIDI_Track {
    Uint32 length;
    Uint32 count;
    Uint32 capacity;
    MIDI_Event *events;
} MIDI_Track;

typedef struct MIDI_Header {
    MIDI_Format format;
    Uint16 num_tracks;
    Uint16 ppq;   // Clock ticks per Quarter-Note
} MIDI_Header;

typedef struct MIDI_File {
    Uint32 uspq;
    MIDI_Header header;
    MIDI_Track *tracks;
} MIDI_File;

typedef struct MIDI_Play_Thread_Data {
    MIDI_File *midi;
    SND_Synth *synth;
} MIDI_Play_Thread_Data;


//
//  Public Functions
//

//  Opens a MIDI file for reading
//
//  Returns NULL if an error is encountered.
//  Prints errors when encountered if `MIDI_PRINT_ERR_MSGS` is defined
MIDI_File *MIDI_OpenFile(const char *filename);

//  Closes a previously opened MIDI file
//
void MIDI_CloseFile(MIDI_File *midi);

//  Plays a MIDI file through the given Synth
//
void MIDI_PlayFile(MIDI_File *midi, SND_Synth *synth);

//  Gets the name of a MIDI File format
//
const char *MIDI_GetFormatName(MIDI_Format fmt);

#endif