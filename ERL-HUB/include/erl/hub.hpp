#pragma once
#include "logistics.hpp"
namespace ReliefOps {
namespace Logistics {
struct AuditRecord {
    Mission mission;
    double fuelCost = 0;
    std::string agency;
};
struct HubState {
    Inventory::DistributionDepot depot;
    std::vector<AuditRecord> records;
};
class BinaryStore {
    std::string path_;

  public:
    explicit BinaryStore(std::string path) : path_(std::move(path)) {}
    HubState load() const;
    void save(const HubState &, bool simulateFailure = false) const;
};
class ReliefHub {
    HubState state_;
    BinaryStore store_;
    std::vector<std::unique_ptr<ReliefVehicle>> fleet_;
    static double dispatchedTonnes_;

  public:
    explicit ReliefHub(std::string path);
    const Inventory::DistributionDepot &depot() const { return state_.depot; }
    const std::vector<AuditRecord> &records() const { return state_.records; }
    std::vector<ReliefVehicle *> fleet() const;
    ReliefVehicle &vehicle(const std::string &) const;
    void restock(const Inventory::ReliefCargo &, bool simulateFailure = false);
    void dispatch(const Mission &, const std::string &agency, bool simulateFailure = false);
    static double dispatchedTonnes() noexcept { return dispatchedTonnes_; }
};
} // namespace Logistics
} // namespace ReliefOps
