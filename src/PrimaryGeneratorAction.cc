#include "PrimaryGeneratorAction.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Tubs.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "Randomize.hh"

namespace B1
{

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
  G4int n_particle = 1;
  fParticleGun = new G4ParticleGun(n_particle);

  G4ParticleDefinition* particle =
      G4ParticleTable::GetParticleTable()->FindParticle("e+");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleEnergy(3.0 * MeV);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  G4double vessel_radius = 600 * mm;
  G4double pmma_thickness = 10 * mm;
  G4double vessel_height = 700 * mm;

  G4double inner_radius = 0.0;
  G4double outer_radius = vessel_radius - pmma_thickness;
  G4double half_height = vessel_height / 2.0;

  G4double r = outer_radius * std::sqrt(G4UniformRand());
  G4double phi = 2. * CLHEP::pi * G4UniformRand();
  G4double z = (2. * G4UniformRand() - 1.) * half_height;

  G4double x = r * std::cos(phi);
  G4double y = r * std::sin(phi);

  fParticleGun->SetParticlePosition(G4ThreeVector(x, y, z));

  G4double costheta = 2. * G4UniformRand() - 1.;
  G4double sintheta = std::sqrt(1. - costheta * costheta);
  G4double phi_dir = 2. * CLHEP::pi * G4UniformRand();

  G4double dx = sintheta * std::cos(phi_dir);
  G4double dy = sintheta * std::sin(phi_dir);
  G4double dz = costheta;

  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(dx, dy, dz));

  fParticleGun->GeneratePrimaryVertex(event);
}

}  // namespace B1
