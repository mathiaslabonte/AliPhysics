/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

#include "AliAnalysisTaskPtCorr.h"
#include "AliEventCuts.h"
#include "AliAnalysisManager.h"
#include "AliInputEventHandler.h"
#include "TList.h"
#include "TProfile.h"
#include "AliMultSelection.h"
#include "AliMCEventHandler.h"
#include "AliMCEvent.h"
#include "AliMCParticle.h"
#include "AliAODMCParticle.h"
#include "AliGenEventHeader.h"
#include "AliCollisionGeometry.h"
#include "AliGenHijingEventHeader.h"
#include "AliGenHepMCEventHeader.h"

using namespace std;
using namespace TMath;

ClassImp(AliAnalysisTaskPtCorr)

AliAnalysisTaskPtCorr::AliAnalysisTaskPtCorr() : AliAnalysisTaskSE(),
    fEventCuts(),
    fCentEst(0),
    fRunNo(0),
    fSystFlag(0),
    fContSubfix(0),
    fIsMC(kFALSE),
    fMCEvent(0),
    fCorrList(0),
    fQAList(0),
    fEfficiencyList(0),
    fEfficiencies(0),
    fPowerEfficiencies(0),
    fWeightSubfix(""),
    fGFWSelection(0),
    fGFWnTrackSelection(0),
    fV0MAxis(0),
    fMultiAxis(0),
    fMultiBins(0), 
    fNMultiBins(0),
    fPtAxis(0), 
    fPtBins(0), 
    fNPtBins(0),
    fEta(0.8),
    fEtaNch(0.8),
    fEtaGap(-1),
    fPUcut(1500),
    fRndm(0),
    fNbootstrap(10),
    fUseWeightsOne(false),
    fUseRecNchForMC(false),
    fPileupOff(false),
    fUseNch(false),
    fUsePowerEff(false),
    fPseudoEff(0),
    fUseEff(0),
    fConstEff(0.8),
    fSigmaEff(0.05),
    mpar(6),
    wp(0),
    wpP(0),
    wpN(0),
    fEventWeight(PtSpace::kWperms),
    fV0MMulti(0),
    fpt(0),
    fNchTrueVsRec(0),
    fV0MvsMult(0),
    fPtMoms(0),
    fPtDist(0),
    fPtDCA(0),
    fPtVsNTrk(0),
    fTriggerType(AliVEvent::kMB+AliVEvent::kINT7),
    fOnTheFly(false),
    fImpactParameter(0)
{};
//_____________________________________________________________________________
AliAnalysisTaskPtCorr::AliAnalysisTaskPtCorr(const char *name, bool IsMC, bool isOnTheFly, int pseudoeff, TString ContSubfix) : AliAnalysisTaskSE(name),
    fEventCuts(),
    fCentEst(0),
    fRunNo(0),
    fSystFlag(0),
    fContSubfix(0),
    fIsMC(IsMC),
    fMCEvent(0),
    fCorrList(0),
    fQAList(0),
    fEfficiencyList(0),
    fEfficiencies(0),
    fPowerEfficiencies(0),
    fWeightSubfix(""),
    fGFWSelection(0),
    fGFWnTrackSelection(0),
    fV0MAxis(0),
    fMultiAxis(0),
    fMultiBins(0), 
    fNMultiBins(0),
    fPtAxis(0), 
    fPtBins(0), 
    fNPtBins(0),
    fEta(0.8),
    fEtaNch(0.8),
    fEtaGap(-1),
    fPUcut(1500),
    fRndm(0),
    fNbootstrap(10),
    fUseWeightsOne(false),
    fUseRecNchForMC(false),
    fPileupOff(false),
    fUseNch(false),
    fUsePowerEff(false),
    fPseudoEff(pseudoeff),
    fUseEff(0),
    fConstEff(0.8),
    fSigmaEff(0.05),
    mpar(6),
    wp(0),
    wpP(0),
    wpN(0),
    fEventWeight(PtSpace::kWperms),
    fV0MMulti(0),
    fpt(0),
    fNchTrueVsRec(0),
    fV0MvsMult(0),
    fPtMoms(0),
    fPtDist(0),
    fPtDCA(0),
    fPtVsNTrk(0),
    fTriggerType(AliVEvent::kMB+AliVEvent::kINT7),
    fOnTheFly(isOnTheFly),
    fImpactParameter(0)
{ 
    SetContSubfix(ContSubfix);
    fCentEst = new TString("V0M");
    if(!(fIsMC && !(fPseudoEff>0)) && !fOnTheFly)
    {
        DefineInput(1, TList::Class());
    }
    if(fOnTheFly)
    {
      vector<double> b = {0.0,3.72,5.23,7.31,8.88,10.20,11.38,12.47,13.50,14.51,100.0};
      vector<double> cent = {0.0,5.0,10.0,20.0,30.0,40.0,50.0,60.0,70.0,80.0,100.0};
      for(size_t i(0); i<b.size(); ++i) centralitymap[b[i]]=cent[i];
    }
    DefineOutput(1, TList::Class());
    DefineOutput(2,TList::Class());
};
//_____________________________________________________________________________
AliAnalysisTaskPtCorr::~AliAnalysisTaskPtCorr()
{
    if(fCorrList) delete fCorrList;
};
void AliAnalysisTaskPtCorr::NotifyRun() {
    if(fOnTheFly) return;
    Bool_t dummy = fEventCuts.AcceptEvent(InputEvent());
    if(!fPileupOff) fEventCuts.SetRejectTPCPileupWithITSTPCnCluCorr(kTRUE);

    //Then override PU cut if required:
    if(fGFWSelection->GetSystFlagIndex()==22 && !fPileupOff)
      fEventCuts.fESDvsTPConlyLinearCut[0] = fPUcut;
}
void AliAnalysisTaskPtCorr::UserCreateOutputObjects()
{
    if(!fGFWSelection) SetSystFlag(0);
    fGFWSelection->PrintSetup();
    fSystFlag = fGFWSelection->GetSystFlagIndex();
    if(fGFWSelection->GetSystFlagIndex() == 20) SetCentralityEstimator("CL0");
    else if(fGFWSelection->GetSystFlagIndex() == 21) SetCentralityEstimator("CL1");
    OpenFile(1);
    const int temp_NV0MBinsDefault = 10;
    double temp_V0MBinsDefault[11] = {0,5,10,20,30,40,50,60,70,80,90}; //Last bin to include V0M beyond anchor point
    if(!fV0MAxis) SetV0MBins(temp_NV0MBinsDefault,temp_V0MBinsDefault);
    double *l_V0MBinsDefault=GetBinsFromAxis(fV0MAxis);
    int l_NV0MBinsDefault=fV0MAxis->GetNbins();
    if(!fMultiAxis)
    {
      printf("Multiplicity axis not set. Using defaults bins\n"); 
      SetMultiplicityBins(l_NV0MBinsDefault,l_V0MBinsDefault);
    }
    fMultiBins = GetBinsFromAxis(fMultiAxis);
    fNMultiBins = fMultiAxis->GetNbins();
    const int l_NPtBinsDefault = 14;
    Double_t l_PtBinsDefault[l_NPtBinsDefault+1] = {0.2,0.4,0.6,0.8,1.0,1.2,1.4,1.6,1.8,2.0,2.2,2.4,2.6,2.8,3.0};   
    if(!fPtAxis) SetPtBins(l_NPtBinsDefault,l_PtBinsDefault);
    fPtBins = GetBinsFromAxis(fPtAxis);
    fNPtBins = fPtAxis->GetNbins();
    fV0MMulti = new TH1D("V0M_Multi","V0M_Multi",l_NV0MBinsDefault,l_V0MBinsDefault);
    fRndm = new TRandom(0);
    if(!(fIsMC && !(fPseudoEff>0)) && !fOnTheFly)
    { 
        fEfficiencyList = (TList*)GetInputData(1);
        fEfficiencies = new TH1D*[l_NV0MBinsDefault];
        fPowerEfficiencies = new TH2D*[l_NV0MBinsDefault];
        for(int i=0;i<l_NV0MBinsDefault;i++) {
          if(fUsePowerEff)
          {
            fPowerEfficiencies[i] = (TH2D*)fEfficiencyList->FindObject(Form("Eff_Cent%i%s",i,fGFWSelection->GetSystPF()));
            if(!fPowerEfficiencies[i])
            {
              if(!i) AliFatal("Could not fetch efficiency!\n");
              printf("Could not find efficiency for V0M bin no. %i! Cloning the previous efficiency instead...\n",i);
              fPowerEfficiencies[i] = (TH2D*)fPowerEfficiencies[i-1]->Clone(Form("Eff_Cent%i%s",i,fGFWSelection->GetSystPF()));
            }
          }
          else
          {
            fEfficiencies[i] = (TH1D*)fEfficiencyList->FindObject(Form("EffRescaled_Cent%i%s",i,fGFWSelection->GetSystPF()));
            if(!fEfficiencies[i]) {
            if(!i) AliFatal("Could not fetch efficiency!\n");
            printf("Could not find efficiency for V0M bin no. %i! Cloning the previous efficiency instead...\n",i);
            fEfficiencies[i] = (TH1D*)fEfficiencies[i-1]->Clone(Form("EffRescaled_Cent%i%s",i,fGFWSelection->GetSystPF()));
            };
          }
        }
    };
    
    fCorrList = new TList(); fCorrList->SetOwner(1);
    fpt = new AliPtContainer("ptcont","ptcont",fNMultiBins,fMultiBins,mpar,fEtaGap>=0);
    fCorrList->Add(fpt);
    if(fNbootstrap) {
      fpt->InitializeSubsamples(fNbootstrap);
    }
    fCorrList->Add(fV0MMulti);
    if(fIsMC) {
      fNchTrueVsRec = new TH2D("NchTrueVsRec",";Nch (MC-true); Nch (MC-reco)",fNMultiBins,fMultiBins,fNMultiBins,fMultiBins);
      fCorrList->Add(fNchTrueVsRec);
    }
    fV0MvsMult = new TH2D("MultVsV0M","MultVsV0M",103,0,103,fNMultiBins,fMultiBins[0],fMultiBins[fNMultiBins]);
    fCorrList->Add(fV0MvsMult);
    PostData(1,fCorrList);
    fQAList = new TList();
    fQAList->SetOwner(1);
    fEventCuts.AddQAplotsToList(fQAList,kTRUE);
    double powers[7] = {0,1,2,3,4,5,6};
    int Npows = 6;
    fPtMoms = new TH3D("ptMoments","ptMoments",fNPtBins,fPtBins,Npows,powers,temp_NV0MBinsDefault,l_V0MBinsDefault);
    fQAList->Add(fPtMoms);
    fPtDist = new TH2D("ptDist","ptDist;p_t;centrality",fNPtBins,fPtBins,temp_NV0MBinsDefault,l_V0MBinsDefault);
    fQAList->Add(fPtDist);
    double binsDCA[61] = {-3.00, -2.90, -2.80, -2.70, -2.60, -2.50, -2.40, -2.30, -2.20, -2.10, -2.00, -1.90, -1.80, -1.70, -1.60, -1.50, -1.40, -1.30, -1.20, -1.10, -1.00, -0.90, -0.80, -0.70, -0.60, -0.50, -0.40, -0.30, -0.20, -0.10, 0.00, 0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.90, 1.00, 1.10, 1.20, 1.30, 1.40, 1.50, 1.60, 1.70, 1.80, 1.90, 2.00, 2.10, 2.20, 2.30, 2.40, 2.50, 2.60, 2.70, 2.80, 2.90, 3.00};
    int NbinsDCA = 60;
    fPtDCA = new TH3D("ptDCA","ptDCA;pt;dcaxy;dcaz",fNPtBins,fPtBins,NbinsDCA,binsDCA,NbinsDCA,binsDCA);
    fQAList->Add(fPtDCA);
    Int_t NNtrkBins=100;
    Double_t *binsNtrk = new Double_t[NNtrkBins+1];
    for(Int_t i=0;i<=NNtrkBins; i++) binsNtrk[i] = 30*i+0.5;
    fPtVsNTrk = new TH2D("NtracksPt",";NtracksPt;N_{trk};p_{T} (GeV/#it{c})",NNtrkBins,binsNtrk,fNPtBins,fPtBins);
    fQAList->Add(fPtVsNTrk);
    PostData(2,fQAList);
    fEventCuts.OverrideAutomaticTriggerSelection(fTriggerType,true);
    fGFWnTrackSelection = new AliGFWCuts();
    fGFWnTrackSelection->SetupCuts(0);
    fGFWnTrackSelection->SetEta(fEtaNch);
    printf("User output objects created!\n"); 
}
void AliAnalysisTaskPtCorr::UserExec(Option_t *)
{
    AliAODEvent *fAOD;
    AliVEvent* fEvent;
    if(fIsMC) {
        fMCEvent = dynamic_cast<AliMCEvent *>(MCEvent());
        if (!fMCEvent) return;
    }
    if(fOnTheFly) fMCEvent = getMCEvent();
    else 
    {
      fEvent = dynamic_cast<AliVEvent*>(InputEvent());
      fAOD = dynamic_cast<AliAODEvent*>(InputEvent());
      if(!fAOD) { return; }
      if(!fEvent) { printf("Event not found!\n"); return; }
    }
    double l_cent = getCentrality();
    if(l_cent<0) return;
    if(!fOnTheFly && !CheckTrigger(l_cent)) return;
    double vtxXYZ[] = {0.,0.,0.};
    if(!fOnTheFly && !AcceptAODEvent(fAOD, vtxXYZ)) return;
    if(!fOnTheFly && !fGFWSelection->AcceptVertex(fAOD)) return;
    (fOnTheFly)?FillPtCorr(fMCEvent,l_cent,vtxXYZ):FillPtCorr(fAOD,l_cent,vtxXYZ);
}
bool AliAnalysisTaskPtCorr::CheckTrigger(Double_t lCent) {
  unsigned int fSelMask = ((AliInputEventHandler*)(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler()))->IsEventSelected();
  if(!(fTriggerType&fSelMask)) { return kFALSE; }; //printf("Returning from the generic check\n");
  if(fSelMask&(fTriggerType&(AliVEvent::kINT7+AliVEvent::kMB))) {return kTRUE; }; //printf("Passed by MB trigger!\n");
  if((fSelMask&fTriggerType&AliVEvent::kCentral) && lCent>10) {return kFALSE; }; //printf("Returnning from kCent case\n");
  if((fSelMask&fTriggerType&AliVEvent::kSemiCentral) && (lCent<30 || lCent>50)) {return kFALSE; }; //printf("Returning from kSC case\n");
  return kTRUE;
};
double AliAnalysisTaskPtCorr::getCentrality()
{
  if(fOnTheFly)
  {
    vector<double> b = {0.0,3.72,5.23,7.31,8.88,10.20,11.38,12.47,13.50,14.51,100.0};
    vector<double>::iterator it = upper_bound(b.begin(),b.end(),fImpactParameter);
    double l_cent = (centralitymap[b[it-b.begin()]]+centralitymap[b[it-b.begin()-1]])/2.0;
    return l_cent;
  }
  else 
  {
    AliMultSelection *l_MultSel = (AliMultSelection*)fInputEvent->FindListObject("MultSelection");
    if(!l_MultSel) { printf("MultSelection not found\n"); return -1.0; }
    double l_cent = l_MultSel->GetMultiplicityPercentile(fCentEst->Data());
    return l_cent;
  }
  return -1.0;
}
void AliAnalysisTaskPtCorr::Terminate(Option_t *)
{

}
AliMCEvent *AliAnalysisTaskPtCorr::getMCEvent() {
  AliMCEvent* ev = dynamic_cast<AliMCEvent*>(MCEvent());
  if(!ev) { AliFatal("MC event not found!"); return 0; }
  AliGenEventHeader *header = dynamic_cast<AliGenEventHeader*>(ev->GenEventHeader());
  if(!header) { AliFatal("MC event not generated!"); return 0; }
  AliCollisionGeometry* headerH;
  TString genName;
  TList *ltgen = (TList*)ev->GetCocktailList();
  if (ltgen) {
  for(auto&& listObject: *ltgen){
    genName = Form("%s",listObject->GetName());
    if (genName.Contains("Hijing")) {
      headerH = dynamic_cast<AliCollisionGeometry*>(listObject);
      break;
      }
    }
  }
  else headerH = dynamic_cast<AliCollisionGeometry*>(ev->GenEventHeader());
  if(headerH){
      fImpactParameter = headerH->ImpactParameter();
  }
  return ev;
}
bool AliAnalysisTaskPtCorr::AcceptAODTrack(AliAODTrack *tr, double *ltrackXYZ, const double &ptMin, const double &ptMax, double *vtxp)
{
  if(tr->Pt()<ptMin) return kFALSE;
  if(tr->Pt()>ptMax) return kFALSE;
  if(ltrackXYZ && vtxp) {
    tr->GetXYZ(ltrackXYZ);
    ltrackXYZ[0] = ltrackXYZ[0]-vtxp[0];
    ltrackXYZ[1] = ltrackXYZ[1]-vtxp[1];
    ltrackXYZ[2] = ltrackXYZ[2]-vtxp[2];
  } else return kFALSE; //DCA cut is a must for now
  return fGFWSelection->AcceptTrack(tr,fSystFlag==1?0:ltrackXYZ,0,kFALSE);
};
bool AliAnalysisTaskPtCorr::AcceptAODTrack(AliAODTrack *mtr, double *ltrackXYZ, const double &ptMin, const double &ptMax, double *vtxp, int &nTot) 
{
  if(mtr->Pt()<ptMin) return kFALSE;
  if(mtr->Pt()>ptMax) return kFALSE;
  if(ltrackXYZ && vtxp) {
    mtr->GetXYZ(ltrackXYZ);
    ltrackXYZ[0] = ltrackXYZ[0]-vtxp[0];
    ltrackXYZ[1] = ltrackXYZ[1]-vtxp[1];
    ltrackXYZ[2] = ltrackXYZ[2]-vtxp[2];
  } else return kFALSE; 
  if(fGFWnTrackSelection->AcceptTrack(mtr,ltrackXYZ,0,kFALSE)) nTot++;
  return fGFWSelection->AcceptTrack(mtr,fSystFlag==1?0:ltrackXYZ,0,kFALSE); 
};
bool AliAnalysisTaskPtCorr::AcceptAODEvent(AliVEvent *ev, Double_t *inVtxXYZ)
{
    const AliAODVertex* vtx = dynamic_cast<const AliAODVertex*>(ev->GetPrimaryVertex());
    if(!vtx || vtx->GetNContributors() < 1)
    return kFALSE;
    const AliAODVertex* vtxSPD = dynamic_cast<const AliAODVertex*>(ev->GetPrimaryVertexSPD());
    Double_t dMaxResol = 0.25; // suggested from DPG
    Double_t cov[6] = {0};
    vtxSPD->GetCovarianceMatrix(cov);
    Double_t zRes = TMath::Sqrt(cov[5]);
    if ( vtxSPD->IsFromVertexerZ() && (zRes > dMaxResol)) return kFALSE;
    const Double_t aodVtxZ = vtx->GetZ();
    if(TMath::Abs(aodVtxZ) > 10)
    return kFALSE;
    vtx->GetXYZ(inVtxXYZ);
    return kTRUE;
};
void AliAnalysisTaskPtCorr::FillWPCounter(vector<vector<double>> &inarr, double w, double p)
{
  for(int i=0;i<=mpar;++i)
  {
    for(int j=0;j<=mpar;++j)
    {
      
      inarr[i][j] += pow(w,i)*pow(p,j);
    }
  }
  return;
}
void AliAnalysisTaskPtCorr::FillWPCounter(vector<vector<double>> &inarr, vector<double> w, double p)
{
  for(int i=0;i<=mpar;++i)
  {
    for(int j=0;j<=mpar;++j)
    {
      double ww = (i==j)?w[j]:pow(w[1],i);
      inarr[i][j] += ww*pow(p,j);
    }
  }
  return;
}
void AliAnalysisTaskPtCorr::FillPtCorr(AliVEvent* ev, const double &l_cent, double *vtxXYZ)
{          
    wp.resize(10,vector<double>(10));
    wpP.resize(10,vector<double>(10));
    wpN.resize(10,vector<double>(10));
    AliAODTrack *track;
    double trackXYZ[3];
    double ptMin = fPtBins[0];
    double ptMax = fPtBins[fNPtBins];
    int iCent = fV0MMulti->FindBin(l_cent);
    if(!iCent || iCent>fV0MMulti->GetNbinsX()) return;
    iCent--;
    int nTracks=0;
    if(fOnTheFly)
    {
      int NTracks = fMCEvent->GetNumberOfPrimaries();
      if(NTracks < 1) { return; }
      for(Int_t iTrack(0); iTrack < NTracks; iTrack++) 
      {
          AliMCParticle* track = dynamic_cast<AliMCParticle*>(fMCEvent->GetTrack(iTrack));
          if(!track) { continue; }
          double l_eta = track->Eta();          
          if (TMath::Abs(l_eta) > fEta) continue;
          double l_pt = track->Pt();
          if (l_pt<ptMin || l_pt>ptMax) continue;
          if(fEtaGap >= 0 && l_eta > fEtaGap) FillWPCounter(wpP,1,l_pt);
          if(fEtaGap >= 0 && l_eta < -fEtaGap) FillWPCounter(wpN,1,l_pt);
          FillWPCounter(wp,1,l_pt);
          for(int i=0;i<6;++i) fPtMoms->Fill(pow(l_pt,i+1),i+0.5,l_cent);
          fPtDist->Fill(l_pt,l_cent);
          fPtDCA->Fill(l_pt,TMath::Sqrt(trackXYZ[0]*trackXYZ[0]+trackXYZ[1]*trackXYZ[1]),trackXYZ[2]);
      }    
    }
    else if(fIsMC)
    {
      int nTracksMC=0;
      int nTracksRec=0;
      if(fUseRecNchForMC) nTracksRec = GetNTracks(ev,ptMin,ptMax,vtxXYZ);
      TClonesArray *tca = (TClonesArray*)fInputEvent->FindListObject("mcparticles");
      Int_t nPrim = tca->GetEntries();
      if(nPrim<1) return;
      AliAODMCParticle *part;
      double rnd_eff = 0.0;
      vector<int> NtracksPt(fNPtBins);
      for(Int_t ipart = 0; ipart < nPrim; ipart++) {
        part = (AliAODMCParticle*)tca->At(ipart);
        if (!part->IsPhysicalPrimary()) continue;
        if (part->Charge()==0.) continue;
        double l_eta = part->Eta();
        if (TMath::Abs(l_eta) > fEta) continue;
        double l_pt = part->Pt();
        if (l_pt<ptMin || l_pt>ptMax) continue;
        double wNUE = 1.0;
        if(fPseudoEff>0) {
          if(fUseEff){
            wNUE = fEfficiencies[iCent]->GetBinContent(fEfficiencies[iCent]->FindBin(l_pt));
            if(wNUE==0.0) continue;
          }
          else wNUE = fConstEff;
          if(fPseudoEff==1) {
            rnd_eff = fRndm->Rndm(); 
            if(rnd_eff > wNUE) continue;
          }
          if(fPseudoEff==2) { 
            wNUE = fRndm->Gaus(wNUE,fSigmaEff); 
            rnd_eff = fRndm->Rndm(); 
            if(rnd_eff > wNUE) continue;
          }
          wNUE = 1./wNUE;
        }
        if(TMath::Abs(l_eta)<fEtaNch) { nTracksMC++; NtracksPt[fPtDist->GetXaxis()->FindBin(l_pt)-1]++; }
        if(fEtaGap >= 0 && l_eta > fEtaGap) FillWPCounter(wpP,1,l_pt);
        if(fEtaGap >= 0 && l_eta < -fEtaGap) FillWPCounter(wpN,1,l_pt);
        FillWPCounter(wp,wNUE,l_pt);
        for(int i=0;i<6;++i) fPtMoms->Fill(pow(l_pt,i+1),i+0.5,l_cent);
        fPtDist->Fill(l_pt,l_cent);
        fPtDCA->Fill(l_pt,TMath::Sqrt(trackXYZ[0]*trackXYZ[0]+trackXYZ[1]*trackXYZ[1]),trackXYZ[2]);
      }
      nTracks = fUseRecNchForMC?nTracksRec:nTracksMC;
      if(fUseRecNchForMC) fNchTrueVsRec->Fill(nTracksMC,nTracksRec);
      for(int i(0);i<fNPtBins;++i) fPtVsNTrk->Fill(NtracksPt[i],fPtBins[i]+0.001); 
    }
    else
    {
      if(ev->GetNumberOfTracks()<1) return;
      nTracks=GetNTracks(ev,ptMin,ptMax,vtxXYZ);
      for(int iTrack(0); iTrack<ev->GetNumberOfTracks();iTrack++)
      {
          track = (AliAODTrack*)ev->GetTrack(iTrack);
          if(!track) continue;
          double l_eta = track->Eta();
          double trackXYZ[] = {0.,0.,0.};
          if(!AcceptAODTrack(track,trackXYZ,ptMin,ptMax,vtxXYZ)) continue;
          double l_pt = track->Pt();
          if(fUsePowerEff)
          {
            vector<double> wNUE(7,1.0);
            for(int i=0;i<6;++i) {
              wNUE[i+1] = fPowerEfficiencies[iCent]->GetBinContent(fPowerEfficiencies[iCent]->GetXaxis()->FindBin(l_pt),fPowerEfficiencies[iCent]->GetYaxis()->FindBin(i+0.5)); 
              if(wNUE[i+1]==0) continue; 
              wNUE[i+1] = (fUseWeightsOne)?1.0:1.0/wNUE[i+1];
            }
            if(TMath::Abs(l_eta)>0.8) continue;
            if(fEtaGap >= 0 && l_eta > fEtaGap) FillWPCounter(wpP,wNUE,l_pt);
            if(fEtaGap >= 0 && l_eta < -fEtaGap) FillWPCounter(wpN,wNUE,l_pt);
            FillWPCounter(wp,wNUE,l_pt);
          }
          else
          {
            double wNUE = fEfficiencies[iCent]->GetBinContent(fEfficiencies[iCent]->FindBin(l_pt));
            if(wNUE==0.0) continue;
            wNUE = (fUseWeightsOne)?1.0:1.0/wNUE;
            if(TMath::Abs(l_eta)>0.8) continue;
            if(fEtaGap >= 0 && l_eta > fEtaGap) FillWPCounter(wpP,wNUE,l_pt);
            if(fEtaGap >= 0 && l_eta < -fEtaGap) FillWPCounter(wpN,wNUE,l_pt);
            FillWPCounter(wp,wNUE,l_pt);
          }
          for(int i=0;i<6;++i) fPtMoms->Fill(pow(l_pt,i+1),i+0.5,l_cent);
          fPtDist->Fill(l_pt,l_cent);
          fPtDCA->Fill(l_pt,TMath::Sqrt(trackXYZ[0]*trackXYZ[0]+trackXYZ[1]*trackXYZ[1]),trackXYZ[2]);
      }
    }
    if(wp[1][0]==0) return;
    fV0MMulti->Fill(l_cent);
    fV0MvsMult->Fill(l_cent,nTracks);
    double l_mult = fUseNch?(1.0*nTracks):l_cent;
    double l_rnd = fRndm->Rndm();
    //Fill recursive pt-correlations
    fpt->FillRecursive(wp,l_mult,l_rnd);
    //Test with explicit ck and Skew calculation
    fpt->FillCk(wp,l_mult,l_rnd);
    fpt->FillSkew(wp,l_mult,l_rnd);
    //Fill subevent profiles with appropriate wp arrays
    if(fEtaGap>=0) {
      fpt->FillRecursive(wpP,l_mult,l_rnd,"subP"); fpt->FillRecursive(wpN,l_mult,l_rnd,"subN");
    }
    wp.clear();
    wpP.clear();
    wpN.clear();
    PostData(1,fCorrList);
}
int AliAnalysisTaskPtCorr::GetNTracks(AliVEvent* ev, const Double_t &ptmin, const Double_t &ptmax, Double_t *vtxp) 
{
  Double_t ltrackXYZ[3];
  AliAODTrack *lTrack;
  Int_t nTracks=0;
  for(Int_t lTr=0;lTr<ev->GetNumberOfTracks();lTr++) {
    lTrack = (AliAODTrack*)ev->GetTrack(lTr);
    if(!lTrack) continue;
    if(!AcceptAODTrack(lTrack,ltrackXYZ,ptmin,ptmax,vtxp,nTracks)) continue;
  };
  return nTracks;
}
double *AliAnalysisTaskPtCorr::GetBinsFromAxis(TAxis *inax) {
  Int_t lBins = inax->GetNbins();
  Double_t *retBins = new Double_t[lBins+1];
  for(Int_t i=0;i<lBins;i++)
    retBins[i] = inax->GetBinLowEdge(i+1);
  retBins[lBins] = inax->GetBinUpEdge(lBins);
  return retBins;
}
void AliAnalysisTaskPtCorr::SetPtBins(int nPtBins, double *PtBins) {
  if(fPtAxis) delete fPtAxis;
  fPtAxis = new TAxis(nPtBins, PtBins);
}
void AliAnalysisTaskPtCorr::SetMultiplicityBins(int nMultiBins, double *multibins) {
  if(fMultiAxis) delete fMultiAxis;
  fMultiAxis = new TAxis(nMultiBins, multibins);
}
void AliAnalysisTaskPtCorr::SetV0MBins(int nMultiBins, double *multibins) {
  if(fV0MAxis) delete fV0MAxis;
  fV0MAxis = new TAxis(nMultiBins, multibins);
}
