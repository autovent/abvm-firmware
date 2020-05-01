#pragma once

class ISensor {
public:
    virtual float read() = 0;
    virtual ~ISensor() {}
};
