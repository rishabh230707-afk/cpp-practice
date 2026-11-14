#pragma once
#include "inventory.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
namespace ReliefOps {
namespace Logistics {
class LogisticsException : public std::exception {
    std::string message_;

  public:
    explicit LogisticsException(std::string message) : message_(std::move(message)) {}
    const char *what() const noexcept override { return message_.c_str(); }
};
class CargoOverweightException : public LogisticsException {
  public:
    using LogisticsException::LogisticsException;
};
class FuelDepletionException : public LogisticsException {
  public:
    using LogisticsException::LogisticsException;
};
class DamagedCorridorException : public LogisticsException {
  public:
    using LogisticsException::LogisticsException;
};
struct GPSBeacon {
    double x = 0, y = 0;
}; // Local Cartesian coordinates in km.
struct EmergencyLifeSupportUnit {
    bool enabled = true;
};
inline double distanceKm(const GPSBeacon &a, const GPSBeacon &b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}
inline double tonnes(double kg) {
    return kg / 1000.0;
}
struct Route {
    GPSBeacon destination;
    bool roadOpen = true, waterOpen = true, weatherClear = true;
};
struct Mission {
    int id = 1, priority = 1;
    std::string vehicleId;
    Route route;
    Inventory::Manifest cargo;
};
Mission &updateMissionPriority(Mission &, int);
struct UrgentMedicalFirst {
    bool operator()(const Mission &a, const Mission &b) const {
        return a.priority != b.priority ? a.priority > b.priority : a.id < b.id;
    }
};
template <class T, class Compare = std::less<T>> class PriorityDispatchQueue {
    std::vector<T> entries_;
    Compare compare_;

  public:
    void push(const T &value) {
        entries_.push_back(value);
        std::stable_sort(entries_.begin(), entries_.end(), compare_);
    }
    const T &top() const {
        if (entries_.empty())
            throw std::out_of_range("Dispatch queue empty");
        return entries_.front();
    }
    void pop() {
        if (entries_.empty())
            throw std::out_of_range("Dispatch queue empty");
        entries_.erase(entries_.begin());
    }
    bool empty() const noexcept { return entries_.empty(); }
    std::size_t size() const noexcept { return entries_.size(); }
};
class IncidentCommander;
class TransportVehicle {
    std::string id_;
    double maxKg_, maxVolume_, fuel_;
    static int operational_;

  protected:
    TransportVehicle(std::string, double, double, double);
    void consumeFuel(double amount) noexcept { fuel_ -= amount; }

  public:
    TransportVehicle(const TransportVehicle &) = delete;
    TransportVehicle &operator=(const TransportVehicle &) = delete;
    virtual ~TransportVehicle();
    const std::string &id() const noexcept { return id_; }
    double fuel() const noexcept { return fuel_; }
    double maxKg() const noexcept { return maxKg_; }
    double maxVolume() const noexcept { return maxVolume_; }
    static int operationalVehicles() noexcept { return operational_; }
    void refuel(double);
};
class ReliefVehicle : public TransportVehicle {
    int emergencyPriority_ = 1;
    GPSBeacon gps_;
    EmergencyLifeSupportUnit lifeSupport_;
    friend class IncidentCommander;

  public:
    ReliefVehicle(std::string, double, double, double);
    virtual ~ReliefVehicle() = default;
    virtual std::string type() const = 0;
    virtual double evaluateRoute(const Route &) const = 0;
    virtual bool medicalCapable() const { return lifeSupport_.enabled; }
    const GPSBeacon &position() const noexcept { return gps_; }
    int emergencyPriority() const noexcept { return emergencyPriority_; }
    double validateDispatch(const Mission &) const;
    void completeDispatch(const Route &, double fuelCost) noexcept;
};
class GroundConvoy : virtual public ReliefVehicle {
  public:
    GroundConvoy(std::string id, double kg = 5000, double volume = 30, double fuel = 300);
    std::string type() const override { return "Ground convoy"; }
    double evaluateRoute(const Route &) const override;
};
class AerialTransport : public ReliefVehicle {
    bool drone_;

  public:
    AerialTransport(std::string id, bool drone = false);
    std::string type() const override {
        return drone_ ? "Rescue quadcopter" : "Medical helicopter";
    }
    double evaluateRoute(const Route &) const override;
    bool medicalCapable() const override { return !drone_; }
};
class AmphibiousCraft : virtual public ReliefVehicle {
  public:
    AmphibiousCraft(std::string id, double kg = 2000, double volume = 15, double fuel = 200);
    std::string type() const override { return "Amphibious boat"; }
    double evaluateRoute(const Route &) const override;
};
class AmphibiousMedicalUnit : public GroundConvoy, public AmphibiousCraft {
  public:
    explicit AmphibiousMedicalUnit(std::string id);
    std::string type() const override { return "Amphibious medical unit"; }
    double evaluateRoute(const Route &) const override;
};
class IncidentCommander {
  public:
    // Direct private-field access; does not bypass payload, fuel or corridor safety.
    void triageOverride(ReliefVehicle &, Mission &, int priority) const;
};
std::vector<std::unique_ptr<ReliefVehicle>> makeFleet();
} // namespace Logistics
} // namespace ReliefOps
