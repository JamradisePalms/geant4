#pragma once

#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"
#include "G4SystemOfUnits.hh"

namespace B1 {

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;

    G4LogicalVolume* GetScoringVolume() const { return fScoringVolume; }

private:
    G4LogicalVolume* fScoringVolume = nullptr;

    // PMT параметры
    G4double fPMTRadius = 1.*cm;
    G4double fPMTzOffset = 5.*cm;
    G4int    fNumPMTperPlane = 4;
};

} // namespace B1
