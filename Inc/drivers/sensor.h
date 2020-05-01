#pragma once

class ISensor {
public:
    struct LinearParams {
        // mx + b params
        float m;
        float b;
    };

    virtual float read() = 0;
    virtual ~ISensor() {}
};
