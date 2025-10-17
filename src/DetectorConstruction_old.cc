// #include "DetectorConstruction.hh"

// #include "G4Box.hh"
// #include "G4Tubs.hh"
// #include "G4LogicalVolume.hh"
// #include "G4NistManager.hh"
// #include "G4PVPlacement.hh"
// #include "G4SystemOfUnits.hh"
// #include "G4SubtractionSolid.hh"
// #include "G4OpticalSurface.hh"
// #include "G4LogicalSkinSurface.hh"
// #include "G4VisAttributes.hh"
// #include "G4Colour.hh"         

// namespace B1
// {

// G4VPhysicalVolume* DetectorConstruction::Construct()
// {
//   // Get nist material manager
//   G4NistManager* nist = G4NistManager::Instance();

//   // Create materials
//   G4Element* elC = nist->FindOrBuildElement("C");
//   G4Element* elH = nist->FindOrBuildElement("H");
//   G4Element* elGd = nist->FindOrBuildElement("Gd");
//   G4Element* elO = nist->FindOrBuildElement("O");
  
//   // LAB material
//   G4Material* LAB = new G4Material("LinearAlkylbenzene", 0.86*g/cm3, 2);
//   LAB->AddElement(elC, 18);
//   LAB->AddElement(elH, 30);

//   // Gd-loaded LAB (1g/liter)
//   G4Material* LAB_Gd = new G4Material("LAB_Gd", 0.86086*g/cm3, 2);
//   LAB_Gd->AddMaterial(LAB, 99.884*perCent);
//   LAB_Gd->AddElement(elGd, 0.116*perCent);

//   // PMMA material
//   G4Material* PMMA = new G4Material("PMMA", 1.19*g/cm3, 3);
//   PMMA->AddElement(elC, 5);
//   PMMA->AddElement(elH, 8);
//   PMMA->AddElement(elO, 2);

//   // Stainless steel
//   G4Material* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");

//   // Air
//   G4Material* air = nist->FindOrBuildMaterial("G4_AIR");

//   // Option to switch on/off checking of volumes overlaps
//   G4bool checkOverlaps = true;

//   // World
//   G4double world_size = 2*m;
//   auto solidWorld = new G4Box("World", 0.5*world_size, 0.5*world_size, 0.5*world_size);
//   auto logicWorld = new G4LogicalVolume(solidWorld, air, "World");
  
//   // Прозрачный мир
//   G4VisAttributes* worldVisAttr = new G4VisAttributes(G4Colour(0.8, 0.8, 0.8, 0.1)); // серый, прозрачный
//   worldVisAttr->SetVisibility(true);
//   logicWorld->SetVisAttributes(worldVisAttr);
  
//   auto physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld,
//                                     "World", nullptr, false, 0, checkOverlaps);

//   // Stainless steel tank
//   G4double tank_height = 1300*mm;
//   G4double tank_radius = 630*mm;
//   G4double steel_thickness = 2*mm;
  
//   auto solidTank = new G4Tubs("SteelTank", tank_radius, tank_radius + steel_thickness,
//                              tank_height/2, 0, 360*deg);
//   auto logicTank = new G4LogicalVolume(solidTank, steel, "SteelTank");
  
//   // Серый цвет для стали
//   G4VisAttributes* steelVisAttr = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5)); // серый
//   steelVisAttr->SetForceWireframe(true); // каркасный режим для лучшего обзора
//   logicTank->SetVisAttributes(steelVisAttr);
  
//   new G4PVPlacement(nullptr, G4ThreeVector(), logicTank, "SteelTank",
//                     logicWorld, false, 0, checkOverlaps);

//   // Pure LAB inside tank
//   auto solidLAB = new G4Tubs("LAB", 0, tank_radius, tank_height/2, 0, 360*deg);
//   auto logicLAB = new G4LogicalVolume(solidLAB, LAB, "LAB");
  
//   G4VisAttributes* labVisAttr = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0, 0.3)); // синий, полупрозрачный
//   labVisAttr->SetForceSolid(true);
//   logicLAB->SetVisAttributes(labVisAttr);
  
//   new G4PVPlacement(nullptr, G4ThreeVector(), logicLAB, "LAB",
//                     logicWorld, false, 0, checkOverlaps);

//   // PMMA vessel
//   G4double vessel_height = 700*mm;
//   G4double vessel_radius = 600*mm;
//   G4double pmma_thickness = 10*mm;
  
//   auto solidVessel = new G4Tubs("PMMAVessel", vessel_radius - pmma_thickness,
//                                vessel_radius, vessel_height/2, 0, 360*deg);
//   auto logicVessel = new G4LogicalVolume(solidVessel, PMMA, "PMMAVessel");
  
//   G4VisAttributes* pmmaVisAttr = new G4VisAttributes(G4Colour(0.0, 1.0, 0.0)); // зеленый
//   pmmaVisAttr->SetForceWireframe(true);
//   logicVessel->SetVisAttributes(pmmaVisAttr);
  
//   new G4PVPlacement(nullptr, G4ThreeVector(), logicVessel, "PMMAVessel",
//                     logicLAB, false, 0, checkOverlaps);

//   // Gd-loaded LAB inside PMMA vessel
//   auto solidGdLAB = new G4Tubs("GdLAB", 0, vessel_radius - pmma_thickness,
//                               vessel_height/2, 0, 360*deg);
//   auto logicGdLAB = new G4LogicalVolume(solidGdLAB, LAB_Gd, "GdLAB");
  
//   G4VisAttributes* gdLabVisAttr = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0)); // красный
//   gdLabVisAttr->SetForceSolid(true);
//   logicGdLAB->SetVisAttributes(gdLabVisAttr);
  
//   new G4PVPlacement(nullptr, G4ThreeVector(), logicGdLAB, "GdLAB",
//                     logicLAB, false, 0, checkOverlaps);

//   fScoringVolume = logicGdLAB;

//   return physWorld;
// }

// }  // namespace B1