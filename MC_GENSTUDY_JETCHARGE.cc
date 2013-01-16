// -*- C++ -*-
//System includes
#include <map>
#include <algorithm>

//Rivet framework
#include "Rivet/Analysis.hh"
#include "Rivet/RivetAIDA.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Tools/ParticleIdUtils.hh"

//Projections
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/WFinder.hh"
#include "Rivet/Projections/FastJets.hh"
//#include "Rivet/Projections/MissingMomentum.hh"

// Histogram booking
#include "LWH/Histogram1D.h"
#include "LWH/Histogram2D.h"

typedef std::map<std::string,AIDA::IHistogram1D*> BookedHistos;
namespace Rivet {

  /// Generic analysis looking at various distributions of final state particles
  class MC_GENSTUDY_JETCHARGE : public Analysis {
  public:
    /// Constructor
    MC_GENSTUDY_JETCHARGE()
      : Analysis("MC_GENSTUDY_JETCHARGE")
    { for(unsigned int i=0; i < 4; i++) _nPassing[i]=0;    }

    /// @name Analysis methods
    //@{
    /// Book histograms and initialise projections before the run
    void init() {
      // Projections
      const FinalState fs;
      addProjection(fs, "FS");
      std::vector<std::pair<double, double > > muonEtaRanges;
      muonEtaRanges.push_back(make_pair(-2.4,2.4));
      WFinder muWFinder(fs, muonEtaRanges, 25*GeV, MUON, 
			40*GeV,1000*GeV,25*GeV,0.6,true,false,80.4,true);
      // Tag a W in the event, focus on jets that don't come from the W decay.
      addProjection(muWFinder,"muWFinder");
      FastJets JetProjection(muWFinder.remainingFinalState(),FastJets::ANTIKT, 0.6); //FastJets::KT,0.7
      addProjection(JetProjection,"Jets");

      ///////////////
      // Histograms
      ///////////////
      _histograms["JetMult"]		= bookHistogram1D("JetMult"		, 6, -0.5, 5.5);
      //Jet Kinematics
      _histograms["JetPt"]		= bookHistogram1D("JetPt"		, 50, 33, 300);
      _histograms["JetE"]		= bookHistogram1D("JetE"		, 25, 20, 300);
      _histograms["JetEta"]		= bookHistogram1D("JetEta"		, 25, -2, 2);
      _histograms["JetRapidity"]	= bookHistogram1D("JetRapidity"		, 25, -2, 2);
      //_histograms["JetPhi"]		= bookHistogram1D("JetPhi"		, 25, 0, TWOPI);
      _histograms["JetMass"]		= bookHistogram1D("JetMass"		, 100, 0, 40);
      //_histograms["Jet2Mass"]		= bookHistogram1D("Jet2Mass"		, 100, 0, 100);
      //_histograms["Jet3Mass"]		= bookHistogram1D("Jet3Mass"		, 100, 0, 100);
      _histograms["SubJetMult"]		= bookHistogram1D("SubJetMult"		, 15, -0.5, 29.5);
      _histograms["SubJet2Mass"]	= bookHistogram1D("SubJet2Mass"		, 100, 0, 35);
      _histograms["SubJet3Mass"]	= bookHistogram1D("SubJet3Mass"		, 100, 0, 45);
      _histograms["SubJetDeltaR"]	= bookHistogram1D("SubJetDeltaR"	, 50, 0, 1.0);
      _histograms["SubJetMass"]		= bookHistogram1D("SubJetMass"		, 100, 0, 12);
      _histograms["SubJetSumEt"]	= bookHistogram1D("SubJetSumEt"		, 30, 0, 175);

      //Jet Charge Histos
      _histograms["WJetCharge"]		= bookHistogram1D("WJetCharge"		, 50, -0.3, 0.3);
      _histograms["WCharge"]		= bookHistogram1D("WCharge"		, 3, -1.5, 1.5);
      _histograms["JetPullTheta"]       = bookHistogram1D("JetPullTheta"	,50,-PI,PI);
      _histograms["JetPullMag"]         = bookHistogram1D("JetPullMag"          ,50,0,0.04);
      _hist2DJetChargeWPt		= bookHistogram2D("JetChargeVsWPt"	,50,-0.3,0.3,50,24,300);

      //N-subjettiness histos	
      _histograms["JetMassFilt"]	= bookHistogram1D("JetMassFilt"		, 60, 0, 50);
      _histograms["JetMassTrim"]	= bookHistogram1D("JetMassTrim"		, 60, 0, 50);
      _histograms["JetMassPrune"]	= bookHistogram1D("JetMassPrune"	, 60, 0, 20);
      _histograms["NSubJettiness"]	= bookHistogram1D("NSubJettiness"	, 40, -0.005, 1.005);
      _histograms["NSubJettiness1Iter"]	= bookHistogram1D("NSubJettiness1Iter"	, 40, -0.005, 1.005);
      _histograms["NSubJettiness2Iter"]	= bookHistogram1D("NSubJettiness2Iter"	, 40, -0.005, 1.005);
    }
    /// quickly calculate standard deviation of pt distribution in jets
    virtual void pt_stddev(const PseudoJets& jets, double& mean,double& stddev,const double N) {
      foreach(const fastjet::PseudoJet& jet, jets)
	mean+=jet.pt();
      mean=mean/N;
      foreach(const fastjet::PseudoJet& jet, jets)
	stddev+=((jet.pt()-mean)*(jet.pt()-mean));
      stddev=stddev/N;
    }

    virtual void analyzeSubJets(const fastjet::PseudoJet& jet,const double weight) {
      const double ptmin=0.5*GeV;
      double sumEt=0.0;
      fastjet::ClusterSequence clusterSeq(jet.validated_cs()->constituents(jet),fastjet::JetDefinition(fastjet::kt_algorithm,0.6)); 

      PseudoJets subJets=clusterSeq.exclusive_jets_up_to(3);
 
      fastjet::ClusterSequence antiKTClusterSeq(jet.validated_cs()->constituents(jet),fastjet::JetDefinition(fastjet::antikt_algorithm,0.1));
      PseudoJets smallSubJets=antiKTClusterSeq.inclusive_jets(ptmin);
      int smallJetMult = smallSubJets.size();
      _histograms["SubJetMult"]->fill(smallJetMult,weight);
      unsigned int nSubJets=subJets.size();

      if(nSubJets==3)
	_histograms["SubJet3Mass"]->fill((subJets.at(0)+subJets.at(1)+subJets.at(2)).m(),weight);
      
      for(unsigned int j=0;j!=nSubJets;++j) {
	sumEt+=subJets.at(j).Et();
	_histograms["SubJetMass"]->fill(subJets.at(j).m());
	for(unsigned int k=(j+1); k!=nSubJets;++k) {
	  _histograms["SubJetDeltaR"]->fill(subJets.at(j).delta_R(subJets.at(k)),weight);
	  _histograms["SubJet2Mass"]->fill((subJets.at(j)+subJets.at(k)).m(),weight);
	}
      }
      _histograms["SubJetSumEt"]->fill(sumEt,weight);
    }
    /// Perform the per-event analysis
    void analyze(const Event& event) {
      _nPassing[0]++;
      const WFinder& muWFinder = applyProjection<WFinder>(event,"muWFinder");
      if (muWFinder.bosons().size() != 1)
	vetoEvent;
      _nPassing[1]++;

      const double weight = event.weight();
      const FastJets& JetProjection = applyProjection<FastJets>(event, "Jets");
      const PseudoJets& jets = JetProjection.pseudoJetsByPt(35.0*GeV);

      if (jets.size() > 0) {
	_nPassing[2]++;
	const unsigned int jetMult=jets.size();
	_histograms["JetMult"]->fill(jetMult);
	/// Rather than loop over all jets, just take the first hard
	/// one, Make sure entire jet is within fiducial volume
	if(jets.front().eta() > -(2.5-0.6) && jets.front().eta() < (2.5-0.6)) {
	  if(jets.front().has_valid_cs())
	    analyzeSubJets(jets.front(),weight);

	  foreach (const fastjet::PseudoJet& jet, jets) {
	    _histograms["JetMassFilt"]->fill(JetProjection.Filter(jet, FastJets::CAM, 3, 0.3).m(), weight);
	    _histograms["JetMassTrim"]->fill(JetProjection.Trimmer(jet, FastJets::CAM, 0.03, 0.3).m(), weight);
	    _histograms["JetMassPrune"]->fill(JetProjection.Pruner(jet, FastJets::CAM, 0.4, 0.1).m(), weight);
	    PseudoJets constituents = jet.constituents();
	    if (constituents.size() > 10) {
	      //todo slurp into one function call
		      
	      PseudoJets axes(JetProjection.GetAxes(2, constituents, FastJets::CAM, 0.5));
	      double tau = JetProjection.TauValue(2, 1, constituents, axes);
	      _histograms["NSubJettiness"]->fill(tau, weight);
	      JetProjection.UpdateAxes(2, constituents, axes);
	      double tau1 = JetProjection.TauValue(2, 1, constituents, axes);
	      JetProjection.UpdateAxes(2, constituents, axes);
	      double tau2 = JetProjection.TauValue(2, 1, constituents, axes);
	      _histograms["NSubJettiness1Iter"]->fill(tau1, weight);
	      _histograms["NSubJettiness2Iter"]->fill(tau2, weight);
	    }
	  }
	  _nPassing[3]++;
	  _histograms["JetMass"]->fill(jets.front().m(),weight);
	  _histograms["JetPt"]->fill(jets.front().pt(),weight);	
	  _histograms["JetE"]->fill(jets.front().E(),weight);
	  _histograms["JetEta"]->fill(jets.front().eta(),weight);	
	  _histograms["JetRapidity"]->fill(jets.front().rapidity(),weight); 
	  //histograms["JetPhi"]->fill(jets.front().phi(),weight);	
	  const double wCharge=PID::charge(muWFinder.bosons().front().pdgId())+0.0;//dirty cast 
	  const double jetCharge=wCharge*JetProjection.JetCharge(jets.front(),0.5,1*GeV);
	  const std::pair<double,double> tvec=JetProjection.JetPull(jets.front());
	  //std::cout<<"mag: "<<tvec.first<<"theta: "<<tvec.second<<endl;
	  _hist2DJetChargeWPt->fill(jetCharge,muWFinder.bosons().front().momentum().pT(),weight);
	  _histograms["WJetCharge"]->fill(jetCharge,weight);
	  _histograms["WCharge"]->fill(wCharge,weight);
	  _histograms["JetPullTheta"]->fill(tvec.first,weight);
	  _histograms["JetPullMag"]->fill(tvec.second,weight);
	}	      
      }
      else {
	vetoEvent;
      }
    }
    /// Finalize
    void finalize() {
      cout<<"Cut summary: "<<endl;
      cout<<"| Inclusive | "<<_nPassing[0]<< " | "<<endl;
      cout<<"| Found W   | "<<_nPassing[1]<< " | "<<endl;
      cout<<"| >1 Jet    | "<<_nPassing[2]<< " | "<<endl;
      cout<<"| Fiducial  | "<<_nPassing[3]<< " | "<<endl;
      cout<<"Mean Jet Charge: "<<_histograms["WJetCharge"]->mean()<<" +/- "<<_histograms["WJetCharge"]->rms()<<endl;

      foreach(BookedHistos::value_type H,_histograms)
	normalize(H.second);
      normalize(_hist2DJetChargeWPt);
    }
    //@}
  private:
    ///@param _histograms Indexed by histogram name for easy management
    ///until Rivet Autobooking takes over, allows any number of
    ///histograms to be added "on the fly" in the init() method.
    //@{
    BookedHistos _histograms;
    AIDA::IHistogram2D *_hist2DJetChargeWPt;
    //@}
    /// @param _nPassing Event count for efficiency study
    //@{
    int _nPassing[4];
    //@}
  };
  // The hook for the plugin system
  DECLARE_RIVET_PLUGIN(MC_GENSTUDY_JETCHARGE);
}
