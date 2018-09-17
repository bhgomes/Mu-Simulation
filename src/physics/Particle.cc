/*
 * src/physics/Particle.cc
 *
 * Copyright 2018 Brandon Gomes
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "physics/Particle.hh"

#include <Geant4/G4ParticleDefinition.hh>
#include <Geant4/G4ParticleTable.hh>

namespace MATHUSLA { namespace MU {

namespace Physics { ////////////////////////////////////////////////////////////////////////////

//__Convert Momentum to Pseudo-Lorentz Triplet__________________________________________________
const PseudoLorentzTriplet Convert(const G4ThreeVector& momentum) {
  const auto magnitude = momentum.mag();
  if (magnitude == 0)
    return {};
  const auto eta = std::atanh(momentum.x() / magnitude);
  return {magnitude / std::cosh(eta), eta, std::atan2(momentum.y(), -momentum.z())};
}
//----------------------------------------------------------------------------------------------

//__Convert Pseudo-Lorentz Triplet to Momentum__________________________________________________
const G4ThreeVector Convert(const PseudoLorentzTriplet& triplet) {
  return G4ThreeVector(
     triplet.pT * std::sinh(triplet.eta),
     triplet.pT * std::sin(triplet.phi),
    -triplet.pT * std::cos(triplet.phi));
}
//----------------------------------------------------------------------------------------------

namespace { ////////////////////////////////////////////////////////////////////////////////////

//__Get Particle Definition from Geant4_________________________________________________________
G4ParticleDefinition* _get_particle_def(int id) {
  return G4ParticleTable::GetParticleTable()->FindParticle(id);
}
//----------------------------------------------------------------------------------------------

//__Get Particle Property from Definition_______________________________________________________
template<class Function, class Output>
Output _get_particle_property(int id,
                              Function f,
                              Output default_value={}) {
  return id == 0 ? default_value : f(_get_particle_def(id));
}
//----------------------------------------------------------------------------------------------

} /* anonymous namespace */ ////////////////////////////////////////////////////////////////////

//__Get Mass of Particle from ID________________________________________________________________
double GetParticleMass(int id) {
  return _get_particle_property(id, [](const auto& def) { return def->GetPDGMass(); }, 0);
}
//----------------------------------------------------------------------------------------------

//__Get Charge of Particle from ID______________________________________________________________
double GetParticleCharge(int id) {
  return _get_particle_property(id, [](const auto& def) { return def->GetPDGCharge(); }, 0);
}
//----------------------------------------------------------------------------------------------

//__Get Name of Particle from ID________________________________________________________________
const std::string GetParticleName(int id) {
  return _get_particle_property(id, [](const auto& def) { return def->GetParticleName(); }, G4String(""));
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle PT_______________________________________________________________________
double BasicParticle::pT() const {
  const auto magnitude = G4ThreeVector(px, py, pz).mag();
  if (magnitude == 0)
    return 0;
  return magnitude / std::cosh(std::atanh(px / magnitude));
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle ETA______________________________________________________________________
double BasicParticle::eta() const {
  const auto magnitude = G4ThreeVector(px, py, pz).mag();
  if (magnitude == 0)
    return 0;
  return std::atanh(px / magnitude);
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle PHI______________________________________________________________________
double BasicParticle::phi() const {
  return std::atan2(py, -pz);
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle PseudoLorentzTriplet_____________________________________________________
const PseudoLorentzTriplet BasicParticle::pseudo_lorentz_triplet() const {
  return Convert(G4ThreeVector(px, py, pz));
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle Name_____________________________________________________________________
const std::string BasicParticle::name() const {
  return GetParticleName(id);
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle Charge___________________________________________________________________
double BasicParticle::charge() const {
  return GetParticleCharge(id);
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle Mass_____________________________________________________________________
double BasicParticle::mass() const {
  return GetParticleMass(id);
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle Kinetic Energy___________________________________________________________
double BasicParticle::ke() const {
  return e() - mass();
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle Total Energy_____________________________________________________________
double BasicParticle::e() const {
  return std::hypot(p_mag(), mass());
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle Total Momentum___________________________________________________________
double BasicParticle::p_mag() const {
  return G4ThreeVector(px, py, pz).mag();
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle Momentum Unit Vector_____________________________________________________
const G4ThreeVector BasicParticle::p_unit() const {
  return G4ThreeVector(px, py, pz).unit();
}
//----------------------------------------------------------------------------------------------

//__Get Basic Particle Momentum Vector__________________________________________________________
const G4ThreeVector BasicParticle::p() const {
  return G4ThreeVector(px, py, pz);
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle PT_______________________________________________________________________
void BasicParticle::set_pT(double new_pT) {

// triplet.pT * std::sinh(triplet.eta),
// triplet.pT * std::sin(triplet.phi),
// -triplet.pT * std::cos(triplet.phi)


  set_pseudo_lorentz_triplet(new_pT, eta(), phi());
}
//----------------------------------------------------------------------------------------------

namespace { ////////////////////////////////////////////////////////////////////////////////////

//__Convert Eta to Theta________________________________________________________________________
long double _eta_to_theta(const long double eta) {
  const auto subangle = 2.0L * std::atan(std::exp(-std::abs(eta)));
  return (eta < 0) ? 3.1415926535897932384626L - subangle : subangle;
}
//----------------------------------------------------------------------------------------------

//__2D-Rotation_________________________________________________________________________________
const std::pair<long double, long double> _rotate(const long double x,
                                                  const long double y,
                                                  const long double theta) {
  const long double cosine{std::cos(theta)};
  const long double sine{std::sin(theta)};
  return {x * cosine - y * sine, x * sine + y * cosine};
}
//----------------------------------------------------------------------------------------------

} /* anonymous namespace */ ////////////////////////////////////////////////////////////////////

//__Set Basic Particle ETA______________________________________________________________________
void BasicParticle::set_eta(double new_eta) {
  const auto rotation = _rotate(px, -pz, _eta_to_theta(new_eta) - _eta_to_theta(eta()));
  px = rotation.first;
  pz = -rotation.second;
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle PHI______________________________________________________________________
void BasicParticle::set_phi(double new_phi) {
  // triplet.pT * std::sinh(triplet.eta),
  // triplet.pT * std::sin(triplet.phi),
  // -triplet.pT * std::cos(triplet.phi)

  const auto rotation = _rotate(-pz, py, new_phi - phi());
  pz = -rotation.first;
  py = rotation.second;

  // set_pseudo_lorentz_triplet(pT(), eta(), new_phi);
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle PseudoLorentzTriplet_____________________________________________________
void BasicParticle::set_pseudo_lorentz_triplet(double new_pT,
                                               double new_eta,
                                               double new_phi) {
  set_pseudo_lorentz_triplet(PseudoLorentzTriplet{new_pT, new_eta, new_phi});
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle PseudoLorentzTriplet_____________________________________________________
void BasicParticle::set_pseudo_lorentz_triplet(const PseudoLorentzTriplet& triplet) {
  set_p(Convert(triplet));
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle Kinetic Energy___________________________________________________________
void BasicParticle::set_ke(double new_ke) {
  set_p(p_unit() * std::sqrt(new_ke * (new_ke + 2.0L * mass())));
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle Total Momentum___________________________________________________________
void BasicParticle::set_p_mag(double magnitude) {
  set_p(magnitude * p_unit());
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle Momentum Unit Vector_____________________________________________________
void BasicParticle::set_p_unit(double pu_x,
                               double pu_y,
                               double pu_z) {
  set_p_unit({pu_x, pu_y, pu_z});
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle Momentum Unit Vector_____________________________________________________
void BasicParticle::set_p_unit(const G4ThreeVector& new_p_unit) {
  const auto magnitude = p_mag();
  set_p((!magnitude ? 1 : magnitude) * new_p_unit.unit());
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle Momentum Vector__________________________________________________________
void BasicParticle::set_p(double new_px,
                          double new_py,
                          double new_pz) {
  px = new_px;
  py = new_py;
  pz = new_pz;
}
//----------------------------------------------------------------------------------------------

//__Set Basic Particle Momentum Vector__________________________________________________________
void BasicParticle::set_p(const G4ThreeVector& new_p) {
  set_p(new_p.x(), new_p.y(), new_p.z());
}
//----------------------------------------------------------------------------------------------

//__Set Particle Vertex_________________________________________________________________________
void Particle::set_vertex(double new_x,
                          double new_y,
                          double new_z) {
  x = new_x;
  y = new_y;
  z = new_z;
}
//----------------------------------------------------------------------------------------------

//__Set Particle Vertex_________________________________________________________________________
void Particle::set_vertex(double new_t,
                          double new_x,
                          double new_y,
                          double new_z) {
  t = new_t;
  x = new_x;
  y = new_y;
  z = new_z;
}
//----------------------------------------------------------------------------------------------

//__Set Particle Vertex_________________________________________________________________________
void Particle::set_vertex(double new_t,
                          const G4ThreeVector& vertex) {
  set_vertex(new_t, vertex.x(), vertex.y(), vertex.z());
}
//----------------------------------------------------------------------------------------------

//__Set Particle Vertex_________________________________________________________________________
void Particle::set_vertex(const G4ThreeVector& vertex) {
  set_vertex(vertex.x(), vertex.y(), vertex.z());
}
//----------------------------------------------------------------------------------------------

//__Add Momentum and Vertex Particle To Event___________________________________________________
void AddParticle(const Particle& particle,
                 G4Event& event) {
  const auto vertex = new G4PrimaryVertex(particle.x, particle.y, particle.z, particle.t);
  vertex->SetPrimary(new G4PrimaryParticle(particle.id, particle.px, particle.py, particle.pz));
  event.AddPrimaryVertex(vertex);
}
//----------------------------------------------------------------------------------------------

} /* namespace Physics */ //////////////////////////////////////////////////////////////////////

} } /* namespace MATHUSLA::MU */