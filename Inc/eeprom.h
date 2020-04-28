#ifndef EEPROM_H
#define EEPROM_H

template <typename T_addr, typename T_data>
class EEPROM {
  public:
    virtual bool read(T_addr addr, T_data *data, T_addr len) = 0;
    virtual bool write(T_addr addr, T_data *data, T_addr len) = 0;
    virtual bool allocate(T_addr size, T_addr *addr) = 0;
    virtual bool erase() = 0;
};

#endif  // EEPROM_H
