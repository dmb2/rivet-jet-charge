// -*- C++ -*-
//System includes
//#include <map>
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

namespace Rivet {

  /// Generic analysis looking at various distributions of final state particles
  class MC_GENSTUDY_JETCHARGE : public Analysis {
  public:

    /// Constructor
    MC_GENSTUDY_JETCHARGE()
      : Analysis("MC_GENSTUDY_JETCHARGE")
    { for(unsigned int i=0; i < 4; i++) _nPassing[i]=0;    }


  public:

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

      // Histograms

      _histJetMult		= bookHistogram1D("JetMult", 6, -0.5, 5.5);
      //Jet Kinematics
      _histJetPt		= bookHistogram1D("JetPt"	, 50, 33, 300);
      _histJetE			= bookHistogram1D("JetE"	, 25, 20, 300);
      _histJetEta		= bookHistogram1D("JetEta"	, 25, -2, 2);
      _histJetRapidity		= bookHistogram1D("JetRapidity"	, 25, -2, 2);
      //_histJetPhi		= bookHistogram1D("JetPhi"	, 25, 0, TWOPI);
      _histJetMass		= bookHistogram1D("JetMass"	, 100, 0, 40);
      //_histJet2Mass		= bookHistogram1D("Jet2Mass"	, 100, 0, 100);
      //_histJet3Mass		= bookHistogram1D("Jet3Mass"	, 100, 0, 100);
      _histSubJetMult		= bookHistogram1D("SubJetMult", 15, -0.5, 29.5);
      /*
	_histSubJetSkew		= bookHistogram1D("SubJetSkew", 25, 0, 0.1); 
	_histSubJetStddev	= bookHistogram1D("SubJetStddev", 25, 0, 0.2); 
      */
      _histSubJet2Mass		= bookHistogram1D("SubJet2Mass"	, 100, 0, 35);
      _histSubJet3Mass		= bookHistogram1D("SubJet3Mass"	, 100, 0, 45);
      _histSubJetDeltaR		= bookHistogram1D("SubJetDeltaR", 50, 0, 1.0);
      _histSubJetMass		= bookHistogram1D("SubJetMass"	, 100, 0, 12);
      _histSubJetSumEt		= bookHistogram1D("SubJetSumEt", 30, 0, 175);

      //Jet Charge Histos
      _histWJetCharge		= bookHistogram1D("WJetCharge", 50, -0.3, 0.3);
      _histWCharge		= bookHistogram1D("WCharge", 3, -1.5, 1.5);
      _hist2DJetChargeWPt       = bookHistogram2D("JetChargeVsWPt",50,-0.3,0.3,50,24,300);
      _histJetPullTy             = bookHistogram1D("JetPullThetaTy",50,0,0.04);

      //N-subjettiness histos	
      
      _histJetMassFilt		= bookHistogram1D("JetMassFilt", 60, 0, 50);
      _histJetMassTrim		= bookHistogram1D("JetMassTrim", 60, 0, 50);
      _histJetMassPrune		= bookHistogram1D("JetMassPrune", 60, 0, 20);
      _histNSubJettiness	= bookHistogram1D("NSubJettiness", 40, -0.005, 1.005);
      _histNSubJettiness1Iter	= bookHistogram1D("NSubJettiness1Iter", 40, -0.005, 1.005);
      _histNSubJettiness2Iter	= bookHistogram1D("NSubJettiness2Iter", 40, -0.005, 1.005);

    }
    /// quickly calculate standard deviation of pt distribution in jets
    virtual void pt_stddev(const PseudoJets& jets, double& mean,double& stddev,const double N)
    {
      foreach(const fastjet::PseudoJet& jet, jets)
	  mean+=jet.pt();
      mean=mean/N;
      
      foreach(const fastjet::PseudoJet& jet, jets)
	  stddev+=((jet.pt()-mean)*(jet.pt()-mean));
      stddev=stddev/N;
    }

    virtual void analyzeSubJets(const fastjet::PseudoJet& jet,const double weight)
    {
      //double dcut=0.4;
      //double ptstddev	= 0.0;
      //double ptmean	= 0.0;
      //double ptskew	= 0.0;
      double ptmin=0.5*GeV;
      double sumEt=0.0;
      fastjet::ClusterSequence clusterSeq(jet.validated_cs()->constituents(jet),fastjet::JetDefinition(fastjet::kt_algorithm,0.6)); 

      PseudoJets subJets=clusterSeq.exclusive_jets_up_to(3);
 
      fastjet::ClusterSequence antiKTClusterSeq(jet.validated_cs()->constituents(jet),fastjet::JetDefinition(fastjet::antikt_algorithm,0.1));
      PseudoJets smallSubJets=antiKTClusterSeq.inclusive_jets(ptmin);
      int smallJetMult = smallSubJets.size();
      _histSubJetMult->fill(smallJetMult,weight);
      /*
      if(smallJetMult > 1)
	{
	  pt_stddev(smallSubJets,ptmean,ptstddev,static_cast<double>(smallJetMult));

	  foreach(const fastjet::PseudoJet& jeta, smallSubJets)
	    ptskew+=pow(jeta.pt()-ptmean,3);
	  ptskew=(ptskew/(static_cast<double>(smallJetMult)))/(pow(ptstddev,1.5)*jet.pt());
	  ptstddev=ptstddev/jet.pt();
	}
      _histSubJetSkew->fill(ptskew,weight);
      _histSubJetStddev->fill(ptskew,weight);
      */
      unsigned int nSubJets=subJets.size();

      if(nSubJets==3)
	_histSubJet3Mass->fill((subJets.at(0)+subJets.at(1)+subJets.at(2)).m(),weight);
      
      for(unsigned int j=0;j!=nSubJets;++j)
	{
	  sumEt+=subJets.at(j).Et();
	  _histSubJetMass->fill(subJets.at(j).m());
	  for(unsigned int k=(j+1); k!=nSubJets;++k)
	    {
	      _histSubJetDeltaR->fill(subJets.at(j).delta_R(subJets.at(k)),weight);
	      _histSubJet2Mass->fill((subJets.at(j)+subJets.at(k)).m(),weight);
	    }
	}
      _histSubJetSumEt->fill(sumEt,weight);

    }
    /// Perform the per-event analysis
    void analyze(const Event& event) 
    {
      _nPassing[0]++;
      const WFinder& muWFinder = applyProjection<WFinder>(event,"muWFinder");
      if (muWFinder.bosons().size() != 1)
	vetoEvent;
      _nPassing[1]++;

      const double weight = event.weight();
      const FastJets& JetProjection = applyProjection<FastJets>(event, "Jets");
      const PseudoJets& jets = JetProjection.pseudoJetsByPt(35.0*GeV);

      if (jets.size() > 0) 
	{
	  _nPassing[2]++;
	  unsigned int jetMult=jets.size();
	  _histJetMult->fill(jetMult);
	  /// Rather than loop over all jets, just take the first hard
	  /// one, Make sure entire jet is within fiducial volume
	  if(jets.front().eta() > -(2.5-0.6) && jets.front().eta() < (2.5-0.6))
	    {
	      if(jets.front().has_valid_cs())
		analyzeSubJets(jets.front(),weight);

	      foreach (const fastjet::PseudoJet& jet, jets) 
		{
		  _histJetMassFilt->fill(JetProjection.Filter(jet, FastJets::CAM, 3, 0.3).m(), weight);
		  _histJetMassTrim->fill(JetProjection.Trimmer(jet, FastJets::CAM, 0.03, 0.3).m(), weight);
		  _histJetMassPrune->fill(JetProjection.Pruner(jet, FastJets::CAM, 0.4, 0.1).m(), weight);
		  PseudoJets constituents = jet.constituents();
		  if (constituents.size() > 10) 
		    {
		      //todo slurp into one function call
		      
		      PseudoJets axes(JetProjection.GetAxes(2, constituents, FastJets::CAM, 0.5));
		      double tau = JetProjection.TauValue(2, 1, constituents, axes);
		      _histNSubJettiness->fill(tau, weight);
		      JetProjection.UpdateAxes(2, constituents, axes);
		      double tau1 = JetProjection.TauValue(2, 1, constituents, axes);
		      JetProjection.UpdateAxes(2, constituents, axes);
		      double tau2 = JetProjection.TauValue(2, 1, constituents, axes);
		      _histNSubJettiness1Iter->fill(tau1, weight);
		      _histNSubJettiness2Iter->fill(tau2, weight);
		    }
		}
	      _nPassing[3]++;
	      _histJetMass->fill(jets.front().m(),weight);
	      _histJetPt->fill(jets.front().pt(),weight);	
	      _histJetE->fill(jets.front().E(),weight);
	      _histJetEta->fill(jets.front().eta(),weight);	
	      _histJetRapidity->fill(jets.front().rapidity(),weight); 
	      //histJetPhi->fill(jets.front().phi(),weight);	
	      double wCharge=PID::charge(muWFinder.bosons().front().pdgId())+0.0;//dirty cast 
	      double jetCharge=wCharge*JetProjection.JetCharge(jets.front(),0.5,1*GeV);
	      std::pair<double,double> tvec=JetProjection.JetPull(jets.front());
	      //std::cout<<"mag: "<<tvec.first<<"theta: "<<tvec.second<<endl;
	      _hist2DJetChargeWPt->fill(jetCharge,muWFinder.bosons().front().momentum().pT(),weight);
	      _histWJetCharge->fill(jetCharge,weight);
	      _histWCharge->fill(wCharge,weight);
	      _histJetPullTy->fill(tvec.first*cos(tvec.second),weight);
	    }	      

	}
      else
	vetoEvent;

    }
    /// Finalize
    void finalize() 
    {
      /*
      //Why oh why can't I use a std::map<AIDA::IHistogram1D*>
      foreach(BookedHistos::value_type H,Histograms)
	scale(H.second,1/sumOfWeights());
      */
      cout<<"Cut summary: "<<endl;
      cout<<"| Inclusive | "<<_nPassing[0]<< " | "<<endl;
      cout<<"| Found W   | "<<_nPassing[1]<< " | "<<endl;
      cout<<"| >1 Jet    | "<<_nPassing[2]<< " | "<<endl;
      cout<<"| Fiducial  | "<<_nPassing[3]<< " | "<<endl;
      
      cout<<"Mean Jet Charge: "<<_histWJetCharge->mean()<<" +/- "<<_histWJetCharge->rms()<<endl;
      
      normalize(_histJetMult);
      normalize(_histJetPt);
      normalize(_histJetE);
      normalize(_histJetEta);
      normalize(_histJetRapidity);
      //normalize(_histJetPhi);
      normalize(_histJetMass);

      //normalize(_histJet2Mass);
      //normalize(_histJet3Mass );

      normalize(_histSubJet2Mass);
      normalize(_histSubJet3Mass);
      normalize(_histSubJetDeltaR);
      normalize(_histSubJetMass);
      normalize(_histSubJetSumEt);
      //normalize(_histSubJetSkew);
      //normalize(_histSubJetStddev);
      //normalize(_histJetDipolarity,1.0);
      normalize(_histWJetCharge);
      normalize(_histWCharge);
      normalize(_hist2DJetChargeWPt);
      normalize(_histJetPullTy);
      normalize(_histSubJetMult);
      normalize(_histJetMassFilt);
      normalize(_histJetMassTrim);
      normalize(_histJetMassPrune);
      normalize(_histNSubJettiness);
      normalize(_histNSubJettiness1Iter);
      normalize(_histNSubJettiness2Iter);

    }
    //@}
  private:

    ///@param Histograms Indexed by histogram name for easy management
    ///until Rivet Autobooking takes over, allows any number of
    ///histograms to be added "on the fly" in the init() method.
    //@{
    AIDA::IHistogram1D *_histJetMult;

    //Jet Kinematics	
    AIDA::IHistogram1D *_histJetPt;
    AIDA::IHistogram1D *_histJetE;
    AIDA::IHistogram1D *_histJetEta;
    AIDA::IHistogram1D *_histJetRapidity;
    //AIDA::IHistogram1D *_histJetPhi;
    AIDA::IHistogram1D *_histJetMass;

    // Jet theoreticals
    //AIDA::IHistogram1D *_histJet2Mass;
    //AIDA::IHistogram1D *_histJet3Mass;

    //AIDA::IHistogram1D *_histSubJetSkew;
    //AIDA::IHistogram1D *_histSubJetStddev;

    AIDA::IHistogram1D *_histSubJet2Mass;
    AIDA::IHistogram1D *_histSubJet3Mass;
    AIDA::IHistogram1D *_histSubJetDeltaR;
    AIDA::IHistogram1D *_histSubJetMass;
    AIDA::IHistogram1D *_histSubJetSumEt;

    //Jet Charge
    AIDA::IHistogram1D *_histWJetCharge;    
    AIDA::IHistogram1D *_histWCharge;
    AIDA::IHistogram1D *_histSubJetMult;
    AIDA::IHistogram2D *_hist2DJetChargeWPt;
    AIDA::IHistogram1D *_histJetPullTy;
      //N-subjettiness histos	
    AIDA::IHistogram1D *_histJetMassFilt;
    AIDA::IHistogram1D *_histJetMassTrim;	
    AIDA::IHistogram1D *_histJetMassPrune;		
    AIDA::IHistogram1D *_histNSubJettiness;	
    AIDA::IHistogram1D *_histNSubJettiness1Iter;	
    AIDA::IHistogram1D *_histNSubJettiness2Iter;	
    //@}
    // Event count for efficiency study
    int _nPassing[4];
  };


  // The hook for the plugin system
  DECLARE_RIVET_PLUGIN(MC_GENSTUDY_JETCHARGE);

}
/// Garbage bin
/*
 */
