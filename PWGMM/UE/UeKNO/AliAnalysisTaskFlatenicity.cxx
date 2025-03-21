/****************************************************************************
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
 *                                                                        *
 * Author: Antonio Ortiz (antonio.ortiz@nucleares.unam.mx)                *
 * Anatask to compute flatenicity (arXiv:2204.13733)                      *
 **************************************************************************/

class TTree;

class AliPPVsMultUtils;
class AliESDtrackCuts;

#include "AliAnalysisManager.h"
#include "AliAnalysisTask.h"
#include "AliAnalysisTaskESDfilter.h"
#include "AliAnalysisTaskSE.h"
#include "AliAnalysisUtils.h"
#include "AliCentrality.h"
#include "AliESDEvent.h"
#include "AliESDInputHandler.h"
#include "AliESDUtils.h"
#include "AliESDVZERO.h"
#include "AliESDtrack.h"
#include "AliESDtrackCuts.h"
#include "AliEventCuts.h"
#include "AliGenCocktailEventHeader.h"
#include "AliGenEventHeader.h"
#include "AliInputEventHandler.h"
#include "AliLog.h"
#include "AliMCEvent.h"
#include "AliMCEventHandler.h"
#include "AliMCParticle.h"
#include "AliMultEstimator.h"
#include "AliMultInput.h"
#include "AliMultSelection.h"
#include "AliMultVariable.h"
#include "AliMultiplicity.h"
#include "AliOADBContainer.h"
#include "AliOADBMultSelection.h"
#include "AliPPVsMultUtils.h"
#include "AliStack.h"
#include "AliVEvent.h"
#include "AliVTrack.h"
#include "AliVVertex.h"
#include "TCanvas.h"
#include "TChain.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "THnSparse.h"
#include "TLegend.h"
#include "TList.h"
#include "TMath.h"
#include "TParticle.h"
#include "TProfile.h"
#include "TVector3.h"
#include <AliAnalysisFilter.h>
#include <AliESDVertex.h>
#include <AliHeader.h>
#include <AliMultiplicity.h>
#include <Riostream.h>
#include <TBits.h>
#include <TDirectory.h>
#include <TMath.h>
#include <TRandom.h>
#include <TTree.h>
using std::cout;
using std::endl;

#include "AliAnalysisTaskFlatenicity.h"

const Int_t nPtbins = 36;
Double_t Ptbins[nPtbins + 1] = {
    0.0, 0.1, 0.15, 0.2,  0.25, 0.3,  0.35, 0.4,  0.45, 0.5,  0.6, 0.7, 0.8,
    0.9, 1.0, 1.25, 1.5,  2.0,  2.5,  3.0,  3.5,  4.0,  4.5,  5.0, 6.0, 7.0,
    8.0, 9.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0, 30.0, 40.0, 50.0};
const Int_t nCent = 9;
Double_t centClass[nCent + 1] = {0.0,  1.0,  5.0,  10.0, 20.0,
                                 30.0, 40.0, 50.0, 70.0, 100.0};

using namespace std; // std namespace: so you can do things like 'cout' etc

ClassImp(AliAnalysisTaskFlatenicity) // classimp: necessary for root

    AliAnalysisTaskFlatenicity::AliAnalysisTaskFlatenicity()
    : AliAnalysisTaskSE(), fESD(0), fEventCuts(0x0), fMCStack(0), fMC(0),
      fUseMC(kFALSE), fV0Mindex(-1), fIsMCclosure(kFALSE),
      fRemoveTrivialScaling(kFALSE), fnGen(-1), fPIDResponse(0x0),
      fTrackFilter(0x0), fOutputList(0), fEtaCut(0.8), fPtMin(0.5),
      ftrackmult08(0), fv0mpercentile(0), fFlat(-1), fFlatMC(-1),
      fMultSelection(0x0), hPtPrimIn(0), hPtPrimOut(0), hPtSecOut(0), hPtOut(0),
      hFlatenicity(0), hFlatenicityMC(0), hFlatResponse(0), hFlatVsPt(0),
      hFlatVsPtMC(0), hActivityV0DataSect(0), hActivityV0McSect(0),
      hFlatVsNchMC(0), hFlatVsV0M(0), hCounter(0) {
  for (Int_t i_c = 0; i_c < nCent; ++i_c) {
    hFlatVsPtV0M[i_c] = 0;
  }
}
//_____________________________________________________________________________
AliAnalysisTaskFlatenicity::AliAnalysisTaskFlatenicity(const char *name)
    : AliAnalysisTaskSE(name), fESD(0), fEventCuts(0x0), fMCStack(0), fMC(0),
      fUseMC(kFALSE), fV0Mindex(-1), fIsMCclosure(kFALSE),
      fRemoveTrivialScaling(kFALSE), fnGen(-1), fPIDResponse(0x0),
      fTrackFilter(0x0), fOutputList(0), fEtaCut(0.8), fPtMin(0.5),
      ftrackmult08(0), fv0mpercentile(0), fFlat(-1), fFlatMC(-1),
      fMultSelection(0x0), hPtPrimIn(0), hPtPrimOut(0), hPtSecOut(0), hPtOut(0),
      hFlatenicity(0), hFlatenicityMC(0), hFlatResponse(0), hFlatVsPt(0),
      hFlatVsPtMC(0), hActivityV0DataSect(0), hActivityV0McSect(0),
      hFlatVsNchMC(0), hFlatVsV0M(0), hCounter(0)

{
  for (Int_t i_c = 0; i_c < nCent; ++i_c) {
    hFlatVsPtV0M[i_c] = 0;
  }

  DefineInput(0, TChain::Class()); // define the input of the analysis: in this
                                   // case you take a 'chain' of events
  DefineOutput(1, TList::Class()); // define the ouptut of the analysis: in this
                                   // case it's a list of histograms
}
//_____________________________________________________________________________
AliAnalysisTaskFlatenicity::~AliAnalysisTaskFlatenicity() {
  // destructor
  if (fOutputList) {
    delete fOutputList; // at the end of your task, it is deleted from memory by
                        // calling this function
    fOutputList = 0x0;
  }
}
//_____________________________________________________________________________
void AliAnalysisTaskFlatenicity::UserCreateOutputObjects() {

  // create track filters
  fTrackFilter = new AliAnalysisFilter("trackFilter");
  AliESDtrackCuts *fCuts = new AliESDtrackCuts();
  fCuts->SetAcceptKinkDaughters(kFALSE);
  fCuts->SetRequireTPCRefit(kTRUE);
  fCuts->SetRequireITSRefit(kTRUE);
  fCuts->SetClusterRequirementITS(AliESDtrackCuts::kSPD, AliESDtrackCuts::kAny);
  fCuts->SetDCAToVertex2D(kFALSE);
  fCuts->SetRequireSigmaToVertex(kFALSE);
  fCuts->SetEtaRange(-0.8, 0.8);
  fCuts->SetMinNCrossedRowsTPC(70);
  fCuts->SetMinRatioCrossedRowsOverFindableClustersTPC(0.8);
  fCuts->SetMaxChi2PerClusterTPC(4);
  fCuts->SetMaxDCAToVertexZ(2);
  fCuts->SetCutGeoNcrNcl(3., 130., 1.5, 0.85, 0.7);
  fCuts->SetMinRatioCrossedRowsOverFindableClustersTPC(0.8);
  fCuts->SetMaxChi2PerClusterTPC(4);
  fCuts->SetMaxDCAToVertexZ(2);
  fCuts->SetMaxChi2PerClusterITS(36);
  fCuts->SetMaxDCAToVertexXYPtDep("0.0105+0.0350/pt^1.1");
  fCuts->SetMaxChi2PerClusterITS(36);
  fTrackFilter->AddCuts(fCuts);

  // create output objects

  OpenFile(1);
  fOutputList =
      new TList(); // this is a list which will contain all of your histograms
  fOutputList->SetOwner(kTRUE); // memory stuff: the list is owner of all

  hFlatenicity = new TH1D("hFlatenicity", "counter", 1000, -0.1, 9.9);
  fOutputList->Add(hFlatenicity);

  hFlatVsPt =
      new TH2D("hFlatVsPt", "Measured; Flatenicity; #it{p}_{T} (GeV/#it{c})",
               1000, -0.1, 9.9, nPtbins, Ptbins);
  fOutputList->Add(hFlatVsPt);

  for (Int_t i_c = 0; i_c < nCent; ++i_c) {
    hFlatVsPtV0M[i_c] = new TH2D(
        Form("hFlatVsPtV0M_c%d", i_c),
        Form("Measured %1.0f-%1.0f%%V0M; Flatenicity; #it{p}_{T} (GeV/#it{c})",
             centClass[i_c], centClass[i_c + 1]),
        1000, -0.1, 9.9, nPtbins, Ptbins);
    fOutputList->Add(hFlatVsPtV0M[i_c]);
  }

  if (fUseMC) {
    hPtPrimIn =
        new TH1D("hPtPrimIn", "Prim In; #it{p}_{T} (GeV/#it{c}; counts)",
                 nPtbins, Ptbins);
    fOutputList->Add(hPtPrimIn);

    hPtPrimOut =
        new TH1D("hPtPrimOut", "Prim Out; #it{p}_{T} (GeV/#it{c}; counts)",
                 nPtbins, Ptbins);
    fOutputList->Add(hPtPrimOut);

    hPtSecOut =
        new TH1D("hPtSecOut", "Sec Out; #it{p}_{T} (GeV/#it{c}; counts)",
                 nPtbins, Ptbins);
    fOutputList->Add(hPtSecOut);

    hPtOut = new TH1D("hPtOut", "all Out; #it{p}_{T} (GeV/#it{c}; counts)",
                      nPtbins, Ptbins);
    fOutputList->Add(hPtOut);

    hFlatenicityMC = new TH1D("hFlatenicityMC", "counter", 1000, -0.1, 9.9);
    fOutputList->Add(hFlatenicityMC);
    hFlatResponse = new TH2D("hFlatResponse", "; true flat; measured flat",
                             1000, -0.1, 9.9, 1000, -0.1, 9.9);
    fOutputList->Add(hFlatResponse);
    hFlatVsPtMC =
        new TH2D("hFlatVsPtMC", "MC true; Flatenicity; #it{p}_{T} (GeV/#it{c})",
                 1000, -0.1, 9.9, nPtbins, Ptbins);
    fOutputList->Add(hFlatVsPtMC);

    hFlatVsNchMC = new TH2D("hFlatVsNchMC", "; true flat; true Nch", 1000, -0.1,
                            9.9, 100, -0.5, 99.5);
    fOutputList->Add(hFlatVsNchMC);
  }

  hActivityV0DataSect =
      new TProfile("hActivityV0DataSect", "rec; V0 sector; #LTmultiplicity#GT",
                   64, -0.5, 63.5);
  fOutputList->Add(hActivityV0DataSect);
  if (fUseMC) {
    hActivityV0McSect =
        new TProfile("hActivityV0McSect", "true; V0 sector; #LTmultiplicity#GT",
                     64, -0.5, 63.5);
    fOutputList->Add(hActivityV0McSect);
  }

  hFlatVsV0M = new TH2D("hFlatVsV0M", "", nCent, centClass, 1000, -0.1, 9.9);
  fOutputList->Add(hFlatVsV0M);

  hCounter = new TH1D("hCounter", "counter", 10, -0.5, 9.5);
  fOutputList->Add(hCounter);

  fEventCuts.AddQAplotsToList(fOutputList);
  PostData(1, fOutputList); // postdata will notify the analysis manager of
                            // changes / updates to the
}
//_____________________________________________________________________________
void AliAnalysisTaskFlatenicity::UserExec(Option_t *) {

  AliVEvent *event = InputEvent();
  if (!event) {
    Error("UserExec", "Could not retrieve event");
    return;
  }

  fESD = dynamic_cast<AliESDEvent *>(event);

  if (!fESD) {
    Printf("%s:%d ESDEvent not found in Input Manager", (char *)__FILE__,
           __LINE__);
    this->Dump();
    return;
  }

  if (fUseMC) {

    //      E S D
    fMC = dynamic_cast<AliMCEvent *>(MCEvent());
    if (!fMC) {
      Printf("%s:%d MCEvent not found in Input Manager", (char *)__FILE__,
             __LINE__);
      this->Dump();
      return;
    }
    fMCStack = fMC->Stack();
  }

  AliHeader *headerMC;
  Bool_t isGoodVtxPosMC = kFALSE;

  if (fUseMC) {
    headerMC = fMC->Header();
    AliGenEventHeader *genHeader = headerMC->GenEventHeader();
    TArrayF vtxMC(3); // primary vertex  MC
    vtxMC[0] = 9999;
    vtxMC[1] = 9999;
    vtxMC[2] = 9999; // initialize with dummy
    if (genHeader) {
      genHeader->PrimaryVertex(vtxMC);
    }
    if (TMath::Abs(vtxMC[2]) <= 10)
      isGoodVtxPosMC = kTRUE;
  }

  // Trigger selection
  UInt_t fSelectMask = fInputHandler->IsEventSelected();
  Bool_t isINT7selected = fSelectMask & AliVEvent::kINT7;
  if (!isINT7selected)
    return;

  // Good events
  if (!fEventCuts.AcceptEvent(event)) {
    PostData(1, fOutputList);
    return;
  }

  // Good vertex
  Bool_t hasRecVertex = kFALSE;
  hasRecVertex = HasRecVertex();
  if (!hasRecVertex)
    return;

  // Multiplicity Estimation
  fv0mpercentile = -999;

  fMultSelection = (AliMultSelection *)fESD->FindListObject("MultSelection");
  if (!fMultSelection)
    cout << "------- No AliMultSelection Object Found --------"
         << fMultSelection << endl;
  fv0mpercentile = fMultSelection->GetMultiplicityPercentile("V0M");
  hCounter->Fill(1);

  for (Int_t i_c = 0; i_c < nCent; ++i_c) {
    if (fv0mpercentile >= centClass[i_c] &&
        fv0mpercentile < centClass[i_c + 1]) {
      fV0Mindex = i_c;
    } else {
      continue;
    }
  }

  fFlat = GetFlatenicity();
  fFlatMC = -1;
  if (fUseMC) {
    fFlatMC = GetFlatenicityMC();
    if (fFlatMC > 0) {
      hFlatenicityMC->Fill(fFlatMC);
      hFlatResponse->Fill(fFlatMC, fFlat);
      MakeMCanalysis();
    }
  }
  if (fFlat > 0) {
    hFlatenicity->Fill(fFlat);
    if (fV0Mindex) {
      hFlatVsV0M->Fill(fv0mpercentile, fFlat);
      MakeDataanalysis();
    }
  }

  if (fIsMCclosure) {
    Double_t randomUE = -1;
    gRandom->SetSeed(0);
    randomUE = gRandom->Uniform(0.0, 1.0);
    if (randomUE < 0.5) { // corrections (50% stat.)
      if (isGoodVtxPosMC) {
      }
    } else { // for testing the method
    }
  } else {
    if (fUseMC) {
      if (isGoodVtxPosMC) {
      }
    } else {
    }
  }

  PostData(1, fOutputList); // stream the result of this event to the output
                            // manager which will write it to a file
}
//______________________________________________________________________________
void AliAnalysisTaskFlatenicity::Terminate(Option_t *) {}
//______________________________________________________________________________
void AliAnalysisTaskFlatenicity::MakeDataanalysis() {

  // rec
  Int_t nTracks = fESD->GetNumberOfTracks();
  for (Int_t iT = 0; iT < nTracks; ++iT) {

    AliESDtrack *esdtrack = static_cast<AliESDtrack *>(
        fESD->GetTrack(iT)); // get a track (type AliesdTrack)
    if (!esdtrack)
      continue;
    if (!fTrackFilter->IsSelected(esdtrack))
      continue;
    if (TMath::Abs(esdtrack->Eta()) > fEtaCut)
      continue;
    if (esdtrack->Pt() < fPtMin)
      continue;
    hFlatVsPt->Fill(fFlat, esdtrack->Pt());
    hFlatVsPtV0M[fV0Mindex]->Fill(fFlat, esdtrack->Pt());
  }
}

//______________________________________________________________________________
void AliAnalysisTaskFlatenicity::MakeMCanalysis() {

  for (Int_t i = 0; i < fMC->GetNumberOfTracks(); ++i) {

    AliMCParticle *particle = (AliMCParticle *)fMC->GetTrack(i);
    if (!particle)
      continue;
    if (!fMC->IsPhysicalPrimary(i))
      continue;
    if (TMath::Abs(particle->Eta()) > fEtaCut)
      continue;
    if (particle->Pt() < fPtMin)
      continue;
    if (TMath::Abs(particle->Charge()) < 0.1)
      continue;
    hFlatVsPtMC->Fill(fFlatMC, particle->Pt());
    hPtPrimIn->Fill(particle->Pt());
  }
  // rec
  Int_t nTracks = fESD->GetNumberOfTracks();
  for (Int_t iT = 0; iT < nTracks; ++iT) {

    AliESDtrack *esdtrack = static_cast<AliESDtrack *>(
        fESD->GetTrack(iT)); // get a track (type AliesdTrack)
    if (!esdtrack)
      continue;
    if (!fTrackFilter->IsSelected(esdtrack))
      continue;
    if (TMath::Abs(esdtrack->Eta()) > fEtaCut)
      continue;
    if (esdtrack->Pt() < fPtMin)
      continue;
    hPtOut->Fill(esdtrack->Pt());
    Int_t mcLabel = -1;
    mcLabel = TMath::Abs(esdtrack->GetLabel());
    if (fMC->IsPhysicalPrimary(mcLabel)) {
      hPtPrimOut->Fill(esdtrack->Pt());
    } else {
      hPtSecOut->Fill(esdtrack->Pt());
    }
  }
}

//______________________________________________________________________________
Double_t AliAnalysisTaskFlatenicity::GetFlatenicity() {

  AliVVZERO *lVV0 = 0x0;
  AliVEvent *lVevent = 0x0;
  lVevent = dynamic_cast<AliVEvent *>(InputEvent());
  if (!lVevent) {
    AliWarning("ERROR: ESD / AOD event not available \n");
    return -1;
  }
  // Get VZERO Information for multiplicity later
  lVV0 = lVevent->GetVZEROData();
  if (!lVV0) {
    AliError("AliVVZERO not available");
    return -1;
  }
  // Flatenicity calculation
  const Int_t nRings = 4;
  const Int_t nSectors = 8;
  Float_t minEtaV0C[nRings] = {-3.7, -3.2, -2.7, -2.2};
  Float_t maxEtaV0C[nRings] = {-3.2, -2.7, -2.2, -1.7};
  Float_t maxEtaV0A[nRings] = {5.1, 4.5, 3.9, 3.4};
  Float_t minEtaV0A[nRings] = {4.5, 3.9, 3.4, 2.8};
  // Grid
  const Int_t nCells = nRings * 2 * nSectors;
  Float_t RhoLattice[nCells];
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    RhoLattice[iCh] = 0.0;
  }

  Int_t nringA = 0;
  Int_t nringC = 0;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    Float_t detaV0 = -1;
    Float_t mult = lVV0->GetMultiplicity(iCh);
    if (iCh < 32) { // V0C
      if (iCh < 8) {
        nringC = 0;
      } else if (iCh >= 8 && iCh < 16) {
        nringC = 1;
      } else if (iCh >= 16 && iCh < 24) {
        nringC = 2;
      } else {
        nringC = 3;
      }
      detaV0 = maxEtaV0C[nringC] - minEtaV0C[nringC];
    } else { // V0A
      if (iCh < 40) {
        nringA = 0;
      } else if (iCh >= 40 && iCh < 48) {
        nringA = 1;
      } else if (iCh >= 48 && iCh < 56) {
        nringA = 2;
      } else {
        nringA = 3;
      }
      detaV0 = maxEtaV0A[nringA] - minEtaV0A[nringA];
    }
    RhoLattice[iCh] =
        mult / detaV0; // needed to consider the different eta coverage
  }
  // Filling histos with mult info
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    hActivityV0DataSect->Fill(iCh, RhoLattice[iCh]);
  }
  Float_t mRho = 0;
  Float_t flatenicity = -1;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    mRho += RhoLattice[iCh];
  }
  // average activity per cell
  mRho /= (1.0 * nCells);
  // get sigma
  Double_t sRho_tmp = 0;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    sRho_tmp += TMath::Power(1.0 * RhoLattice[iCh] - mRho, 2);
  }
  sRho_tmp /= (1.0 * nCells);
  Float_t sRho = TMath::Sqrt(sRho_tmp);
  if (mRho > 0) {
    if (fRemoveTrivialScaling) {
      flatenicity = TMath::Sqrt(mRho) * sRho / mRho;
    } else {
      flatenicity = sRho / mRho;
    }
  } else {
    flatenicity = -1;
  }
  return flatenicity;
}
//______________________________________________________________________________
Double_t AliAnalysisTaskFlatenicity::GetFlatenicityMC() {

  // Flatenicity calculation
  const Int_t nRings = 8;
  Float_t maxEta[nRings] = {-3.2, -2.7, -2.2, -1.7, 5.1, 4.5, 3.9, 3.4};
  Float_t minEta[nRings] = {-3.7, -3.2, -2.7, -2.2, 4.5, 3.9, 3.4, 2.8};

  const Int_t nSectors = 8;
  Float_t PhiBins[nSectors + 1];
  Float_t deltaPhi = (2.0 * TMath::Pi()) / (1.0 * nSectors);
  for (int i_phi = 0; i_phi < nSectors + 1; ++i_phi) {
    PhiBins[i_phi] = 0;
    if (i_phi < nSectors) {
      PhiBins[i_phi] = i_phi * deltaPhi;
    } else {
      PhiBins[i_phi] = 2.0 * TMath::Pi();
    }
  }

  // Grid
  const Int_t nCells = nRings * nSectors;
  Float_t RhoLattice[nCells];
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    RhoLattice[iCh] = 0.0;
  }

  Int_t nMult = 0;
  for (Int_t i = 0; i < fMC->GetNumberOfTracks(); ++i) {

    AliMCParticle *particle = (AliMCParticle *)fMC->GetTrack(i);
    if (!particle)
      continue;
    if (!fMC->IsPhysicalPrimary(i))
      continue;
    if (particle->Pt() <= 0.0)
      continue;
    if (TMath::Abs(particle->Charge()) < 0.1)
      continue;
    Double_t phi = particle->Phi();
    Double_t eta = particle->Eta();

    Int_t i_segment = 0;
    for (int i_eta = 0; i_eta < nRings; ++i_eta) {

      for (int i_phi = 0; i_phi < nSectors; ++i_phi) {

        if (eta >= minEta[i_eta] && eta < maxEta[i_eta] &&
            phi >= PhiBins[i_phi] && phi < PhiBins[i_phi + 1]) {
          nMult++;
          RhoLattice[i_segment] += 1.0;
        }
        i_segment++;
      }
    }
  }

  Int_t i_segment = 0;
  for (int i_eta = 0; i_eta < nRings; ++i_eta) {
    for (int i_phi = 0; i_phi < nSectors; ++i_phi) {
      Float_t deltaEta = TMath::Abs(maxEta[i_eta] - minEta[i_eta]);
      RhoLattice[i_segment] /= deltaEta;
      // Filling histos with mult info
      hActivityV0McSect->Fill(i_segment, RhoLattice[i_segment]);
      i_segment++;
    }
  }

  Float_t mRho = 0;
  Float_t flatenicity = -1;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    mRho += RhoLattice[iCh];
  }
  // average activity per cell
  mRho /= (1.0 * nCells);
  // get sigma
  Float_t sRho_tmp = 0;
  for (Int_t iCh = 0; iCh < nCells; iCh++) {
    sRho_tmp += TMath::Power(1.0 * RhoLattice[iCh] - mRho, 2);
  }
  sRho_tmp /= (1.0 * nCells);
  Float_t sRho = TMath::Sqrt(sRho_tmp);
  if (mRho > 0) {
    if (fRemoveTrivialScaling) {
      flatenicity = TMath::Sqrt(mRho) * sRho / mRho;
    } else {
      flatenicity = sRho / mRho;
    }
  } else {
    sRho = -1;
  }
  hFlatVsNchMC->Fill(flatenicity, nMult);
  return flatenicity;
}

Bool_t AliAnalysisTaskFlatenicity::HasRecVertex() {

  float fMaxDeltaSpdTrackAbsolute = 0.5f;
  float fMaxDeltaSpdTrackNsigmaSPD = 1.e14f;
  float fMaxDeltaSpdTrackNsigmaTrack = 1.e14;
  float fMaxResolutionSPDvertex = 0.25f;
  float fMaxDispersionSPDvertex = 1.e14f;

  Bool_t fRequireTrackVertex = true;
  unsigned long fFlag;
  fFlag = BIT(AliEventCuts::kNoCuts);

  const AliVVertex *vtTrc = fESD->GetPrimaryVertex();
  bool isTrackV = true;
  if (vtTrc->IsFromVertexer3D() || vtTrc->IsFromVertexerZ())
    isTrackV = false;
  const AliVVertex *vtSPD = fESD->GetPrimaryVertexSPD();

  if (vtSPD->GetNContributors() > 0)
    fFlag |= BIT(AliEventCuts::kVertexSPD);

  if (vtTrc->GetNContributors() > 1 && isTrackV)
    fFlag |= BIT(AliEventCuts::kVertexTracks);

  if (((fFlag & BIT(AliEventCuts::kVertexTracks)) || !fRequireTrackVertex) &&
      (fFlag & BIT(AliEventCuts::kVertexSPD)))
    fFlag |= BIT(AliEventCuts::kVertex);

  const AliVVertex *&vtx =
      bool(fFlag & BIT(AliEventCuts::kVertexTracks)) ? vtTrc : vtSPD;
  AliVVertex *fPrimaryVertex = const_cast<AliVVertex *>(vtx);
  if (!fPrimaryVertex)
    return kFALSE;

  /// Vertex quality cuts
  double covTrc[6], covSPD[6];
  vtTrc->GetCovarianceMatrix(covTrc);
  vtSPD->GetCovarianceMatrix(covSPD);
  double dz = bool(fFlag & AliEventCuts::kVertexSPD) &&
                      bool(fFlag & AliEventCuts::kVertexTracks)
                  ? vtTrc->GetZ() - vtSPD->GetZ()
                  : 0.; /// If one of the two vertices is not available this cut
                        /// is always passed.
  double errTot = TMath::Sqrt(covTrc[5] + covSPD[5]);
  double errTrc =
      bool(fFlag & AliEventCuts::kVertexTracks) ? TMath::Sqrt(covTrc[5]) : 1.;
  double nsigTot = TMath::Abs(dz) / errTot, nsigTrc = TMath::Abs(dz) / errTrc;
  /// vertex dispersion for run1, only for ESD, AOD code to be added here
  const AliESDVertex *vtSPDESD = dynamic_cast<const AliESDVertex *>(vtSPD);
  double vtSPDdispersion = vtSPDESD ? vtSPDESD->GetDispersion() : 0;
  if ((TMath::Abs(dz) <= fMaxDeltaSpdTrackAbsolute &&
       nsigTot <= fMaxDeltaSpdTrackNsigmaSPD &&
       nsigTrc <=
           fMaxDeltaSpdTrackNsigmaTrack) && // discrepancy track-SPD vertex
      (!vtSPD->IsFromVertexerZ() ||
       TMath::Sqrt(covSPD[5]) <= fMaxResolutionSPDvertex) &&
      (!vtSPD->IsFromVertexerZ() ||
       vtSPDdispersion <= fMaxDispersionSPDvertex) /// vertex dispersion cut for
                                                   /// run1, only for ESD
      ) // quality cut on vertexer SPD z
    fFlag |= BIT(AliEventCuts::kVertexQuality);

  Bool_t hasVtx = (TESTBIT(fFlag, AliEventCuts::kVertex)) &&
                  (TESTBIT(fFlag, AliEventCuts::kVertexQuality));

  return hasVtx;
}
