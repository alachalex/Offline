#ifndef Mu2eKinKal_KKFit_hh
#define Mu2eKinKal_KKFit_hh
//
// helper class for constructing KinKal Fits using Mu2e data
//
#include "Offline/Mu2eKinKal/inc/KKStrawHit.hh"
#include "Offline/Mu2eKinKal/inc/KKStrawHitCluster.hh"
#include "Offline/Mu2eKinKal/inc/KKTrack.hh"
#include "Offline/Mu2eKinKal/inc/KKStrawXing.hh"
#include "Offline/Mu2eKinKal/inc/KKStrawMaterial.hh"
#include "Offline/Mu2eKinKal/inc/KKCaloHit.hh"
#include "Offline/Mu2eKinKal/inc/KKFitUtilities.hh"
#include "Offline/Mu2eKinKal/inc/KKFitSettings.hh"
#include "Offline/Mu2eKinKal/inc/WireHitState.hh"
// art includes
#include "canvas/Persistency/Common/Ptr.h"
#include "art/Framework/Principal/Handle.h"
// mu2e Includes
#include "Offline/TrackerConditions/inc/StrawResponse.hh"
#include "Offline/DataProducts/inc/PDGCode.hh"
#include "Offline/DataProducts/inc/StrawIdMask.hh"
#include "Offline/TrackerGeom/inc/Tracker.hh"
#include "Offline/CalorimeterGeom/inc/Calorimeter.hh"
#include "Offline/RecoDataProducts/inc/KalSeed.hh"
#include "Offline/RecoDataProducts/inc/HelixSeed.hh"
#include "Offline/RecoDataProducts/inc/ComboHit.hh"
#include "Offline/RecoDataProducts/inc/CaloCluster.hh"
#include "Offline/RecoDataProducts/inc/StrawHitFlag.hh"
#include "Offline/RecoDataProducts/inc/StrawHitIndex.hh"
#include "Offline/RecoDataProducts/inc/KalSeedAssns.hh"
// KinKal includes
#include "KinKal/Fit/Status.hh"
#include "KinKal/Fit/Config.hh"
#include "KinKal/Trajectory/ParticleTrajectory.hh"
#include "KinKal/Trajectory/PiecewiseClosestApproach.hh"
#include "KinKal/Trajectory/Line.hh"
// Other
#include "cetlib_except/exception.h"
#include <memory>
#include <cmath>
#include <algorithm>
namespace mu2e {
  using KinKal::Line;
  using KinKal::TimeRange;
  using KinKal::Status;
  using KinKal::BFieldMap;
  using RESIDCOL = std::array<KinKal::Residual,2>;
  using StrawHitIndexCollection = std::vector<StrawHitIndex>;
  using Mu2eKinKal::Mu2eConfig;
  using CCHandle = art::ValidHandle<CaloClusterCollection>;
  template <class KTRAJ> class KKFit {
    public:
      // fit configuration
      using KKTRK = KKTrack<KTRAJ>;
      using KKTRKPTR = std::unique_ptr<KKTRK>;
      using PKTRAJ = KinKal::ParticleTrajectory<KTRAJ>;
      using PTCA = KinKal::PiecewiseClosestApproach<KTRAJ,Line>;
      using TCA = KinKal::ClosestApproach<KTRAJ,Line>;
      using KKHIT = KinKal::Measurement<KTRAJ>;
      using KKMAT = KinKal::Material<KTRAJ>;
      using KKSTRAWHIT = KKStrawHit<KTRAJ>;
      using KKSTRAWHITPTR = std::shared_ptr<KKSTRAWHIT>;
      using KKSTRAWHITCOL = std::vector<KKSTRAWHITPTR>;
      using KKSTRAWHITCLUSTER = KKStrawHitCluster<KTRAJ>;
      using KKSTRAWHITCLUSTERPTR = std::shared_ptr<KKSTRAWHITCLUSTER>;
      using KKSTRAWHITCLUSTERCOL = std::vector<KKSTRAWHITCLUSTERPTR>;
      using KKSTRAWHITCLUSTERER = KKStrawHitClusterer<KTRAJ>;
      using KKSTRAWXING = KKStrawXing<KTRAJ>;
      using KKSTRAWXINGPTR = std::shared_ptr<KKSTRAWXING>;
      using KKSTRAWXINGCOL = std::vector<KKSTRAWXINGPTR>;
      using KKCALOHIT = KKCaloHit<KTRAJ>;
      using KKCALOHITPTR = std::shared_ptr<KKCALOHIT>;
      using KKCALOHITCOL = std::vector<KKCALOHITPTR>;
      //      using KKPANELHIT = KKPanelHit<KTRAJ>;
      using MEAS = KinKal::Hit<KTRAJ>;
      using MEASPTR = std::shared_ptr<MEAS>;
      using MEASCOL = std::vector<MEASPTR>;
      using EXING = KinKal::ElementXing<KTRAJ>;
      using EXINGPTR = std::shared_ptr<EXING>;
      using EXINGCOL = std::vector<EXINGPTR>;
      // construct from fit configuration objects
      explicit KKFit(Mu2eConfig const& fitconfig);
      // helper functions used to create components of the fit
      void makeStrawHits(Tracker const& tracker,StrawResponse const& strawresponse, BFieldMap const& kkbf, KKStrawMaterial const& smat,
          PKTRAJ const& ptraj, ComboHitCollection const& chcol, StrawHitIndexCollection const& strawHitIdxs,
          KKSTRAWHITCOL& hits, KKSTRAWXINGCOL& exings) const;
      bool makeCaloHit(CCPtr const& cluster, Calorimeter const& calo, PKTRAJ const& pktraj, KKCALOHITCOL& hits) const;
      // extend a track with a new configuration, optionally searching for and adding hits and straw material
      void extendTrack(Config const& config, BFieldMap const& kkbf, Tracker const& tracker,
          StrawResponse const& strawresponse, KKStrawMaterial const& smat, ComboHitCollection const& chcol,
          Calorimeter const& calo, CCHandle const& cchandle,
          KKTRK& kktrk) const;
      KalSeed createSeed(KKTRK const& kktrk, TrkFitFlag const& seedflag, Calorimeter const& calo, std::set<double> const& tsave) const;
      TimeRange range(KKSTRAWHITCOL const& strawhits, KKCALOHITCOL const& calohits, KKSTRAWXINGCOL const& strawxings) const; // time range from a set of hits and element Xings
      bool useCalo() const { return usecalo_; }
      PDGCode::type fitParticle() const { return tpart_;}
      TrkFitDirection fitDirection() const { return tdir_;}
      bool addMaterial() const { return addmat_; }
      auto const& strawHitClusterer() const { return shclusterer_; }
    private:
      void fillTrackerInfo(Tracker const& tracker) const;
      void addStrawHits(Tracker const& tracker,StrawResponse const& strawresponse, BFieldMap const& kkbf, KKStrawMaterial const& smat,
          KKTRK const& kktrk, ComboHitCollection const& chcol, KKSTRAWHITCOL& hits) const;
      void addStraws(Tracker const& tracker, KKStrawMaterial const& smat, KKTRK const& kktrk, KKSTRAWHITCOL const& addhits, KKSTRAWXINGCOL& addexings) const;
      void addCaloHit(Calorimeter const& calo, KKTRK& kktrk, CCHandle cchandle, KKCALOHITCOL& hits) const;
      PDGCode::type tpart_;
      TrkFitDirection tdir_;
      bool addmat_, usecalo_; // flags
      KKSTRAWHITCLUSTERER shclusterer_; // functor to cluster KKStrawHits
      // CaloHit configuration
      double caloDt_; // calo time offset; should come from proditions FIXME!
      double caloPosRes_; // calo cluster transverse position resolution; should come from proditions or CaloCluster FIXME!
      double caloPropSpeed_; // effective light propagation speed in a crystal (including reflections).  Should come from prodtions FIXME
      double minCaloEnergy_; // minimum CaloCluster energy
      double maxCaloDt_; // maximum track-calo time difference
      double maxCaloDoca_; // maximum track-calo DOCA
      // straw hit creation and processing
      double tprec_; // TPOCA calculation nominal precision
      StrawHitFlag addsel_, addrej_; // selection and rejection flags when adding hits
      // parameters controlling adding hits
      float maxStrawHitDoca_, maxStrawHitDt_, maxStrawDoca_;
      int sbuff_; // maximum distance from the track a strawhit can be to consider it for adding.
      int printLevel_;
      // cached info computed from the tracker, used in hit adding; these must be lazy-evaluated as the tracker doesn't exist on construction
      mutable double ymin_, ymax_, umax_; // panel-level info
      mutable double rmin_, rmax_; // plane-level info
      mutable double spitch_;
      mutable bool needstrackerinfo_;
  };

  template <class KTRAJ> KKFit<KTRAJ>::KKFit(Mu2eConfig const& fitconfig) :
    tpart_(static_cast<PDGCode::type>(fitconfig.fitParticle())),
    tdir_(static_cast<TrkFitDirection::FitDirection>(fitconfig.fitDirection())),
    addmat_(fitconfig.addMaterial()),
    usecalo_(fitconfig.useCaloCluster()),
    shclusterer_(StrawIdMask(fitconfig.strawHitClusterLevel()),fitconfig.strawHitClusterDeltaStraw(),fitconfig.strawHitClusterDeltaT()),
    caloDt_(fitconfig.caloDt()),
    caloPosRes_(fitconfig.caloPosRes()),
    caloPropSpeed_(fitconfig.caloPropSpeed()),
    minCaloEnergy_(fitconfig.minCaloEnergy()),
    maxCaloDt_(fitconfig.maxCaloDt()),
    maxCaloDoca_(fitconfig.maxCaloDoca()),
    tprec_(fitconfig.tpocaPrec()),
    addsel_(fitconfig.addHitSelect()),
    addrej_(fitconfig.addHitReject()),
    maxStrawHitDoca_(fitconfig.maxStrawHitDOCA()),
    maxStrawHitDt_(fitconfig.maxStrawHitDt()),
    maxStrawDoca_(fitconfig.maxStrawDOCA()),
    sbuff_(fitconfig.strawBuffer()),
    printLevel_(fitconfig.printLevel()),
    needstrackerinfo_(true)
  {}

  template <class KTRAJ> void KKFit<KTRAJ>::makeStrawHits(Tracker const& tracker,StrawResponse const& strawresponse,BFieldMap const& kkbf, KKStrawMaterial const& smat,
      PKTRAJ const& ptraj, ComboHitCollection const& chcol, StrawHitIndexCollection const& strawHitIdxs,
      KKSTRAWHITCOL& hits, KKSTRAWXINGCOL& exings) const {
    // loop over the individual straw combo hits
    for(auto strawidx : strawHitIdxs) {
      const ComboHit& combohit(chcol.at(strawidx));
      if(combohit.mask().level() != StrawIdMask::uniquestraw){
        throw cet::exception("RECO")<<"mu2e::KKFit: ComboHit error"<< endl;
      }
      const Straw& straw = tracker.getStraw(combohit.strawId());
      auto wline = Mu2eKinKal::hitLine(combohit,straw,strawresponse); // points from the signal to the straw center
      double htime = combohit.correctedTime();
      // use the hit z position to estimate the particle time.
      auto hpos = wline.position3(htime);
      auto ppos = ptraj.position3(htime);
      auto vel = ptraj.velocity(htime);
      double ptime = htime + (hpos.Z()-ppos.Z())/vel.Z();
      CAHint hint(ptime,htime);
      // compute PTCA between the seed trajectory and this straw
      PTCA ptca(ptraj, wline, hint, tprec_ );
      // create the hit
      hits.push_back(std::make_shared<KKSTRAWHIT>(kkbf, ptca, combohit, straw, strawidx, strawresponse));
      // create the material crossing, including this reference
      if(addmat_) exings.push_back(std::make_shared<KKSTRAWXING>(hits.back(),smat));
    }
  }

  template <class KTRAJ> bool KKFit<KTRAJ>::makeCaloHit(CCPtr const& cluster, Calorimeter const& calo, PKTRAJ const& ptraj, KKCALOHITCOL& hits) const {
    bool retval(false);
    // move cluster COG into the tracker frame.  COG is at the front face of the disk
    CLHEP::Hep3Vector cog = calo.geomUtil().mu2eToTracker(calo.geomUtil().diskFFToMu2e( cluster->diskID(), cluster->cog3Vector()));
    // project this along the crystal axis to the SIPM, which is at the back.  This is the point the time measurement corresponds to
    VEC3 ffcog(cog);
    double lcrystal = calo.caloInfo().getDouble("crystalZLength"); // text-keyed lookup is very inefficient FIXME!
    VEC3 crystalF2B = VEC3(0.0,0.0,lcrystal); // this should come directly from the calogeometry, TODO
    VEC3 sipmcog = ffcog + crystalF2B;
    // create the Line trajectory from this information: signal goes towards the sipm
    Line caxis(sipmcog,ffcog,cluster->time()+caloDt_,caloPropSpeed_);
    // find the time the seed traj passes the middle of the crystal
    double zt = Mu2eKinKal::zTime(ptraj,0.5*(sipmcog.Z()+ffcog.Z()),cluster->time());
    CAHint hint( zt, caxis.t0());
    // compute a preliminary PTCA between the seed trajectory and the cluster axis
    PTCA ptca(ptraj, caxis, hint, tprec_ );
    // check that this is within tolerance
    if(ptca.usable() && fabs(ptca.doca()) < maxCaloDoca_){
      // check that the sensor position is within the active position of the crystal
      auto poca = ptca.sensorPoca().Vect();
      if((poca.Z()-ffcog.Z()) > -maxCaloDoca_ &&  (poca.Z()-sipmcog.Z()) < maxCaloDoca_) {
        // create the hit
        double tvar = cluster->timeErr()*cluster->timeErr();
        double wvar = caloPosRes_*caloPosRes_;
        hits.push_back(std::make_shared<KKCALOHIT>(cluster,ptca,tvar,wvar));
        retval = true;
      }
    }
    return retval;
  }

  template <class KTRAJ> void KKFit<KTRAJ>::extendTrack(Config const& exconfig, BFieldMap const& kkbf, Tracker const& tracker,
      StrawResponse const& strawresponse, KKStrawMaterial const& smat, ComboHitCollection const& chcol,
      Calorimeter const& calo, CCHandle const& cchandle,
      KKTRK& kktrk) const {
    KKSTRAWHITCOL addstrawhits;
    KKCALOHITCOL addcalohits;
    KKSTRAWXINGCOL addstrawxings;
    addStrawHits(tracker, strawresponse, kkbf, smat, kktrk, chcol, addstrawhits );
    if(addMaterial())addStraws(tracker, smat, kktrk, addstrawhits, addstrawxings);
    if(useCalo()&&kktrk.caloHits().size()==0)addCaloHit(calo, kktrk, cchandle, addcalohits);
    if(printLevel_ > 1){
      std::cout << "KKTrk extension adding "
        << addstrawhits.size() << " StrawHits and "
        << addcalohits.size() << " CaloHits and "
        << addstrawxings.size() << " Straw Xings" << std::endl;
    }
    if(addstrawhits.size() > 0 || addstrawxings.size() > 0 || addcalohits.size() > 0)
      kktrk.extendTrack(exconfig,addstrawhits,addstrawxings,addcalohits);
  }

  template <class KTRAJ> void KKFit<KTRAJ>::addStrawHits(Tracker const& tracker,StrawResponse const& strawresponse, BFieldMap const& kkbf, KKStrawMaterial const& smat,
      KKTRK const& kktrk, ComboHitCollection const& chcol, KKSTRAWHITCOL& addhits) const {
    auto const& ftraj = kktrk.fitTraj();
    // build the set of existing hits
    std::set<StrawHitIndex> oldhits;
    for(auto const& strawhit : kktrk.strawHits())oldhits.insert(strawhit->strawHitIndex());
    for( size_t ich=0; ich < chcol.size();++ich){
      if(oldhits.find(ich)==oldhits.end()){      // make sure this hit wasn't already found
        ComboHit const& strawhit = chcol[ich];
        if(strawhit.flag().hasAllProperties(addsel_) && (!strawhit.flag().hasAnyProperty(addrej_))){
          double zt = Mu2eKinKal::zTime(ftraj,strawhit.pos().Z(),strawhit.correctedTime());
          if(fabs(strawhit.correctedTime()-zt) < maxStrawHitDt_) {      // compare the measured time with the estimate from the fit
            const Straw& straw = tracker.getStraw(strawhit.strawId());
            auto wline = Mu2eKinKal::hitLine(strawhit,straw,strawresponse);
            double psign = wline.direction().Dot(straw.wireDirection());  // wire distance is WRT straw center, in the nominal wire direction
            double htime = wline.t0() - (straw.halfLength()-psign*strawhit.wireDist())/wline.speed();
            CAHint hint(zt,htime);
            // compute PTCA between the trajectory and this straw
            PTCA ptca(ftraj, wline, hint, tprec_ );
            if(fabs(ptca.doca()) < maxStrawHitDoca_){ // add test of chi TODO
              addhits.push_back(std::make_shared<KKSTRAWHIT>(kkbf, ptca, strawhit, straw, ich, strawresponse));
            }
          }
        }
      }
    }
  }

  template <class KTRAJ> void KKFit<KTRAJ>::addStraws(Tracker const& tracker, KKStrawMaterial const& smat, KKTRK const& kktrk,
      KKSTRAWHITCOL const& addhits, KKSTRAWXINGCOL& addexings) const {
    // this algorithm assumes the track never hits the same straw twice.  That could be violated by reflecting tracks, and could be addressed
    // by including the time of the Xing as part of its identity.  That would slow things down so it remains to be proven it's a problem  TODO
    // build the set of existing straws
    auto const& ftraj = kktrk.fitTraj();
    // pre-compute some tracker info if needed
    if(needstrackerinfo_)fillTrackerInfo(tracker);
    // list the IDs of existing straws: this speeds the search
    std::set<StrawId> oldstraws;
    for(auto const& strawxing : kktrk.strawXings())oldstraws.insert(strawxing->strawId());
    // create materials for straw hits just added
    for(auto const& strawhit : addhits){
      if(oldstraws.find(strawhit->strawId()) == oldstraws.end()){
        addexings.push_back(std::make_shared<KKSTRAWXING>(strawhit,smat));
        oldstraws.insert(strawhit->strawId());
      }
    }
    // Go hierarchically through planes and panels to find new straws hit by this track.
    for(auto const& plane : tracker.planes()){
      if(tracker.planeExists(plane.id())) {
        double plz = plane.origin().z();
        // find the track position in at this plane's central z.
        double zt = Mu2eKinKal::zTime(ftraj,plz,ftraj.range().begin());
        auto plpos = ftraj.position3(zt);
        // rough check on the point radius
        double rho = plpos.Rho();
        if(rho > rmin_ && rho < rmax_){
          // loop over panels in this plane
          for(auto panel_p : plane.panels()){
            auto const& panel = *panel_p;
            // linearly correct position for the track direction due to difference in panel-plane Z position
            auto tdir = ftraj.direction(zt);
            double dz = panel.origin().z()-plz;
            auto papos = plpos + (dz/tdir.Z())*tdir;
            // convert this position into panel coordinates
            CLHEP::Hep3Vector cpos(papos.X(),papos.Y(),papos.Z()); // clumsy translation
            auto pposv = panel.dsToPanel()*cpos;
            // translate the y position into a rough straw number
            int istraw = static_cast<int>(rint( (pposv.y()-ymin_)*spitch_));
            // require this be within the (integral) straw buffer.  This just reduces the number of calls to PTCA
            if(istraw >= -sbuff_ && istraw < static_cast<int>(panel.nStraws()) + sbuff_ ){
              unsigned istrmin = static_cast<unsigned>(std::max(istraw-sbuff_,0));
              // largest straw is the innermost; use that to test length
              if(fabs(pposv.x()) < panel.getStraw(istrmin).halfLength() ) {
                unsigned istrmax = static_cast<unsigned>(std::min(istraw+sbuff_,static_cast<int>(panel.nStraws())-1));
                // loop over straws
                for(unsigned istr = istrmin; istr <= istrmax; ++istr){
                  auto const& straw = panel.getStraw(istr);
                  // add strawExists test TODO
                  // make sure we haven't already seen this straw
                  if(oldstraws.find(straw.id()) == oldstraws.end()){
                    KinKal::VEC3 vp0(straw.wireEnd(StrawEnd::cal));
                    KinKal::VEC3 vp1(straw.wireEnd(StrawEnd::hv));
                    KinKal::VEC3 smid = 0.5*(vp0+vp1);
                    // eventually this trajectory should be a native member of Straw TODO
                    KinKal::Line wline(vp0,vp1,zt,CLHEP::c_light); // time is irrelevant: use speed of light as sprop
                    CAHint hint(zt,zt);
                    // compute PTCA between the trajectory and this straw
                    PTCA ptca(ftraj, wline, hint, tprec_ );
                    double du = (ptca.sensorPoca().Vect()-smid).R();
                    if(fabs(ptca.doca()) < maxStrawDoca_ && du < straw.halfLength()){ // add test of chi TODO
                      addexings.push_back(std::make_shared<KKSTRAWXING>(ptca,smat,straw.id()));
                    }
                  } // not existing straw cut
                } // straws loop
              } // straw length cut
            } // straw index cut
          } // panels loop
        } // radius cut
      } // plane exists cut
    }  // planes loop
  } // end function

  template <class KTRAJ> void KKFit<KTRAJ>::addCaloHit(Calorimeter const& calo, KKTRK& kktrk, CCHandle cchandle, KKCALOHITCOL& hits) const {
    double crystalLength = calo.caloInfo().getDouble("crystalZLength");
    auto const& ftraj = kktrk.fitTraj();
    bool added(false);
    auto cccol = cchandle.product();
    unsigned idisk=0;
    // loop over disks, or until we find a matching cluster
    while(!added && idisk < 2){
      auto ffpos = calo.geomUtil().mu2eToTracker(calo.disk(idisk).geomInfo().frontFaceCenter());
      double rmin = calo.disk(idisk).geomInfo().innerEnvelopeR();
      double rmax = calo.disk(idisk).geomInfo().outerEnvelopeR();
      // test at both faces; if the track is in the right area, test the clusters on this disk
      bool test(false);
      for(int iface=0; iface<2; ++iface){
        double zt = Mu2eKinKal::zTime(ftraj,ffpos.z()+iface*crystalLength,ftraj.range().end());
        auto tpos = ftraj.position3(zt);
        double rho = tpos.Rho();
        if ( rho > rmin && rho < rmax ){
          test=true;
          break;
        }
      }
      if(test){
        for(size_t icc=0;icc < cccol->size(); ++icc){
          auto const& cc = (*cccol)[icc];
          if (static_cast<unsigned>(cc.diskID()) == idisk && cc.energyDep() > minCaloEnergy_){
            auto ccPtr = art::Ptr<CaloCluster>(cchandle,icc);
            added = makeCaloHit(ccPtr,calo,ftraj,hits);
            if(added)break;
          }
        }
      }
      ++idisk;
    }
  }

  template <class KTRAJ> void KKFit<KTRAJ>::fillTrackerInfo(Tracker const& tracker) const {
    // pre-compute some info derived from the tracker
    double strawradius = tracker.strawOuterRadius();
    auto const& frontplane = tracker.planes().front();
    auto const& firstpanel = frontplane.getPanel(0);
    auto const& innerstraw = firstpanel.getStraw(0);
    auto const& outerstraw = firstpanel.getStraw(StrawId::_nstraws-1);
    // compute limits: add some buffer for the finite size of the straw
    auto DStoP = firstpanel.dsToPanel();
    auto innerstraw_origin = DStoP*innerstraw.origin();
    auto outerstraw_origin = DStoP*outerstraw.origin();
    ymin_ = innerstraw_origin.y();
    ymax_ = outerstraw_origin.y();
    umax_ = innerstraw.halfLength() + strawradius; // longest possible straw
    // plane-level variables: these add some buffer
    rmin_ = innerstraw_origin.y() - sbuff_*strawradius;
    rmax_ = outerstraw.wireEnd(StrawEnd::cal).mag() + sbuff_*strawradius;
    spitch_ = (StrawId::_nstraws-1)/(ymax_-ymin_);
    needstrackerinfo_= false;
  }


  template <class KTRAJ> TimeRange KKFit<KTRAJ>::range(KKSTRAWHITCOL const& strawhits, KKCALOHITCOL const& calohits, KKSTRAWXINGCOL const& strawxings) const{
    double tmin = std::numeric_limits<float>::max();
    double tmax = std::numeric_limits<float>::min();
    for( auto const& strawhit : strawhits) {
      tmin = std::min(tmin,strawhit->time());
      tmax = std::max(tmax,strawhit->time());
    }
    for( auto const& calohit : calohits) {
      tmin = std::min(tmin,calohit->time());
      tmax = std::max(tmax,calohit->time());
    }
    for( auto const& strawxing : strawxings) {
      tmin = std::min(tmin,strawxing->time());
      tmax = std::max(tmax,strawxing->time());
    }
    return TimeRange(tmin,tmax);
  }

  template <class KTRAJ> KalSeed KKFit<KTRAJ>::createSeed(KKTRK const& kktrk, TrkFitFlag const& seedflag, Calorimeter const& calo, std::set<double> const& savetimes) const {
    TrkFitFlag fflag(seedflag);  // initialize the flag with the seed fit flag
    if(kktrk.fitStatus().usable())fflag.merge(TrkFitFlag::kalmanOK);
    if(kktrk.fitStatus().status_ == Status::converged) fflag.merge(TrkFitFlag::kalmanConverged);
    if(addmat_)fflag.merge(TrkFitFlag::MatCorr);
    if(kktrk.config().bfcorr_ )fflag.merge(TrkFitFlag::BFCorr);
    // explicit T0 is needed for backwards-compatibility; sample from the appropriate trajectory piece
    auto const& fittraj = kktrk.fitTraj();
    double tz0 = Mu2eKinKal::zTime(fittraj,0.0,fittraj.range().begin());
    auto const& t0piece = fittraj.nearestPiece(tz0);
    double t0val = t0piece.paramVal(KTRAJ::t0_);
    //    double t0sig = sqrt(t0piece.paramVar(KTRAJ::t0_)); Temporary FIXME
    double t0sig = sqrt(t0piece.params().covariance()(KTRAJ::t0_,KTRAJ::t0_));
    HitT0 t0(t0val,t0sig);
    // create the shell for the output.  Note the (obsolete) flight length is given as t0
    KalSeed fseed(tpart_,tdir_,fflag,t0.t0());
    auto const& fstatus = kktrk.fitStatus();
    fseed._chisq = fstatus.chisq_.chisq();
    fseed._fitcon = fstatus.chisq_.probability();
    fseed._nseg = fittraj.pieces().size();
    // loop over track components and store them
    fseed._hits.reserve(kktrk.strawHits().size());
    for(auto const& strawhit : kktrk.strawHits() ) {
      Residual utres, udres;
      if(kktrk.fitStatus().usable()) {
        // compute unbiased residuals
        udres = strawhit->residual(Mu2eKinKal::dresid);
        if(strawhit->refResidual(Mu2eKinKal::tresid).active()) utres = strawhit->residual(Mu2eKinKal::tresid);
      }
      fseed._hits.emplace_back(strawhit->strawHitIndex(),strawhit->hit(),
          strawhit->closestApproach().tpData(),
          strawhit->unbiasedClosestApproach().tpData(),
          utres, udres,
          strawhit->refResidual(Mu2eKinKal::tresid),
          strawhit->refResidual(Mu2eKinKal::dresid),
          strawhit->fillDriftInfo(), strawhit->hitState());
    }
    if(kktrk.caloHits().size() > 0){
      auto const& calohit = kktrk.caloHits().front(); // for now take the front: not sure if there will ever be >1 TODO
      auto const& ca = calohit->closestApproach();
      StrawHitFlag hflag;
      if(calohit->active()){
        hflag.merge(StrawHitFlag::active);
        hflag.merge(StrawHitFlag::doca);
      }
      // calculate the unbiased time residual
      auto tres = (kktrk.fitStatus().usable()) ? calohit->residual(0) : calohit->refResidual(0);
      // calculate the cluster depth
      // First, find the Z of the front face of this disk
      double frontz = calo.geomUtil().mu2eToTracker(calo.disk(calohit->caloCluster()->diskID()).geomInfo().frontFaceCenter()).z();
      // calculate the distance from the front to the POCA, projected along the track
      float clen = (ca.sensorPoca().Z()- frontz)/ca.particleTraj().direction(ca.particleToca()).Z();
      fseed._chit = TrkCaloHitSeed(calohit->caloCluster(),hflag,
          clen,
          ca.tpData(),tres,ca.particleTraj().momentum(ca.particleToca()));
    }
    fseed._straws.reserve(kktrk.strawXings().size());
    for(auto const& sxing : kktrk.strawXings()) {
      std::array<double,3> dmom = {0.0,0.0,0.0}, momvar = {0.0,0.0,0.0};
      // compute energy loss
      sxing->materialEffects(KinKal::TimeDir::forwards, dmom, momvar);
      VEC3 dm(dmom[0],dmom[1],dmom[2]);
      // find material crossing properties
      double gaspath(0.0);
      double radfrac(0.0);
      for(auto const& matxing : sxing->matXings()){
        if(matxing.dmat_.name().compare(5,3,"gas",3)==0)gaspath = matxing.plen_;
        radfrac += matxing.dmat_.radiationFraction(matxing.plen_);
      }
      auto const& tpocad = sxing->closestApproach();
      fseed._straws.emplace_back(sxing->strawId(),
          tpocad.doca(),
          tpocad.particleToca(),
          tpocad.sensorToca(),
          gaspath,
          radfrac,
          dm.R(),
          sxing->active() );
    }
    // sample the fit at the requested times and save those segments.  Uniqueness needs to be checked in the calling function
    fseed._segments.reserve(savetimes.size());
    for(auto time : savetimes) fseed._segments.emplace_back(fittraj.nearestPiece(time),time);
    return fseed;
  }

}
#endif
