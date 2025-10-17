#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4LogicalVolume.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "Randomize.hh"

namespace B1 {

SteppingAction::SteppingAction(EventAction* eventAction)
 : G4UserSteppingAction(),
   fEventAction(eventAction),
   fScoringVolume(nullptr)
{
    fNumQEPoints = 23;
    G4double photonEnergy_eV[23] = {2.175, 2.214, 2.254, 2.296,
                                    2.339, 2.384, 2.431, 2.480,
                                    2.530, 2.583, 2.638, 2.695,
                                    2.755, 2.818, 2.883, 2.952,
                                    3.024, 3.100, 3.179, 3.263,
                                    3.351, 3.444, 3.542};
    G4double QE[23];
    for (int i=0; i<23; i++) QE[i] = 0.28;

    fPhotonEnergies.assign(photonEnergy_eV, photonEnergy_eV + 23);
    fQE.assign(QE, QE + 23);
}

G4double SteppingAction::GetQE(G4double energy)
{
    G4double minDiff = std::abs(fPhotonEnergies[0] - energy);
    int idx = 0;
    for (int i=1; i<fNumQEPoints; ++i) {
        G4double diff = std::abs(fPhotonEnergies[i] - energy);
        if (diff < minDiff) {
            minDiff = diff;
            idx = i;
        }
    }
    return fQE[idx];
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    if (!fScoringVolume) {
        const auto detConstruction = static_cast<const DetectorConstruction*>(
            G4RunManager::GetRunManager()->GetUserDetectorConstruction());
        fScoringVolume = detConstruction->GetScoringVolume();
    }

    G4LogicalVolume* volume = step->GetPreStepPoint()->GetTouchableHandle()
                              ->GetVolume()->GetLogicalVolume();

    G4Track* track = step->GetTrack();

    if (track->GetDefinition()->GetParticleName() != "opticalphoton") return;
    if (!(volume == fScoringVolume)) return;

    G4double Ephoton = track->GetKineticEnergy();
    G4double prob = GetQE(Ephoton/eV);
    if (G4UniformRand() > prob) return;

    G4double time = track->GetGlobalTime();
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillNtupleDColumn(0, Ephoton/eV); // энергия eV
    analysisManager->FillNtupleDColumn(1, time/ns);    // время ns
    analysisManager->AddNtupleRow();

    fEventAction->AddEdep(Ephoton);
}

} // namespace B1
