#include "record_store.h"

#include <assert.h>
#include <string.h>

#include "crc16.h"

RecordStore::RecordStore(EEPROM<uint16_t, uint8_t> *eeprom) : eeprom(eeprom), num_entries(0) {}

bool RecordStore::init() {
    return add_entry("header", &header, sizeof(header), sizeof(header));
}

bool RecordStore::add_entry(const char *name, void *data, uint16_t size_bytes, uint16_t max_size_bytes) {
    assert(name != nullptr);
    assert(data != nullptr);
    assert(max_size_bytes <= MAX_BUFFER_SIZE_BYTES + 2);
    assert(0 < size_bytes && size_bytes <= max_size_bytes);
    assert(num_entries < MAX_NUM_ENTRIES);

    Entry *entry = &entries[num_entries];
    entry->name = name;
    entry->data = data;
    entry->size = size_bytes;
    entry->is_valid = false;

    bool success = eeprom->allocate(max_size_bytes + CRC_SIZE_BYTES, &entry->address);

    if (!success) {
        return false;
    }

    num_entries++;
    return true;
}

bool RecordStore::store_all() {
    for (uint16_t i = 0; i < num_entries; i++) {
        if (!store_entry(&entries[i])) {
            return false;
        }
    }
    return true;
}

bool RecordStore::load_all() {
    for (uint16_t i = 0; i < num_entries; i++) {
        if (!load_entry(&entries[i], false)) {
            return false;
        }
    }
    return true;
}

bool RecordStore::first_load() {
    bool success = load("header", true);

    // The CRC is not valid AND the magic header has been corrupted. Assume this means
    // the memory has been erased or never programmed. Program defaults
    if (header.magic_num != MAGIC_HEADER) {
        reset_header();
        success = store_all();
        return success;
    } else if (!success) {
        return false;
    } else {
        return load_all();
    }
}

bool RecordStore::load(const char *name, bool force_load) {
    Entry *entry = find_entry_by_name(name);

    if (entry == nullptr) {
        return false;
    }

    return load_entry(entry, force_load);
}

bool RecordStore::store(const char *name) {
    Entry *entry = find_entry_by_name(name);

    if (entry == nullptr) {
        return false;
    }

    return store_entry(entry);
}

RecordStore::Entry *RecordStore::find_entry_by_name(const char *name) {
    for (uint16_t i = 0; i < num_entries; i++) {
        if (strcmp(name, entries[i].name) == 0) {
            return &entries[i];
        }
    }
    return nullptr;
}

bool RecordStore::load_entry(Entry *entry, bool force_load) {
    bool success = eeprom->read(entry->address, write_buf, entry->size);
    if (!success) {
        return false;
    }

    uint16_t read_crc;
    success = eeprom->read(entry->address + entry->size, (uint8_t *)&read_crc, CRC_SIZE_BYTES);
    if (!success) {
        return false;
    }

    uint16_t crc = CRC16::calc(write_buf, entry->size);
    entry->is_valid = (crc == read_crc);

    if (entry->is_valid || force_load) {
        memcpy(entry->data, write_buf, entry->size);
    }

    return entry->is_valid;
}

bool RecordStore::store_entry(Entry *entry) {
    bool success = eeprom->write(entry->address, (uint8_t *)entry->data, entry->size);
    if (!success) {
        return false;
    }

    uint16_t crc = CRC16::calc((uint8_t *)entry->data, entry->size);
    success = eeprom->write(entry->address + entry->size, (uint8_t *)&crc, CRC_SIZE_BYTES);

    return success;
}

void RecordStore::reset_header() {
    header.magic_num = MAGIC_HEADER;
    header.version = VERSION_NUM;
}
