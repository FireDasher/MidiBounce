#include "MidiParser.hpp"
#include <algorithm>
#include <array>

// Read big endian
inline uint32_t read(std::ifstream& file, int size) {
	uint32_t data = 0;
	for (int byte = 0; byte < size; ++byte) {
		data = (data << 8) | file.get();
	}
	return data;
}
// Read variable length quantity
inline uint32_t readVLQ(std::ifstream& file) {
	uint32_t value = 0;
	while (true) {
		int byte = file.get();
		value = (value << 7) | (byte & 0x7F);
		if (byte < 0x80) return value;
	}
}
// Read one byte
inline int readByte(std::ifstream& file) {
	return file.get();
}

std::vector<Midi::Note> Midi::loadMidi10(const std::string& path) {
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
		throw std::runtime_error("Error opening MIDI file.");
    }

    uint32_t MThd = read(file, 4);
    if (MThd != 0x4D'54'68'64) {
        throw std::runtime_error("Error file is not a MIDI file.");
    }

    uint32_t header_length = read(file, 4);

    if (header_length != 6) {
        throw std::runtime_error("Unexpected header length");
    }
    
    uint32_t format =   read(file, 2);
    uint32_t ntracks =  read(file, 2);
    uint32_t division = read(file, 2);

    std::vector<Message> messages{};

    for (uint32_t trackn = 0; trackn < ntracks; ++trackn) {
        uint32_t MTrk = read(file, 4);
        if (MTrk != 0x4D'54'72'6B) {
            throw std::runtime_error("Error invalid track");
        }

        uint32_t track_length = read(file, 4);
        std::streampos start = file.tellg();

        int last_status = 0;
        uint32_t currentTime = 0.0f;

        while (true) {
            if (file.tellg() - start == track_length) break;

            uint32_t delta  = readVLQ(file);

            currentTime += delta; // 500000.0 (tempo) * 1e-6 = 0.5

            int status = readByte(file);

            if (status < 0x80) {
                status = last_status;
            } else if (status != 0xFF) {
                // Meta messages don't set running status
                last_status = status;
            }

            if (status == 0xFF) {
                // Meta message, just skip over it UNLESS it's Set Tempo event because those are important!!!

                int metatype = readByte(file);
                uint32_t length = readVLQ(file);

				if (metatype == 0x51 && length == 3u) {
					// Set Tempo event
					int tempo = read(file, 3);
                    // std::cout << "Tempo: " << 6e7f / tempo << std::endl;
					// Formula: ticksToSeconds = (tempo * 1e-6f) / division;
                    messages.emplace_back(Message{'=', currentTime, tempo, 64, 0});
				} else {
					// Other random event, ignore it
					file.seekg(length, std::ios::cur);
				}

            } else if (status == 0xF0 || status == 0xF7) {
                // Sysex message, just skip over it

                uint32_t length = readVLQ(file);
                file.seekg(length, std::ios::cur);

            } else {
                int statusType = status & 0xF0;

                int data1 = 0;
                int data2 = 0;

                if (statusType == 0x80 || statusType == 0x90 || statusType == 0xA0 || statusType == 0xB0 || statusType == 0xE0 || status == 0xF2) {
                    data1 = readByte(file);
                    data2 = readByte(file);
                } else if (statusType == 0xC0 || statusType == 0xD0 || status == 0xF1 || status == 0xF3) {
                    data1 = readByte(file);
                }

                if (statusType == 0x90 && data2 != 0) {
                    // Note on
                    int channel = status & 0x0F;
                    messages.emplace_back(Message{'+', currentTime, data1, data2, channel});

                } else if (statusType == 0x80 || (statusType == 0x90 && data2 == 0)) {
                    // Note off
                    int channel = status & 0x0F;
                    messages.emplace_back(Message{'-', currentTime, data1, data2, channel});
                    
                } else if (statusType == 0xC0) {
                    // Program change
                    int channel = status & 0x0F;
                    messages.emplace_back(Message{'*', currentTime, data1, 64, channel});
                }
            }
        }
    }

	std::sort(messages.begin(), messages.end(), [](const Message& a, const Message& b) {
		return a.time < b.time;
	});
    
    std::vector<Note> notes;

    std::unordered_map<int, Note> ongoingNotes;
    std::array<int, 16> currentPrograms{};
    float ticks2seconds = 0.5f / division;
    float currentTime = 0.0f;
    uint32_t lastTime = 0;

    for (int i = 0; i < messages.size(); ++i) {
        const Message& msg = messages[i];
        currentTime += (msg.time - lastTime) * ticks2seconds;
        lastTime = msg.time;
        if (msg.type == '=') {
            ticks2seconds = (msg.note * 1e-6f) / division;
            // std::cout << "Found set tempo event for " << 6e7f / msg.note << " BPM At " << currentTime << std::endl;
        } else if (msg.type == '*') {
            currentPrograms[msg.channel] = msg.note;
        } else if (msg.type == '+') {
            int id = (msg.channel << 8) | msg.note;
            ongoingNotes[id] = Note{
                .time = currentTime,
                .volume = msg.velocity / 128.0f,
                .instrument = currentPrograms[msg.channel],
                .note = msg.note
            };
        } else if (msg.type == '-') {
            int id = (msg.channel << 8) | msg.note;
            Note note = ongoingNotes[id];
            note.duration = currentTime - note.time;
            ongoingNotes.erase(id);
            notes.emplace_back(note);
        }
    }

    return notes;
};