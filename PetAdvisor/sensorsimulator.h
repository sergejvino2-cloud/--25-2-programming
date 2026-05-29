// SensorSimulator.h
#pragma once
#include "ISensorProvider.h"
#include "Pet.h"
#include <random>
#include <chrono>

class SensorSimulator : public ISensorProvider {
public:
    SensorSimulator(double waterCapacityML = 500.0, double foodCapacityG = 400.0);

    SensorData GetLatestData() override;
    bool IsConnected() const override { return true; }
    bool SendCommand(const std::string& cmd) override { return false; }

    double GetTemperature(const Pet& pet) const;
    void Update(double elapsedSeconds = 1.0);

private:
    double m_waterCapacity;
    double m_foodCapacity;
    double m_waterLevel;
    double m_foodLevel;
    mutable std::mt19937 m_rng;
};
