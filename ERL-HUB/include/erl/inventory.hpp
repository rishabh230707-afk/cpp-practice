#pragma once
#include <cstddef>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>
namespace ReliefOps {
namespace Inventory {
class ReliefCargo {
    std::string sku_;
    int units_;
    double unitKg_, unitVolume_;

  public:
    ReliefCargo(std::string sku = "ration", int units = 0, double kg = 1, double volume = 0.01);
    const std::string &sku() const noexcept { return sku_; }
    int units() const noexcept { return units_; }
    double unitKg() const noexcept { return unitKg_; }
    double unitVolume() const noexcept { return unitVolume_; }
    double weight() const noexcept { return units_ * unitKg_; }
    double volume() const noexcept { return units_ * unitVolume_; }
    bool operator==(const ReliefCargo &) const noexcept;
    friend std::ostream &operator<<(std::ostream &, const ReliefCargo &);
    friend std::istream &operator>>(std::istream &, ReliefCargo &);
};
// Explicit heap ownership is intentional: demonstrates the Rule of Five.
class Manifest {
    ReliefCargo *items_;
    std::size_t size_, capacity_;

  public:
    explicit Manifest(std::size_t capacity = 4);
    Manifest(const Manifest &);
    Manifest(Manifest &&) noexcept;
    Manifest &operator=(Manifest other);
    ~Manifest();
    void swap(Manifest &) noexcept;
    void add(const ReliefCargo &);
    std::size_t size() const noexcept { return size_; }
    const ReliefCargo &at(std::size_t) const;
    double weight() const noexcept;
    double volume() const noexcept;
    bool operator==(const Manifest &) const noexcept;
    friend std::ostream &operator<<(std::ostream &, const Manifest &);
    friend std::istream &operator>>(std::istream &, Manifest &);
};
class DistributionDepot {
    std::unordered_map<std::string, ReliefCargo> stock_;

  public:
    DistributionDepot &operator+=(const ReliefCargo &);
    DistributionDepot &operator-=(const ReliefCargo &);
    int available(const std::string &) const;
    const ReliefCargo &item(const std::string &) const;
    std::vector<ReliefCargo> items() const;
    void swap(DistributionDepot &) noexcept;
};
// Both functions mutate caller-owned objects and return those same objects by reference.
DistributionDepot &restock(DistributionDepot &, const ReliefCargo &);
Manifest &allocateSupply(Manifest &, const DistributionDepot &, const std::string &,
                         double maxVolume);
Manifest &allocateSupply(Manifest &, const DistributionDepot &, const std::string &, int patients,
                         int triagePriority);
std::ostream &manifestHeader(std::ostream &);
void printManifest(std::ostream &, const Manifest &);
} // namespace Inventory
} // namespace ReliefOps
