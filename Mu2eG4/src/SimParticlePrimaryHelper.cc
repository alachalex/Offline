// Andrei Gaponenko, 2013

#include "Mu2eG4/inc/SimParticlePrimaryHelper.hh"
#include "art/Framework/Principal/Event.h"

#include "MCDataProducts/inc/StepPointMC.hh"
#include "MCDataProducts/inc/StageParticle.hh"

#include "cetlib_except/exception.h"

#include <iostream>
#define AGDEBUG(stuff) std::cerr<<"AG: "<<__FILE__<<", line "<<__LINE__<<": "<<stuff<<std::endl;

namespace mu2e {

  //================================================================
  SimParticlePrimaryHelper::SimParticlePrimaryHelper(const art::Event* event,
                                                     const art::ProductID& simProdID,
                                                     const art::EDProductGetter* sim_prod_getter):
    simProdID_(simProdID),
    event_(event),
    simProductGetter_(sim_prod_getter)
  {}

  //================================================================
  art::Ptr<GenParticle> SimParticlePrimaryHelper::genParticlePtr(int g4TrkID) const {
    art::Ptr<GenParticle> res;
    auto pgen = std::get_if<art::Ptr<GenParticle> >(&entries_.at(g4TrkID - 1));
    if(pgen) {
      res = *pgen;
    }
    return res;
  }

  //================================================================
  art::Ptr<SimParticle>  SimParticlePrimaryHelper::simParticlePrimaryPtr(int g4TrkID) const {
    auto& v = entries_.at(g4TrkID - 1);

    if(std::holds_alternative<art::Ptr<GenParticle>>(v)) {
      return art::Ptr<SimParticle>();
    }
    else {

      SimParticle::key_type id;
      if(std::holds_alternative<const SimParticle*>(v)) {
        std::cout<<"simParticlePrimaryPtr from SimParticle*"<<std::endl;
        id = std::get<const SimParticle*>(v)->id();
      }
      else if(std::holds_alternative<const  StepPointMC*>(v)) {
        std::cout<<"simParticlePrimaryPtr from StepPointMC*"<<std::endl;
        id = std::get<const StepPointMC*>(v)->simParticle()->id();
      }
      else if(std::holds_alternative<const StageParticle*>(v)) {
        std::cout<<"simParticlePrimaryPtr from StageParticle*"<<std::endl;
        id = std::get<const StageParticle*>(v)->parent()->id();
      }

      try { id.ensure_valid(); }
      catch(cet::exception& e) {
        throw cet::exception("ImplementationError", "SimParticlePrimaryHelper::simParticlePrimaryPtr(int) missed a variant alternative", e);
      }

      return art::Ptr<SimParticle>(simProdID_, id.asUint(), simProductGetter_);
    }
  }

  //================================================================
  SimParticlePrimaryHelper::InputParticle SimParticlePrimaryHelper::getEntry(int g4TrkID) const {
    AGDEBUG("size = "<<entries_.size()<<", g4TrkID = "<<g4TrkID);
    return entries_.at(g4TrkID-1);
  }

  //================================================================

}
