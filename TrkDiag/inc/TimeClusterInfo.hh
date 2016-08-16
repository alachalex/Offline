//
// information about time clusters for diagnostics
// original author Dave Brown (LBNL) 8/9/2016
//
#ifndef TrkDiag_TimeClusterInfo_HH
#define TrkDiag_TimeClusterInfo_HH
// mu2e includes
#include "DataProducts/inc/threevec.hh"
// root includes
#include "Rtypes.h"
// C++ includes
#include <functional>
#include <string>
namespace mu2e {
  struct TimeClusterInfo {
    TimeClusterInfo() { reset(); }
    Int_t _tcindex; // cluster ID
    Int_t _nhits; // # of hits in this cluster
    Int_t _ncehits; // # of CE hits in this cluster
    Float_t _time; // cluster time
    Float_t _minhtime, _maxhtime; // min and max cluster hit time
    threevec _pos; // average position of cluster
    
    void reset() { _tcindex = _nhits = _ncehits = -1; _time=0.0; _pos.reset(); }
    static std::string const& leafnames() { static const std::string leaves =
      std::string("clusterindex/I:nhits/I:ncehits/I:time/F:")+threevec::leafnames();
      return leaves;
    } 
  };

  struct TimeClusterHitInfo {
    TimeClusterHitInfo() { reset(); }
    Float_t _dt; // time relative to the cluster
    Float_t _dphi; // resolved phi relative to the cluster
    Float_t _rho; // transverse radius of this hit
    Float_t _mva; // cluster MVA output
    Int_t _mcpdg, _mcgen, _mcproc; // MC truth info for this hit
    void reset() { _dt = _dphi = _rho = _mva = -1000.0; _mcpdg = _mcgen = _mcproc = 0; }
  };
    
  struct MCClusterInfo {  
    MCClusterInfo() { reset(); }
    Int_t	_nce; // # of conversion electron hits
    Int_t	_ncesel; // # of selected conversion electron hits
    Int_t	_nceclust; // # of conversion electron hits found in clusters
    Float_t	_time; // average time of CE hits (doesn't include drift!)
    threevec	_pos; // average position of cluster
    Float_t	_maxdphi; // max dphi WRT average
    Float_t	_minrho; // min rho WRT average
    Float_t	_maxrho; // max rho WRT average
    void reset() { _nce = _ncesel = _nceclust = -1; _time = _maxdphi = _maxrho = _minrho = 0.0; _pos.reset();}
    static std::string const& leafnames() {
      static const std::string leaves =
	std::string("nce/I:ncesel/I:nceclust/I:time/F:")
	+threevec::leafnames()
	+std::string(":maxdphi/F:minrho/F:maxrho/F");
      return leaves;
    } 
  };
// predicate to sort by decreasing # of CE hits
  struct NCEComp : public std::binary_function<TimeClusterInfo,TimeClusterInfo, bool> {
    bool operator()(TimeClusterInfo const& x,TimeClusterInfo const& y) { return x._ncehits > y._ncehits; }
  };

}
#endif
