#pragma once
#include "G4UserSteppingAction.hh"
#include "G4LogicalVolume.hh"
#include <vector>

namespace B1 {
class EventAction;
class DetectorConstruction;

class SteppingAction : public G4UserSteppingAction
{
public:
    SteppingAction(EventAction* eventAction);
    virtual ~SteppingAction() = default;

    virtual void UserSteppingAction(const G4Step* step) override;

private:
    EventAction* fEventAction;
    G4LogicalVolume* fScoringVolume = nullptr;

    // PMT квантовая эффективность
    std::vector<G4double> fPhotonEnergies; // энергии фотонов (eV)
    std::vector<G4double> fQE;             // соответствующая QE (0..1)

    int fNumQEPoints;
    G4double GetQE(G4double energy);
};
} // namespace B1
