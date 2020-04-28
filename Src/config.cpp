#include "config.h"

ConfigCommandRPC::ConfigCommandRPC(uint8_t id, RecordStore *store) :
    CommEndpoint(id, NULL, sizeof(uint32_t)), store(store) {}

uint8_t ConfigCommandRPC::write(void *data, size_t size) {
    uint32_t *cmd_in = (uint32_t*)data;
    switch (*cmd_in) {
        case CONFIG_SAVE_CMD: {
            store->store_all();
            break;
        }
        case CONFIG_ERASE_CMD: {
            store->get_eeprom()->erase();
            break;
        }
        case CONFIG_LOAD_CMD: {
            store->load_all();
            break;
        }
        case CONFIG_RESET_CMD: {
            store->get_eeprom()->erase();
            store->first_load();
            break;
        }
        default: {
            return INVALID_CMD_ERR;
            break;
        }
    }
}

uint8_t ConfigCommandRPC::read(void *data, size_t size) {
    // this is a write-only endpoint, so error on read
    return (uint8_t)CommError::ERROR_READ;
}
