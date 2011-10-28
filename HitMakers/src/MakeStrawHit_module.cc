//
// An EDProducer Module that reads StepPointMC objects and turns them into
// StrawHit objects.
//
// $Id: MakeStrawHit_module.cc,v 1.10 2011/10/28 18:47:06 greenc Exp $
// $Author: greenc $
// $Date: 2011/10/28 18:47:06 $
//
// Original author Rob Kutschke. Updated by Ivan Logashenko.
//                               Updated by Hans Wenzel to include sigma in deltat

// C++ includes.
#include <iostream>
#include <string>
#include <cmath>

// Framework includes.
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Selector.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Principal/Handle.h"

// From the art tool-chain
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Mu2e includes.
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/getTrackerOrThrow.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "LTrackerGeom/inc/LTracker.hh"
#include "MCDataProducts/inc/StepPointMCCollection.hh"
#include "RecoDataProducts/inc/StrawHit.hh"
#include "RecoDataProducts/inc/StrawHitCollection.hh"
#include "MCDataProducts/inc/StrawHitMCTruth.hh"
#include "MCDataProducts/inc/StrawHitMCTruthCollection.hh"
#include "MCDataProducts/inc/PtrStepPointMCVectorCollection.hh"
#include "Mu2eUtilities/inc/TwoLinePCA.hh"
#include "Mu2eUtilities/inc/LinePointPCA.hh"
#include "ConditionsService/inc/ConditionsHandle.hh"
#include "ConditionsService/inc/TrackerCalibrations.hh"

// Other includes.
#include "CLHEP/Random/RandGaussQ.h"
#include "CLHEP/Vector/ThreeVector.h"

using namespace std;
//using art::Event;

namespace mu2e {

  // Utility class (structure) to hold calculated drift time for G4 hits

  class StepHit {
    typedef art::Handle<StepPointMCCollection> const* PHandle;

  public:

    art::Ptr<StepPointMC> _ptr;
    double _edep;
    double _dca;
    double _driftTime;
    double _distanceToMid;
    double _t1;
    double _t2;

    StepHit( art::Ptr<StepPointMC> const& ptr, double edep, double dca, double driftT, double toMid, double t1, double t2):
      _ptr(ptr), _edep(edep), _dca(dca), _driftTime(driftT),
      _distanceToMid(toMid), _t1(t1), _t2(t2) {}

    // This operator is overloaded in order to time-sort the hits
    bool operator <(const StepHit& b) const { return (_t1 < b._t1); }

  };

  //--------------------------------------------------------------------
  //
  //
  class MakeStrawHit : public art::EDProducer {

  public:
    explicit MakeStrawHit(fhicl::ParameterSet const& pset) :

      // Parameters
      _diagLevel(pset.get<int>("diagLevel",0)),
      _maxFullPrint(pset.get<int>("maxFullPrint",5)),
      _trackerStepPoints(pset.get<string>("trackerStepPoints","tracker")),
      _t0Sigma(pset.get<double>("t0Sigma",0.0)), // ns
      _minimumEnergy(pset.get<double>("minimumEnergy",0.0001)), // MeV
      _minimumLength(pset.get<double>("minimumLength",0.01)),   // mm
      _driftVelocity(pset.get<double>("driftVelocity",0.05)),   // mm/ns
      _driftSigma(pset.get<double>("driftSigma",0.1)),          // mm
      _minimumTimeGap(pset.get<double>("minimumTimeGap",100.0)),// ns
      _g4ModuleLabel(pset.get<string>("g4ModuleLabel")),

      // Random number distributions
      _gaussian( createEngine( get_seed_value(pset)) ),

      _messageCategory("HITS"),

      // Control some information messages.
      _firstEvent(true){

      // Tell the framework what we make.
      produces<StrawHitCollection>();
      produces<StrawHitMCTruthCollection>();
      produces<PtrStepPointMCVectorCollection>("StrawHitMCPtr");

    }
    virtual ~MakeStrawHit() { }

    virtual void beginJob();

    void produce( art::Event& e);

  private:

    typedef std::map<StrawIndex,std::vector<art::Ptr<StepPointMC> > > StrawHitMap;

    // Diagnostics level.
    int _diagLevel;

    // Limit on number of events for which there will be full printout.
    int _maxFullPrint;

    // Name of the tracker StepPoint collection
    std::string _trackerStepPoints;

    // Parameters
    double _t0Sigma;        // T0 spread in ns
    double _minimumEnergy;  // minimum energy deposition of G4 step
    double _minimumLength;  // is G4Step is shorter than this, consider it a point
    double _driftVelocity;
    double _driftSigma;
    double _minimumTimeGap;
    string _g4ModuleLabel;  // Name of the module that made these hits.

    // Random number distributions
    CLHEP::RandGaussQ _gaussian;

    // A category for the error logger.
    const std::string _messageCategory;

    // Give some informationation messages only on the first event.
    bool _firstEvent;

    void fillHitMap ( art::Event const& event, StrawHitMap& hitmap );

  };

  void MakeStrawHit::beginJob(){
  }


  // Find StepPointMCs in the event and use them to fill the hit map.
  void MakeStrawHit::fillHitMap ( art::Event const& event, StrawHitMap& hitmap){

    // This selector will select only data products with the given instance name.
    art::ProductInstanceNameSelector selector("tracker");

    typedef std::vector< art::Handle<StepPointMCCollection> > HandleVector;

    // Get all of the tracker StepPointMC collections from the event:
    HandleVector stepsHandles;
    event.getMany( selector, stepsHandles);

    // Informational message on the first event.
    if ( _firstEvent ) {
      mf::LogInfo log(_messageCategory);
      log << "MakeStrawHit::fillHitMap will uses StepPointMCs from: \n";
      for ( HandleVector::const_iterator i=stepsHandles.begin(), e=stepsHandles.end();
            i != e; ++i ){

        art::Provenance const& prov(*(i->provenance()));
        log  << "   " << prov.branchName() << "\n";

      }
    }

    // Populate hitmap from stepsHandles.
    for ( HandleVector::const_iterator i=stepsHandles.begin(), e=stepsHandles.end();
          i != e; ++i ){

      art::Handle<StepPointMCCollection> const& handle(*i);
      StepPointMCCollection const& steps(*handle);

      int index(0);
      for ( StepPointMCCollection::const_iterator j = steps.begin(), je=steps.end();
            j != je; ++j, ++index){

        StepPointMC const& step(*j);
        if( step.totalEDep()<_minimumEnergy ) continue; // Skip steps with very low energy deposition
        StrawIndex straw_id = step.strawIndex();

        // The main event for this method.
        hitmap[straw_id].push_back( art::Ptr<StepPointMC>(handle,index) );
      }
    }

  }   // end MakeStrawHit::fillHitMap 


  void
  MakeStrawHit::produce(art::Event& event) {

    if ( _diagLevel > 0 ) cout << "MakeStrawHit: produce() begin" << endl;

    static int ncalls(0);
    ++ncalls;

    // Get a reference to one of the L or T trackers.
    // Throw exception if not successful.
    const Tracker& tracker = getTrackerOrThrow();

    // Containers to hold the output information.
    auto_ptr<StrawHitCollection>             strawHits(new StrawHitCollection);
    auto_ptr<StrawHitMCTruthCollection>      truthHits(new StrawHitMCTruthCollection);
    auto_ptr<PtrStepPointMCVectorCollection> mcptrHits(new PtrStepPointMCVectorCollection);

    // Calculate T0 for this event
    double t0 = _gaussian.fire(0.,_t0Sigma);

    // Handle to the conditions service
    ConditionsHandle<TrackerCalibrations> trackerCalibrations("ignored");

    // Organize the StepPointMCs by straws.
    StrawHitMap hitmap;
    fillHitMap( event, hitmap );

    // Temporary working space per straw.
    vector<StepHit> straw_hits;

    // Loop over all straws and create StrawHits. There can be several
    // hits per straw if they are separated by time. The general algorithm
    // is as follows: calculate signal time for each G4step, order them
    // in time and look for gaps. If gap exceeds _minimumTimeGap create
    // separate hit.

    for(StrawHitMap::const_iterator istraw = hitmap.begin(); istraw != hitmap.end(); ++istraw) {

      if ( ncalls < _maxFullPrint && _diagLevel > 2 ) {
        cout << "MakeStrawHit: straw ID=" << istraw->first
             << ": number of G4 step hits " << istraw->second.size()
             << endl;
      }

      // Get the straw information, also by reference.
      StrawIndex straw_id = istraw->first;
      Straw const&      straw = tracker.getStraw(straw_id);
      CLHEP::Hep3Vector const& mid   = straw.getMidPoint();
      CLHEP::Hep3Vector const& w     = straw.getDirection();
      double strawHalfLength         = straw.getHalfLength();

      const double signalVelocity = trackerCalibrations->SignalVelocity(straw_id);

      // The StepPointMCs that are on this straw.
      vector<art::Ptr<StepPointMC> > const& ihits = istraw->second;

      // Prepare working sapce.
      straw_hits.clear();
      straw_hits.reserve(ihits.size());

      // Loop over all hits found for this straw
      int nn(0);
      for( vector<art::Ptr<StepPointMC> >::const_iterator i=ihits.begin(), e=ihits.end();
           i !=e ; ++i, ++nn ){

        StepPointMC const& hit = **i;
        CLHEP::Hep3Vector  const& pos = hit.position();
        CLHEP::Hep3Vector  const& mom = hit.momentum();
        double length  = hit.stepLength();
        double edep    = hit.totalEDep();
        double hitTime = hit.time();

        if ( ncalls < _maxFullPrint && _diagLevel > 2 ) {
          cout << "MakeStrawHit: Hit #" << nn << " : length=" << length
               << " energy=" << edep << " time=" << hitTime
               << endl;
        }

        // Calculate the drift distance from this step.
        double hit_dca;
        CLHEP::Hep3Vector hit_pca;

        if( length < _minimumLength ) {

          // If step length is very small, consider it a point

          LinePointPCA pca(mid, w, pos);
          hit_dca = pca.dca();
          hit_pca = pca.pca();

        } else {

          // Step is not a point. Calculate the distance between two lines.

          TwoLinePCA pca( mid, w, pos, mom);
          CLHEP::Hep3Vector const& p2 = pca.point2();

          if( (pos-p2).mag()<=length && (pos-p2).dot(mom)<=0 ) {

            // If the point of closest approach is within the step and wire - thats it.
            hit_dca = pca.dca();
            hit_pca = pca.point1();

          } else {

            // The point of closest approach is not within the step. In this case
            // the closes distance should be calculated from the ends

            LinePointPCA pca1(mid, w, pos);
            LinePointPCA pca2(mid, w, pos+mom.unit()*length);
            if( pca1.dca() < pca2.dca() ) {
              hit_dca = pca1.dca();
              hit_pca = pca1.pca();
            } else {
              hit_dca = pca2.dca();
              hit_pca = pca2.pca();
            }

          }

        } // drift distance calculation

        // Calculate signal time. It is Geant4 time + signal propagation time
        // t1 is signal time at positive end (along w vector),
        // t2 - at negative end (opposite to w vector)


        double driftTime = (hit_dca + _gaussian.fire(0.,_driftSigma))/_driftVelocity;
        double distanceToMiddle = (hit_pca-mid).dot(w);
        double hit_t1 = t0 + hitTime + driftTime + (strawHalfLength-distanceToMiddle)/signalVelocity;
        double hit_t2 = t0 + hitTime + driftTime + (strawHalfLength+distanceToMiddle)/signalVelocity;

        straw_hits.push_back( StepHit( *i,edep,hit_dca,driftTime,distanceToMiddle,hit_t1,hit_t2));

      } // loop over hits

      // Now that we calculated estimated signal time for all G4Steps, we can analyze
      // the time structure and create StrawHits

      // First we need to sort StepHits according to t1 time
      sort(straw_hits.begin(), straw_hits.end() );

      if ( ncalls < _maxFullPrint && _diagLevel > 2 ) {
        for( size_t i=0; i<straw_hits.size(); i++ ) {
          cout << "MakeStrawHit: StepHit # (" << straw_hits[i]._ptr.id() << " " << straw_hits[i]._ptr.key() << ")"
               << " DCA=" << straw_hits[i]._dca
               << " driftT=" << straw_hits[i]._driftTime
               << " distToMid=" << straw_hits[i]._distanceToMid
               << " t1=" << straw_hits[i]._t1
               << " t2=" << straw_hits[i]._t2
               << " edep=" << straw_hits[i]._edep
               << " t0=" << t0
               << endl;
        }
      }

      // Now loop over all StepHits and create StrawHits as needed

      if( straw_hits.size()<1 ) continue; // This should never be needed. Added for safety.

      double digi_time   = straw_hits[0]._t1;
      double digi_t2     = straw_hits[0]._t2;
      double digi_edep   = straw_hits[0]._edep;
      double digi_driftT = straw_hits[0]._driftTime;
      double digi_toMid  = straw_hits[0]._distanceToMid;
      double digi_dca    = straw_hits[0]._dca;
      double deltadigitime;
      double distSigma;
      PtrStepPointMCVector mcptr;
      mcptr.push_back( straw_hits[0]._ptr );

      for( size_t i=1; i<straw_hits.size(); i++ ) {
        if( (straw_hits[i]._t1-straw_hits[i-1]._t1) > _minimumTimeGap ) {
          // The is bit time gap - save current data as a hit...
          distSigma = trackerCalibrations->TimeDivisionResolution(straw_id, (strawHalfLength-digi_toMid)/(2.*strawHalfLength) );
          deltadigitime = (digi_t2-digi_time)+_gaussian.fire(0.,2.*distSigma/signalVelocity);

          strawHits->push_back(StrawHit(straw_id,digi_time,deltadigitime,digi_edep));
          truthHits->push_back(StrawHitMCTruth(t0,digi_driftT,digi_dca,digi_toMid));
          mcptrHits->push_back(mcptr);
          // ...and create new hit
          mcptr.clear();
          mcptr.push_back( straw_hits[i]._ptr );
          digi_time   = straw_hits[i]._t1;
          digi_t2     = straw_hits[i]._t2;
          digi_edep   = straw_hits[i]._edep;
          digi_driftT = straw_hits[i]._driftTime;
          digi_toMid  = straw_hits[i]._distanceToMid;
          digi_dca    = straw_hits[i]._dca;
        } else {
          // Append existing hit
          if( digi_t2 > straw_hits[i]._t2 ) digi_t2 = straw_hits[i]._t2;
          digi_edep += straw_hits[i]._edep;
          mcptr.push_back( straw_hits[i]._ptr );
        }
      }

      distSigma = trackerCalibrations->TimeDivisionResolution(straw_id, (strawHalfLength-digi_toMid)/(2.*strawHalfLength) );
      deltadigitime = (digi_t2-digi_time)+_gaussian.fire(0.,2.*distSigma/signalVelocity);
      strawHits->push_back(StrawHit(straw_id,digi_time,deltadigitime,digi_edep));
      truthHits->push_back(StrawHitMCTruth(t0,digi_driftT,digi_dca,digi_toMid));
      mcptrHits->push_back(mcptr);

    }

    if ( ncalls < _maxFullPrint && _diagLevel > 2 ) {
      cout << "MakeStrawHit: Total number of hit straws = " << strawHits->size() << endl;
      for( size_t i=0; i<strawHits->size(); ++i ) {
        cout << "MakeStrawHit: Straw #" << (*strawHits)[i].strawIndex()
             << " time="  << (*strawHits)[i].time()
             << " dt="    << (*strawHits)[i].dt()
             << " edep="  << (*strawHits)[i].energyDep()
             << " nsteps="<< (*mcptrHits)[i].size()
             << endl;
      }
    }

    // Add the output hit collection to the event
    event.put(strawHits);
    event.put(truthHits);
    event.put(mcptrHits,"StrawHitMCPtr");

    if ( _diagLevel > 0 ) cout << "MakeStrawHit: produce() end" << endl;

    // Done with the first event; disable some messages.
    _firstEvent = false;

  } // end MakeStrawHit::produce.

} // end namespace mu2e

using mu2e::MakeStrawHit;
DEFINE_ART_MODULE(MakeStrawHit);
