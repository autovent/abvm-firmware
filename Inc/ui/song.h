#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <array>

enum class Tone { REST, C7, E7, G7, C8 };

struct SongEntry {
    Tone tone;          // Tone to play
    float volume;       // 0 to 1 volume
    uint32_t duration;  // duration in terms of beats
};

class Song {
public:
    Song(SongEntry *e, size_t num_entries)
        : entries(e),
          num_entries(num_entries),
          is_looping(false),
          is_running(false),
          timestamp_ms(millis()),
          current_entry(0) {}
    Song(SongEntry e[], size_t num_entries)
        : entries(e),
          num_entries(num_entries),
          is_looping(false),
          is_running(false),
          timestamp_ms(millis()),
          current_entry(0) {}
    virtual ~Song() {}

    virtual SongEntry const &update() {
        static SongEntry zero = {Tone::REST, 0, 0};
        if (is_running) {
            if (time_since_ms(timestamp_ms) > entries[current_entry].duration) {
                timestamp_ms = millis();
                if (++current_entry >= num_entries) {
                    current_entry = 0;
                    is_running = is_looping;
                }
            }
            return entries[current_entry];
        } else {
            return zero;
        }
    }

    virtual void start() {
        is_running = true;
        timestamp_ms = millis();
        current_entry = 0;
    }

    virtual void stop() {
        is_running = false;
    }

    virtual void loop() {
        is_looping = true;
    }

    virtual bool get_is_running() {
        return is_running;
    }

private:
    SongEntry *entries;
    size_t num_entries;

    bool is_looping;
    bool is_running;
    uint32_t timestamp_ms;
    uint32_t current_entry;
};

template <size_t N>
class FixedSong : public Song {
public:
    FixedSong(std::initializer_list<SongEntry> &&list) : Song(data.data(), N) {
        assert(list.size() == N);

        uint32_t i = 0;
        for (SongEntry const &entry : list) {
            data[i++] = entry;
        }
    }

    virtual ~FixedSong() {}

private:
    std::array<SongEntry, N> data;
};
