#pragma once

#include <fstream>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

namespace Midi {
    // MIDI note
    struct Note {
        // This note_on message's absolute time
        float time = 0.0f;
        // Time between this note_on message and the corrosponding note_off message
        float duration = 0.0f;
        // This note_on message's velocity divided by 128
        float volume = 0.0f;
        // The program of the last program_change message before this note_on message
        int instrument = 0;
        // This note_on message's note
        int note = 0;

        // A function for printing the note
        std::string str() {
            return "MidiNote{{ Time: " + std::to_string(time) + ", Duration: " + std::to_string(duration) + ", Volume: " + std::to_string(volume) + ", Instrument: " + std::to_string(instrument) + ", Note: " + std::to_string(note) + " }}";
        }
    };
	
    // A MIDI file
    struct MidiFile {
        std::vector<Note> notes;
    };

    // A function that loads a MIDI 1.0 file
    MidiFile loadMidi10(const std::string& path);
}