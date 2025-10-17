// DetectorConstruction.cc  — реализация Construct() под ТЗ (исправленная версия)
#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpticalSurface.hh"
#include "G4MaterialPropertiesTable.hh"

#include <cmath>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>

namespace B1
{
DetectorConstruction::DetectorConstruction()
 : G4VUserDetectorConstruction(),
   fScoringVolume(nullptr),
   fNumPMTperPlane(12),
   fPMTRadius(480.*mm),      // радиус размещения ФЭУ (по умолчанию 480 mm)
   fPMTzOffset( (700.*mm)/2 - 60.*mm ) // расстояние от центра до плоскости верх/ниж ФЭУ (пример)
{
}

DetectorConstruction::~DetectorConstruction()
{
}

// ---- Construct() ----------------------------------------------------------------
G4VPhysicalVolume* DetectorConstruction::Construct()
{
  // -----------------------
  // 0) Загрузка спектра Bi-MSB из файла
  //    (будет применён только к LAB_Gd)
  // -----------------------
  const std::string bisFile = "~/bisMSB_EmissionSpectra.dat";
  std::vector<G4double> filePhotonEnergies; // энергии (eV) — from file
  std::vector<G4double> fileEmission;       // относительная интенсивность

  std::ifstream fin(bisFile.c_str());
  if (!fin.is_open()) {
      G4cerr << "WARNING: cannot open Bi-MSB file: " << bisFile << G4endl;
      G4cerr << "Using fallback simple two-point emission spectrum." << G4endl;
      filePhotonEnergies.push_back(2.0*eV);
      fileEmission.push_back(0.0);
      filePhotonEnergies.push_back(3.5*eV);
      fileEmission.push_back(1.0);
  } else {
      double lambda_nm=0.0, intensity=0.0;
      while (fin >> lambda_nm >> intensity) {
          if (lambda_nm <= 0.0) continue;
          G4double energy = (1240.0 / lambda_nm) * eV; // convert nm -> eV
          filePhotonEnergies.push_back(energy);
          fileEmission.push_back((G4double) intensity);
      }
      fin.close();
  }

  std::vector<size_t> idx(filePhotonEnergies.size());
  for (size_t i=0;i<idx.size();++i) idx[i]=i;
  std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b){ return filePhotonEnergies[a] < filePhotonEnergies[b]; });

  std::vector<G4double> sortedE, sortedI;
  sortedE.reserve(idx.size());
  sortedI.reserve(idx.size());
  for (size_t k=0;k<idx.size();++k){
      sortedE.push_back(filePhotonEnergies[idx[k]]);
      sortedI.push_back(fileEmission[idx[k]]);
  }

  // Нормировка интенсивности (максимум -> 1.0)
  G4double maxI = 0.0;
  for (auto v : sortedI) if (v > maxI) maxI = v;
  if (maxI <= 0.0) {
      // если всё нули — заменить на плоский спектр
      sortedI.assign(sortedI.size(), 1.0);
      maxI = 1.0;
  }
  for (size_t i=0;i<sortedI.size();++i) sortedI[i] = sortedI[i] / maxI;

  const G4int nSpecPoints = static_cast<G4int>(sortedE.size());
  std::vector<G4double> labGdEnergies = sortedE;
  std::vector<G4double> labGdEmis     = sortedI;

  // -----------------------
  // 1) Get nist manager and materials
  // -----------------------
  G4NistManager* nist = G4NistManager::Instance();
  G4bool checkOverlaps = true;

  // Air (world)
  G4Material* air = nist->FindOrBuildMaterial("G4_AIR");

  // Steel
  G4Material* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");

  // PMMA
  G4Material* PMMA = nist->FindOrBuildMaterial("G4_PLEXIGLASS");

  // Si for PMT simplification
  G4Material* silicon = nist->FindOrBuildMaterial("G4_Si");

  // Create LAB_pure
  G4Element* elH = nist->FindOrBuildElement("H");
  G4Element* elC = nist->FindOrBuildElement("C");
  G4double densityLAB = 0.853*g/cm3;
  G4Material* LAB_pure = new G4Material("LAB_pure", densityLAB, 2);
  LAB_pure->AddElement(elC, 17);
  LAB_pure->AddElement(elH, 27);

  // Gd-loaded LAB (1 g/L)
  G4Element* elGd = nist->FindOrBuildElement("Gd");
  G4double densityLAB_Gd = 0.86086*g/cm3;
  G4Material* LAB_Gd = new G4Material("LAB_Gd", densityLAB_Gd, 2);
  LAB_Gd->AddMaterial(LAB_pure, 99.884*perCent);
  LAB_Gd->AddElement(elGd, 0.116*perCent);

  // -----------------------
  // 2) Оптичесные свойства для "базовых" материалов (fixed spectrum)
  // -----------------------
  const G4int nSpec = 8;
  G4double specEnergies[nSpec] = {
    2.00*eV, 2.25*eV, 2.50*eV, 2.75*eV, 3.00*eV, 3.25*eV, 3.40*eV, 3.50*eV
  };

  // rindex arrays
  G4double rindex_LAB[nSpec];
  G4double rindex_PMMA[nSpec];
  for (G4int i=0;i<nSpec;i++){
    rindex_LAB[i]  = 1.48;
    rindex_PMMA[i] = 1.49;
  }

  // ABS lengths according to TЗ
  G4double abs_LAB_Gd[nSpec];
  G4double abs_LAB_pure[nSpec];
  for (G4int i=0;i<nSpec;i++){
    abs_LAB_Gd[i]   = 6.*m;   // Gd-LAB = 6 m
    abs_LAB_pure[i] = 12.*m;  // pure LAB = 12 m
  }

  G4double abs_PMMA[nSpec];
  for (G4int i=0;i<nSpec;i++) abs_PMMA[i] = 5.*m;

  // PMMA MPT
  G4MaterialPropertiesTable* mptPMMA = new G4MaterialPropertiesTable();
  mptPMMA->AddProperty("RINDEX", specEnergies, rindex_PMMA, nSpec);
  mptPMMA->AddProperty("ABSLENGTH", specEnergies, abs_PMMA, nSpec);
  PMMA->SetMaterialPropertiesTable(mptPMMA);

  // LAB_pure MPT (no scintillation)
  G4MaterialPropertiesTable* mptLABpure = new G4MaterialPropertiesTable();
  mptLABpure->AddProperty("RINDEX", specEnergies, rindex_LAB, nSpec);
  mptLABpure->AddProperty("ABSLENGTH", specEnergies, abs_LAB_pure, nSpec);
  LAB_pure->SetMaterialPropertiesTable(mptLABpure);

  // -----------------------
  // 3) LAB_Gd MPT
  // -----------------------
  G4MaterialPropertiesTable* mptLABGd = new G4MaterialPropertiesTable();

  // RINDEX и ABSLENGTH для LAB_Gd задаём на той же сетке, что и спектр файла:
  std::vector<G4double> rindex_labgd_vec(nSpecPoints, 1.48);
  std::vector<G4double> abs_labgd_vec(nSpecPoints, 6.*m);

  mptLABGd->AddProperty("RINDEX", &labGdEnergies[0], &rindex_labgd_vec[0], nSpecPoints);
  mptLABGd->AddProperty("ABSLENGTH", &labGdEnergies[0], &abs_labgd_vec[0], nSpecPoints);

  // FASTCOMPONENT = спектр Bi-MSB (normalized)
  mptLABGd->AddProperty("FASTCOMPONENT", &labGdEnergies[0], &labGdEmis[0], nSpecPoints, true);
  mptLABGd->AddProperty("SLOWCOMPONENT", &labGdEnergies[0], &labGdEmis[0], nSpecPoints, true);

  // Сцинтилляционные параметры (примерные — можно менять)
  mptLABGd->AddConstProperty("SCINTILLATIONYIELD", 4300.0/MeV, true);
  mptLABGd->AddConstProperty("RESOLUTIONSCALE", 1.0, true);
  mptLABGd->AddConstProperty("FASTTIMECONSTANT", 5.0*ns, true);
  mptLABGd->AddConstProperty("YIELDRATIO", 1.0, true);

  LAB_Gd->SetMaterialPropertiesTable(mptLABGd);

  // -----------------------
  // 4) Геометрия: World
  // -----------------------
  G4double world_size = 2.0*m;
  G4Box* solidWorld = new G4Box("World", 0.5*world_size, 0.5*world_size, 0.5*world_size);
  G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
  G4VPhysicalVolume* physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0, checkOverlaps);

  G4VisAttributes* visWorld = new G4VisAttributes(G4Colour(0.8,0.8,0.8,0.05));
  visWorld->SetVisibility(true);
  logicWorld->SetVisAttributes(visWorld);

  // -----------------------
  // 5) Стальной бак (shell) и внутренний volume LAB (lab buffer)
  // -----------------------
  G4double steelInnerRadius = 1260.*mm/2.0;    // 630 mm
  G4double steelThickness   = 2.*mm;
  G4double steelOuterRadius = steelInnerRadius + steelThickness;
  G4double steelHalfHeight  = 1300.*mm/2.0;    // half-length

  // Steel shell (only the steel)
  G4Tubs* solidSteelTank = new G4Tubs("SteelTank", steelInnerRadius, steelOuterRadius, steelHalfHeight, 0.*deg, 360.*deg);
  G4LogicalVolume* logicSteelTank = new G4LogicalVolume(solidSteelTank, steel, "SteelTank");
  new G4PVPlacement(nullptr, G4ThreeVector(), logicSteelTank, "SteelTank", logicWorld, false, 0, checkOverlaps);

  G4Tubs* solidLabBuffer = new G4Tubs("LabBuffer", 0.*mm, steelInnerRadius, steelHalfHeight, 0.*deg, 360.*deg);
  G4LogicalVolume* logicLabBuffer = new G4LogicalVolume(solidLabBuffer, LAB_pure, "LabBuffer");
  new G4PVPlacement(nullptr, G4ThreeVector(), logicLabBuffer, "LabBuffer", logicWorld, false, 0, checkOverlaps);

  // визуализация
  G4VisAttributes* visSteel = new G4VisAttributes(G4Colour(0.5,0.5,0.5));
  visSteel->SetForceWireframe(true);
  logicSteelTank->SetVisAttributes(visSteel);

  G4VisAttributes* visLabBuffer = new G4VisAttributes(G4Colour(0.0,0.0,1.0,0.12));
  visLabBuffer->SetForceSolid(true);
  logicLabBuffer->SetVisAttributes(visLabBuffer);

  // -----------------------
  // 6) PMMA vessel and Gd-LAB inside it
  // -----------------------
  G4double vesselOuterRadius = 1200.*mm / 2.0; // 600 mm
  G4double pmmaThickness = 10.*mm;
  G4double vesselInnerRadius = vesselOuterRadius - pmmaThickness; // 590 mm
  G4double vesselHalfHeight = 700.*mm / 2.0; // half-length

  // PMMA shell
  G4Tubs* solidPMMAvessel = new G4Tubs("PMMAVessel", vesselInnerRadius, vesselOuterRadius, vesselHalfHeight, 0.*deg, 360.*deg);
  G4LogicalVolume* logicPMMAvessel = new G4LogicalVolume(solidPMMAvessel, PMMA, "PMMAVessel");
  // PMMA vessel размещаем внутри LabBuffer
  new G4PVPlacement(nullptr, G4ThreeVector(), logicPMMAvessel, "PMMAVessel", logicLabBuffer, false, 0, checkOverlaps);

  // Gd-LAB inside PMMA
  G4Tubs* solidGdLAB = new G4Tubs("GdLAB", 0.*mm, vesselInnerRadius, vesselHalfHeight, 0.*deg, 360.*deg);
  G4LogicalVolume* logicGdLAB = new G4LogicalVolume(solidGdLAB, LAB_Gd, "GdLAB");
  new G4PVPlacement(nullptr, G4ThreeVector(), logicGdLAB, "GdLAB", logicPMMAvessel, false, 0, checkOverlaps);

  // Visualization
  G4VisAttributes* visPMMA = new G4VisAttributes(G4Colour(0.0,1.0,0.0,0.2));
  visPMMA->SetForceWireframe(true);
  logicPMMAvessel->SetVisAttributes(visPMMA);

  G4VisAttributes* visGdLab = new G4VisAttributes(G4Colour(1.0,0.0,0.0,0.25));
  visGdLab->SetForceSolid(true);
  logicGdLAB->SetVisAttributes(visGdLab);

  fScoringVolume = logicGdLAB;

  // -----------------------
  // 7) Optical surfaces
  //    7.2 Steel inner surface — Mylar (reflectivity 0.9)
  // -----------------------
  // 7.1 PMMA - Gd interface
  G4OpticalSurface* surfPMMA_Gd = new G4OpticalSurface("PMMA_Gd_Surface");
  surfPMMA_Gd->SetType(dielectric_dielectric);
  surfPMMA_Gd->SetFinish(polished);
  surfPMMA_Gd->SetModel(unified);

  G4MaterialPropertiesTable* mptSurfPMMA = new G4MaterialPropertiesTable();
  std::vector<G4double> zeroRef(nSpec, 0.0);
  surfPMMA_Gd->SetMaterialPropertiesTable(mptSurfPMMA);
  new G4LogicalSkinSurface("PMMA_Skin", logicPMMAvessel, surfPMMA_Gd);

  // 7.2 Steel inner surface covered with Mylar (dielectric_metal)
  G4OpticalSurface* surfSteelMylar = new G4OpticalSurface("Steel_Mylar_Surface");
  surfSteelMylar->SetType(dielectric_metal);
  surfSteelMylar->SetFinish(polished);
  surfSteelMylar->SetModel(unified);

  std::vector<G4double> reflMylar(nSpec, 0.90);
  std::vector<G4double> zeroEff(nSpec, 0.0);
  G4MaterialPropertiesTable* mptSurfSteel = new G4MaterialPropertiesTable();
  mptSurfSteel->AddProperty("REFLECTIVITY", specEnergies, &reflMylar[0], nSpec);
  mptSurfSteel->AddProperty("EFFICIENCY", specEnergies, &zeroEff[0], nSpec);
  surfSteelMylar->SetMaterialPropertiesTable(mptSurfSteel);

  // Apply skin surface to the steel logical volume (inner surface reflection)
  new G4LogicalSkinSurface("SteelInnerMylar", logicSteelTank, surfSteelMylar);

  // -----------------------
  // 8) PMTs: material properties + placement
  // -----------------------
  // PMT window material MPT (use specEnergies grid)
  G4double rindex_pmt[nSpec];
  for (G4int i=0;i<nSpec;i++) rindex_pmt[i] = 1.52;
  G4double abs_pmt[nSpec];
  for (G4int i=0;i<nSpec;i++) abs_pmt[i] = 1.*nm;

  G4MaterialPropertiesTable* mptPMT = new G4MaterialPropertiesTable();
  mptPMT->AddProperty("RINDEX", specEnergies, rindex_pmt, nSpec);
  mptPMT->AddProperty("ABSLENGTH", specEnergies, abs_pmt, nSpec);
  silicon->SetMaterialPropertiesTable(mptPMT);

  // Logical PMT (simplified short disk)
  G4double pmtRadius = 75.*mm;
  G4Tubs* solidPMT = new G4Tubs("PMT", 0.*mm, pmtRadius, 2.*mm, 0.*deg, 360.*deg);
  G4LogicalVolume* logicPMT = new G4LogicalVolume(solidPMT, silicon, "PMT");

  // Photocathode optical surface: set EFFICIENCY = 0.28 (const) on specEnergies
  std::vector<G4double> QE(nSpec, 0.28); // 28%
  std::vector<G4double> zeroR(nSpec, 0.0);

  G4OpticalSurface* surfPMT_cath = new G4OpticalSurface("PMT_Cath_Surface");
  surfPMT_cath->SetType(dielectric_metal); // commonly used for photocathode modelling
  surfPMT_cath->SetModel(unified);
  surfPMT_cath->SetFinish(polished);

  G4MaterialPropertiesTable* mptSurfPMT = new G4MaterialPropertiesTable();
  mptSurfPMT->AddProperty("EFFICIENCY", specEnergies, &QE[0], nSpec);
  mptSurfPMT->AddProperty("REFLECTIVITY", specEnergies, &zeroR[0], nSpec);
  surfPMT_cath->SetMaterialPropertiesTable(mptSurfPMT);

  // Apply skin surface to logicPMT once (not in loop)
  new G4LogicalSkinSurface("PMT_Skin", logicPMT, surfPMT_cath);

  // Place PMTs: top and bottom planes (children of LabBuffer so they are in LAB)
  G4double zTop  = +fPMTzOffset;
  G4double zBottom = -fPMTzOffset;
  G4double twoPi = 2.0*M_PI;

  for (G4int i=0; i < fNumPMTperPlane; ++i) {
    G4double phi = (twoPi / fNumPMTperPlane) * i;
    G4double x = fPMTRadius * std::cos(phi);
    G4double y = fPMTRadius * std::sin(phi);

    // Top PMT
    G4ThreeVector posTop(x, y, zTop);
    new G4PVPlacement(nullptr, posTop, logicPMT, "PMT_top", logicLabBuffer, false, i, checkOverlaps);

    // Bottom PMT
    G4ThreeVector posBot(x, y, zBottom);
    new G4PVPlacement(nullptr, posBot, logicPMT, "PMT_bot", logicLabBuffer, false, i + fNumPMTperPlane, checkOverlaps);
  }

  // -----------------------
  // 9) Finish
  // -----------------------
  return physWorld;
}
}