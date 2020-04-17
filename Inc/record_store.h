#ifndef RECORD_STORE_H
#define RECORD_STORE_H

#include <stdint.h>

#include "eeprom.h"

class RecordStore {
public:
    static constexpr uint32_t VERSION_NUM = 1;

    RecordStore(EEPROM<uint16_t, uint8_t> *eeprom);

    bool init();

    bool add_entry(const char *name, void *data, uint16_t size_bytes, uint16_t max_size_bytes);

    bool store_all();
    bool load_all();

    bool first_load();

    bool load(const char *name, bool force_load);
    bool store(const char *name);

private:
    static constexpr uint16_t MAX_BUFFER_SIZE_BYTES = 256;
    static constexpr uint16_t MAX_NUM_ENTRIES = 8;
    static constexpr uint16_t NUM_HEADER_ENTRIES = 1;
    static constexpr uint16_t CRC_SIZE_BYTES = 2;
    static constexpr uint32_t MAGIC_HEADER = 0x4142564D;  // "ABVM"

    EEPROM<uint16_t, uint8_t> *eeprom;

    struct Header {
        uint32_t magic_num;
        uint32_t version;
    };

    Header header;

    struct Entry {
        const char *name;
        void *data;
        uint16_t size;
        bool is_valid;
        uint16_t address;
    };

    Entry entries[MAX_NUM_ENTRIES + NUM_HEADER_ENTRIES];
    uint16_t num_entries;
    uint8_t write_buf[MAX_BUFFER_SIZE_BYTES];

    Entry *find_entry_by_name(const char *name);

    bool load_entry(Entry *entry, bool force_load);
    bool store_entry(Entry *entry);

    void reset_header();
};

#endif
