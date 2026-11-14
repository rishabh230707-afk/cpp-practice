#include "erl/logistics.hpp"
#include <stdexcept>
namespace ReliefOps {
namespace Logistics {
int TransportVehicle::operational_ = 0;
TransportVehicle::TransportVehicle(std::string id, double kg, double vol, double fuel)
    : id_(std::move(id)), maxKg_(kg), maxVolume_(vol), fuel_(fuel) {
    if (id_.empty() || !std::isfinite(kg) || kg <= 0 || !std::isfinite(vol) || vol <= 0 ||
        !std::isfinite(fuel) || fuel < 0)
        throw std::invalid_argument("Invalid vehicle");
    ++operational_;
}
TransportVehicle::~TransportVehicle() {
    --operational_;
}
void TransportVehicle::refuel(double n) {
    if (!std::isfinite(n) || n <= 0 || !std::isfinite(fuel_ + n))
        throw std::invalid_argument("Invalid fuel");
    fuel_ += n;
}
ReliefVehicle::ReliefVehicle(std::string id, double kg, double vol, double fuel)
    : TransportVehicle(std::move(id), kg, vol, fuel) {}
Mission &updateMissionPriority(Mission &m, int p) {
    if (p < 1 || p > 5)
        throw std::invalid_argument("Priority must be 1..5");
    m.priority = p;
    return m;
}
static double distance(const ReliefVehicle &v, const Route &r) {
    double d = distanceKm(v.position(), r.destination);
    if (!std::isfinite(d) || d > 10000)
        throw std::invalid_argument("Invalid destination");
    return d;
}
GroundConvoy::GroundConvoy(std::string id, double kg, double vol, double fuel)
    : ReliefVehicle(std::move(id), kg, vol, fuel) {}
double GroundConvoy::evaluateRoute(const Route &r) const {
    if (!r.roadOpen)
        throw DamagedCorridorException("Ground route blocked by damaged road");
    return distance(*this, r) * 0.35;
}
AerialTransport::AerialTransport(std::string id, bool drone)
    : ReliefVehicle(std::move(id), drone ? 10 : 1500, drone ? 0.2 : 12, drone ? 20 : 500),
      drone_(drone) {}
double AerialTransport::evaluateRoute(const Route &r) const {
    if (!r.weatherClear)
        throw DamagedCorridorException("Unsafe flight weather");
    return distance(*this, r) * (drone_ ? 0.2 : 2.0);
}
AmphibiousCraft::AmphibiousCraft(std::string id, double kg, double vol, double fuel)
    : ReliefVehicle(std::move(id), kg, vol, fuel) {}
double AmphibiousCraft::evaluateRoute(const Route &r) const {
    if (!r.waterOpen)
        throw DamagedCorridorException("Water corridor obstructed");
    return distance(*this, r) * 0.6;
}
AmphibiousMedicalUnit::AmphibiousMedicalUnit(std::string id)
    : ReliefVehicle(id, 2500, 18, 250), GroundConvoy(id), AmphibiousCraft(id) {}
double AmphibiousMedicalUnit::evaluateRoute(const Route &r) const {
    if (r.roadOpen)
        return GroundConvoy::evaluateRoute(r);
    return AmphibiousCraft::evaluateRoute(r);
}
double ReliefVehicle::validateDispatch(const Mission &m) const {
    if (m.id <= 0 || m.priority < 1 || m.priority > 5 || m.vehicleId != id() || m.cargo.size() == 0)
        throw std::invalid_argument("Invalid mission");
    if (m.cargo.weight() > maxKg() || m.cargo.volume() > maxVolume())
        throw CargoOverweightException("Payload exceeds vehicle mass or volume capacity");
    double cost = evaluateRoute(m.route);
    if (cost > fuel())
        throw FuelDepletionException("Insufficient fuel for route");
    return cost;
}
void ReliefVehicle::completeDispatch(const Route &r, double cost) noexcept {
    consumeFuel(cost);
    gps_ = r.destination;
}
void IncidentCommander::triageOverride(ReliefVehicle &v, Mission &m, int p) const {
    if (m.vehicleId != v.id() || !v.medicalCapable())
        throw std::invalid_argument("Medical override requires assigned medical-capable vehicle");
    updateMissionPriority(m, p);
    v.emergencyPriority_ = p;
}
std::vector<std::unique_ptr<ReliefVehicle>> makeFleet() {
    std::vector<std::unique_ptr<ReliefVehicle>> f;
    f.emplace_back(new GroundConvoy("TRUCK-1"));
    f.emplace_back(new AerialTransport("HELI-1"));
    f.emplace_back(new AerialTransport("DRONE-1", true));
    f.emplace_back(new AmphibiousCraft("BOAT-1"));
    f.emplace_back(new AmphibiousMedicalUnit("HYBRID-1"));
    return f;
}
} // namespace Logistics
} // namespace ReliefOps
