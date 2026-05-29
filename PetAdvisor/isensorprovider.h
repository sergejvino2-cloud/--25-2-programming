#pragma once

struct SensorData {
    double water_ml = 0.0;   // вес поилки в граммах ≈ мл
    double food_g   = 0.0;   // вес корма в граммах
    double total_g  = 0.0;   // общий вес (опционально)
    bool   valid    = false;
};

class ISensorProvider {
public:
    virtual ~ISensorProvider() = default;
    virtual SensorData GetLatestData() = 0;
    virtual bool IsConnected() const = 0;
    virtual bool SendCommand(const std::string& cmd) = 0;
};
