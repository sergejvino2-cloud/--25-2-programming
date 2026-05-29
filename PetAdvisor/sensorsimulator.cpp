// SensorSimulator.cpp
#include "SensorSimulator.h"
#include <algorithm>

SensorSimulator::SensorSimulator(double waterCapacityML, double foodCapacityG)
    : m_waterCapacity(waterCapacityML), m_foodCapacity(foodCapacityG),
      m_waterLevel(waterCapacityML), m_foodLevel(foodCapacityG),
      m_rng(static_cast<unsigned>(
          std::chrono::steady_clock::now().time_since_epoch().count())) {}

SensorData SensorSimulator::GetLatestData() {
    SensorData data;
    data.water_ml = m_waterLevel;
    data.food_g   = m_foodLevel;
    data.total_g  = m_waterLevel + m_foodLevel;
    data.valid    = true;
    return data;
}

double SensorSimulator::GetTemperature(const Pet& pet) const {
    auto [minT, maxT] = pet.GetTemperatureRange();
    std::uniform_real_distribution<double> baseDist(minT + 0.1, maxT - 0.1);
    double baseTemp = baseDist(m_rng);
    std::normal_distribution<double> noise(0.0, 0.2);
    double temp = baseTemp + noise(m_rng);

    std::uniform_real_distribution<double> outChance(0.0, 1.0);
    if (outChance(m_rng) < 0.05) {
        if (outChance(m_rng) < 0.5)
            temp = maxT + 0.5 + std::abs(noise(m_rng)) * 1.5;
        else
            temp = minT - 0.5 - std::abs(noise(m_rng)) * 1.5;
    }
    return std::round(temp * 10.0) / 10.0;
}

void SensorSimulator::Update(double elapsedSeconds) {
    if (elapsedSeconds <= 0.0) return;
    std::uniform_real_distribution<double> waterRateDist(0.5, 1.5);
    std::uniform_real_distribution<double> foodRateDist(0.2, 0.6);
    m_waterLevel -= waterRateDist(m_rng) * elapsedSeconds;
    m_foodLevel  -= foodRateDist(m_rng) * elapsedSeconds;
    m_waterLevel = std::max(0.0, m_waterLevel);
    m_foodLevel  = std::max(0.0, m_foodLevel);
    if (m_waterLevel <= 0.01) m_waterLevel = m_waterCapacity * 0.9;
    if (m_foodLevel  <= 0.01) m_foodLevel  = m_foodCapacity  * 0.9;
}
