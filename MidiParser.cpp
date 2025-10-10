#include "MidiParser.hpp"
#include <algorithm>

// Read big endian
template<typename T> inline T read(std::ifstream& file) {
	T data = 0;
	for (size_t byte = 0; byte < sizeof(T); ++byte) {
		data = (data << 8) | file.get();
	}
	return data;
}
// Read variable length quantity
inline uint32_t readVLQ(std::ifstream& file) {
	uint32_t value = 0;
	while (true) {
		uint8_t byte = file.get();
		value = (value << 7) | (byte & 0x7F);
		if (byte < 0x80) return value;
	}
}
// Read one byte
inline uint8_t readByte(std::ifstream& file) {
	return file.get();
}

Midi::MidiFile Midi::loadMidi10(const std::string& path) {
    MidiFile mid;

    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
		throw std::runtime_error("Error opening MIDI file.");
    }

    uint32_t MThd = read<uint32_t>(file);
    if (MThd != 0x4D'54'68'64) {
        throw std::runtime_error("Error file is not a MIDI file.");
    }

    uint32_t header_length = read<uint32_t>(file);

    if (header_length != 6) {
        throw std::runtime_error("Unexpected header length");
    }

    uint16_t format =   read<uint16_t>(file);
    uint16_t ntracks =  read<uint16_t>(file);
    uint16_t division = read<uint16_t>(file);
    float ticksPerBeat = 0.5f / (float)division;

    for (uint16_t trackn = 0; trackn < ntracks; ++trackn) {
        uint32_t MTrk = read<uint32_t>(file);
        if (MTrk != 0x4D'54'72'6B) {
            throw std::runtime_error("Error invalid track");
        }

        uint32_t track_length = read<uint32_t>(file);
        std::streampos start = file.tellg();

        uint8_t last_status = 0;
        float currentTime = 0.0f;
        std::vector<int> currentProgram(16, 0);

        std::unordered_map<uint16_t, Note> ongoingNotes{};

        while (true) {
            if (file.tellg() - start == track_length) break;

            uint32_t delta  = readVLQ(file);

            currentTime += (float)delta * ticksPerBeat; // 500000.0 (tempo) * 1e-6 = 0.5

            uint8_t status = readByte(file);

            if (status < 0x80) {
                status = last_status;
            } else if (status != 0xFF) {
                // Meta messages don't set running status
                last_status = status;
            }

            if (status == 0xFF) {
                // Meta message, just skip over it UNLESS it's Set Tempo event because those are important!!!

                uint8_t metatype = readByte(file);
                uint32_t length = readVLQ(file);

				if (metatype == 0x51 && length == 3u) {
					// Set Tempo event // There isn't any uint24_t type so this does the same
					uint32_t tempo = (readByte(file) << 16) | (readByte(file) << 8) | readByte(file);
					ticksPerBeat = ((float)tempo * 1e-6f) / (float)division;
				} else {
					// Other random event, ignore it
					file.seekg(length, std::ios::cur);
				}

            } else if (status == 0xF0 || status == 0xF7) {
                // Sysex message, just skip over it

                uint32_t length = readVLQ(file);
                file.seekg(length, std::ios::cur);

            } else {
                uint8_t statusType = status & 0xF0;

                uint8_t data1 = 0;
                uint8_t data2 = 0;

                if (statusType == 0x80 || statusType == 0x90 || statusType == 0xA0 || statusType == 0xB0 || statusType == 0xE0 || status == 0xF2) {
                    data1 = readByte(file);
                    data2 = readByte(file);
                } else if (statusType == 0xC0 || statusType == 0xD0 || status == 0xF1 || status == 0xF3) {
                    data1 = readByte(file);
                }

                if (statusType == 0x90 && data2 != 0) {
                    // Note on
                    uint8_t channel = status & 0x0F;
                    uint16_t id = ((uint16_t)channel << 8) | (uint16_t)data1;
                    
                    Note note {
                        .time = currentTime,
                        .volume = (float)data2 / 128.0f,
                        .instrument = currentProgram[channel],
                        .note = data1,
                    };

                    ongoingNotes[id] = note;
                } else if (statusType == 0x80 || (statusType == 0x90 && data2 == 0)) {
                    // Note off
                    uint8_t channel = status & 0x0F;
                    uint16_t id = ((uint16_t)channel << 8) | (uint16_t)data1;

                    Note note = ongoingNotes[id];
                    note.duration = currentTime - note.time;
                    ongoingNotes.erase(id);

                    mid.notes.push_back(note);
                } else if (statusType == 0xC0) {
                    // Program change
                    uint8_t channel = status & 0x0F;
                    currentProgram[channel] = data1;
                }
            }
        }
    }

	std::sort(mid.notes.begin(), mid.notes.end(), [](const Midi::Note& a, const Midi::Note& b) {
		return a.time < b.time;
	});
    return mid;
};