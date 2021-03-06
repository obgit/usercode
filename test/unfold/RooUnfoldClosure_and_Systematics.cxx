//=====================================================================-*-C++-*-
// File and Version Information:
//      $Id: RooUnfoldExample.cxx 248 2010-10-04 22:18:19Z T.J.Adye $
//
// Description:
//      Simple example usage of the RooUnfold package using toy MC.
//
// Authors: Tim Adye <T.J.Adye@rl.ac.uk> and Fergus Wilson <fwilson@slac.stanford.edu>
//          Applied to CMS AFB analysis by Efe Yazgan <efe.yazgan@cern.ch>
//
//==============================================================================

#if !defined(__CINT__) || defined(__MAKECINT__)
#include <iostream>
using std::cout;
using std::endl;

#include "TRandom.h"
#include "TH1.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH1F.h"
#include "TF1.h"
#include "TChain.h"
#include "TMath.h"
#include <TVector.h>
#include <TVector3.h>
#include "TCanvas.h"
#include "TFile.h"
#include "Riostream.h"
#include <fstream>

#include "RooGlobalFunc.h"
#include "RooUnfold.h"
#include "RooUnfoldResponse.h"
#include "RooUnfoldBayes.h"
#include "RooUnfoldSvd.h"
#include "RooUnfoldBinByBin.h"
#include "RooUnfoldInvert.h"
#include "RooUnfoldErrors.h"


#include <iomanip>

#endif

//==============================================================================
// Global definitions
//==============================================================================

const Double_t cutdummy= -99999.0;

//==============================================================================
// Gaussian smearing, systematic translation, and variable inefficiency
//==============================================================================

Double_t smear (Double_t xt)
{
  Double_t xeff= 0.3 + (1.0-0.3)/20*(xt+10.0);  // efficiency
  Double_t x= gRandom->Rndm();
  if (x>xeff) return cutdummy;
  Double_t xsmear= gRandom->Gaus(-2.5,0.2);     // bias and smear
  return xt+xsmear;
}

//==============================================================================
// Example Unfolding
//==============================================================================

void RooUnfoldClosure_and_Systematics()
{
#ifdef __CINT__
  gSystem->Load("libRooUnfold");
#endif

  char name_h[100];
  int nb = 12;
  int nbcos = 8;
  float xAxis_AFB[nb+1]; 
  xAxis_AFB[0] = 40;
  xAxis_AFB[1] = 50;
  xAxis_AFB[2] = 60;
  xAxis_AFB[3] = 70;//
  xAxis_AFB[4] = 80;
  xAxis_AFB[5]  = 88.5;
  xAxis_AFB[6]  = 93.5;
  xAxis_AFB[7] = 96;
  xAxis_AFB[8] = 106;
  xAxis_AFB[9] = 120;
  xAxis_AFB[10] = 150;
  xAxis_AFB[11] = 200;
  xAxis_AFB[12] = 600;
  int nb_Y = 4;
  float Y_bin_limits[nb_Y+1];
  Y_bin_limits[0] = 0.0;
  Y_bin_limits[1] = 1.0;
  Y_bin_limits[2] = 1.25;
  Y_bin_limits[3] = 1.5;
  Y_bin_limits[4] = 2.1;
  int k_reg_cos = 2;//!!!
  int k_iter = 11;
  RooUnfoldResponse* resp[nb][nb_Y];
  RooUnfoldResponse* resp_2[nb][nb_Y];
  //RooUnfoldInvert* unfold_fsr[nb][nb_Y];
  RooUnfoldSvd* unfold_fsr[nb][nb_Y];
  //RooUnfoldBayes* unfold_fsr[nb][nb_Y];
  RooUnfoldSvd* unfold_fsr_up[nb][nb_Y];
  RooUnfoldSvd* unfold_fsr_down[nb][nb_Y];
  RooUnfoldSvd* unfold_dilution[nb][nb_Y];
  //RooUnfoldInvert* unfold_dilution[nb][nb_Y];
  RooUnfoldSvd* unfold_dilution_fsr_up[nb][nb_Y];
  RooUnfoldSvd* unfold_dilution_fsr_down[nb][nb_Y];
  TH1D *hMeasCos_M_Y[30][5];
  TH1D *hNoFsrTruthCos_M_Y[30][5];
  TH1D *hNoFsrTruthCos_M_Y_copy[30][5];
  TH1D *hNoFsrCos_M_Y[30][5];
  TH1D *hTrueCos_M_Y[30][5];
  TH1D *hMeasCos_M_Y_fsr_up[30][5];
  TH1D *hMeasCos_M_Y_fsr_down[30][5];
  TH1D *AFB1_err_M_Y[30][5];
  TH1D *AFB2_err_M_Y[30][5];
  TF1 *fitfunc[30][5]; 
  TFile input_fsr_file("RootFilesForUnfolding/MUON_MC_Meas_NoFsr.root","read");
  TFile input_dilution_file("RootFilesForUnfolding/MUON_MC_NoFsr_NoDilution.root","read");
  //  TFile input_resp_file("Response_and_Historgrams.root","read");
  //  TFile input_data_file("Data_Histograms.root","read");
  for (int i=0;i<nb;i++){
    for (int j=0;j<nb_Y;j++){
      sprintf(name_h,"response_%i_%i",i,j);
      input_dilution_file.GetObject(name_h,resp[i][j]);
      sprintf(name_h,"truth_non_diluted_%i_%i",i,j);
      input_dilution_file.GetObject(name_h,hTrueCos_M_Y[i][j]);
      sprintf(name_h,"np_fsr_cos_%i_%i",i,j);
      input_dilution_file.GetObject(name_h,hNoFsrCos_M_Y[i][j]);

      sprintf(name_h,"response2_%i_%i",i,j);
      input_fsr_file.GetObject(name_h,resp_2[i][j]);
      sprintf(name_h,"truth_no_fsr_%i_%i",i,j);
      input_fsr_file.GetObject(name_h,hNoFsrTruthCos_M_Y[i][j]);


      sprintf(name_h,"MC_meas_%i_%i",i,j);
      input_fsr_file.GetObject(name_h,hMeasCos_M_Y[i][j]);     
      sprintf(name_h,"MC_meas_fsr_up_%i_%i",i,j);
      input_fsr_file.GetObject(name_h,hMeasCos_M_Y_fsr_up[i][j]);
      sprintf(name_h,"MC_meas_fsr_down_%i_%i",i,j);
      input_fsr_file.GetObject(name_h,hMeasCos_M_Y_fsr_down[i][j]);

      sprintf(name_h,"AFB1_err_M_Y_%i_%i",i,j);
      AFB1_err_M_Y[i][j] = new TH1D(name_h,name_h,nbcos,-1.,1.);
      sprintf(name_h,"AFB2_err_M_Y_%i_%i",i,j);
      AFB2_err_M_Y[i][j] = new TH1D(name_h,name_h,nbcos,-1.,1.);

      sprintf(name_h,"Fit_%i_%i",i,j);
      fitfunc[i][j] = new TF1(name_h,"[0]*((3./8.)*(1+x*x)+[1]*x)");
    } 
  }
 
  for (int i=0;i<nb;i++){
    for (int j=0;j<nb_Y;j++){
      hNoFsrTruthCos_M_Y_copy[i][j] = hNoFsrTruthCos_M_Y[i][j];
      hNoFsrTruthCos_M_Y[i][j]->Scale(hMeasCos_M_Y[i][j]->Integral()*1./hNoFsrTruthCos_M_Y[i][j]->Integral()*1.);
      unfold_fsr[i][j] = new RooUnfoldSvd(resp_2[i][j], hMeasCos_M_Y[i][j], k_reg_cos);
      //unfold_fsr[i][j] = new RooUnfoldBayes(resp_2[i][j], hMeasCos_M_Y[i][j], k_iter);
      //unfold_fsr[i][j] = new RooUnfoldInvert(resp_2[i][j], hMeasCos_M_Y[i][j]);
      //unfold_fsr_up[i][j] = new RooUnfoldSvd(resp_2[i][j], hMeasCos_M_Y_fsr_up[i][j], k_reg_cos);//fsr
      //unfold_fsr_down[i][j] = new RooUnfoldSvd(resp_2[i][j], hMeasCos_M_Y_fsr_down[i][j], k_reg_cos);//fsr
    }
  }

  TH1D* hRecoCos[nb][nb_Y];
  TH1D* hRecoCos2[nb][nb_Y];
  TH1D* hRecoCos2_copy[nb][nb_Y];


  TH1D* hRecoCos_fsr_up[nb][nb_Y];
  TH1D* hRecoCos_fsr_down[nb][nb_Y];
  TH1D* hRecoCos2_fsr_up[nb][nb_Y];
  TH1D* hRecoCos2_fsr_down[nb][nb_Y];

  const double *cov_a;
  const double *cov_a_fsr_up;
  const double *cov_a_fsr_down;

  const double *cov_a2;
  const double *cov_a2_fsr_up;
  const double *cov_a2_fsr_down;

  char name_outhisto2[100];
  TH1D *AFB[nb_Y];
  TH1D *h_Unfold_Y2[nb_Y]; 
  TH1D *h_Unfold_Y2_fsr_up[nb_Y]; 
  TH1D *h_Unfold_Y2_fsr_down[nb_Y]; 
  TH1D *h_Unfold_Y2_fsr[nb_Y];				       
  TH1D *h_Meas_Y2[nb_Y]; 
  TH1D *h_True_Y2[nb_Y]; 
  TH1D *FSR_Error_1_UP_Y[5];
  TH1D *FSR_Error_1_DOWN_Y[5];
  TH1D *AFB_err_unfold_meas_1[5];
  int bin_min,bin_mid,bin_mid_for_backward,bin_max;
  double NF_unfold, NB_unfold, NF_meas, NB_meas, NF_true, NB_true, afb;
  double afb_unf_error = 0;
  double afb_unf_error_central = 0;
  double binw;
  double NF_unfold_fsr_up, NB_unfold_fsr_up;
  double NF_unfold_fsr_down, NB_unfold_fsr_down;
  TAxis *axis = hMeasCos_M_Y[0][0]->GetXaxis();
  bin_mid = axis->FindBin(0.);
  bin_mid_for_backward = bin_mid - 1;
  bin_min = axis->FindBin(-1.);
  bin_max = axis->FindBin(1.) - 1;
  binw = axis->GetBinWidth(bin_mid);
  //you do not need to worry about bin size because of the ratio


  double err2[nbcos];
  double error2[nb][nb_Y];
  double err[nbcos];
  double error[nb][nb_Y];

  char matrix_name[100];
  for (int i=0;i<nb;i++){
    err2[i] = 0;
    err[i] = 0;
    for (int j=0;j<nb_Y;j++){
      error2[i][j] = 0;
      error[i][j] = 0;
    }
  }  



  for (int i=0;i<nb;i++){
    for (int j=0;j<nb_Y;j++){
      hRecoCos2[i][j] = (TH1D*) unfold_fsr[i][j]->Hreco();
      hRecoCos2_copy[i][j] = hRecoCos2[i][j];
      TMatrixD cov2 = unfold_fsr[i][j]->Ereco();
      //cov2.Print();
      cov_a2 = cov2.GetMatrixArray();
      for (int l=0;l<nbcos;l++){
	err2[l] = 0;
	for (int m=0;m<nbcos;m++){
	  int k = nbcos*l+m;
	  err2[l] += cov_a2[k]; 
	  error2[i][j] += cov_a2[k]; 
	}
	hRecoCos2[i][j]->SetBinError(l+1,sqrt(err2[l]));
      }     
      error2[i][j] = sqrt(error2[i][j])*sqrt(2*(pow(hRecoCos2[i][j]->Integral(bin_min,bin_mid_for_backward),2)+pow(hRecoCos2[i][j]->Integral(bin_mid,bin_max),2)))/(pow(hRecoCos2[i][j]->Integral(),2));

    }
  }

  
  cout<<"UNFOLDED FSR"<<endl;

  cout<<"if you want to get the fsr systemnatics uncomments the fsr's at the comments!"<<endl;


  /*fsr
  for (int i=0;i<nb;i++){
    for (int j=0;j<nb_Y;j++){
      hRecoCos2_fsr_up[i][j] = (TH1D*) unfold_fsr_up[i][j]->Hreco();
      TMatrixD cov2_fsr_up = unfold_fsr_up[i][j]->Ereco();
      //cov2_fsr_up.Print();
      cov_a2_fsr_up = cov2_fsr_up.GetMatrixArray();
      for (int l=0;l<nbcos;l++){
	for (int m=0;m<nbcos;m++){
	  int k = nbcos*l+m;
     
	}
      }
    }
  }

  cout<<"UNFOLDED FSR SCALE UP"<<endl;

  for (int i=0;i<nb;i++){
    for (int j=0;j<nb_Y;j++){
      hRecoCos2_fsr_down[i][j] = (TH1D*) unfold_fsr_down[i][j]->Hreco();
      TMatrixD cov2_fsr_down = unfold_fsr_down[i][j]->Ereco();
      //cov2_fsr_down.Print();
      cov_a2_fsr_down = cov2_fsr_down.GetMatrixArray();
      for (int l=0;l<nbcos;l++){
	for (int m=0;m<nbcos;m++){
	  int k = nbcos*l+m;
 
	}
      }
    }
  }

  cout<<"UNFOLDED FSR SCALE DOWN"<<endl;
  */ //fsr

  for (int i=0;i<nb;i++){
    for (int j=0;j<nb_Y;j++){
      float scale_tmp = hRecoCos2[i][j]->Integral()*1./hTrueCos_M_Y[i][j]->Integral()*1.;
      hTrueCos_M_Y[i][j]->Scale(scale_tmp);
      unfold_dilution[i][j] = new RooUnfoldSvd(resp[i][j], hRecoCos2_copy[i][j], k_reg_cos);
      //unfold_dilution[i][j] = new RooUnfoldInvert(resp[i][j], hRecoCos2_copy[i][j]);
      //unfold_dilution[i][j] = new RooUnfoldSvd(resp[i][j], hNoFsrTruthCos_M_Y_copy[i][j], k_reg_cos);
      //unfold_dilution[i][j] = new RooUnfoldSvd(resp[i][j], hNoFsrCos_M_Y[i][j], k_reg_cos);

      /* fsr
      unfold_dilution_fsr_up[i][j] = new RooUnfoldSvd(resp[i][j], hRecoCos2_fsr_up[i][j], k_reg_cos);
      unfold_dilution_fsr_down[i][j] = new RooUnfoldSvd(resp[i][j], hRecoCos2_fsr_down[i][j], k_reg_cos);
      */ // fsr
    }
  }


  
  for (int i=0;i<nb;i++){
    for (int j=0;j<nb_Y;j++){
      hRecoCos[i][j] = (TH1D*) unfold_dilution[i][j]->Hreco();
      TMatrixD cov = unfold_dilution[i][j]->Ereco();
      sprintf(matrix_name,"cov_mat1__%i_%i",i,j);
      //cov.Print();
      //     cov.Write(matrix_name);
      cov_a = cov.GetMatrixArray();
      for (int l=0;l<nbcos;l++){
	err[l] = 0;	
	for (int m=0;m<nbcos;m++){
	  int k = nbcos*l+m;
     	  err[l] += cov_a[k];  
	  error[i][j] += cov_a[k];
	}
	hRecoCos[i][j]->SetBinError(l+1,sqrt(err2[l]));
      }
      error[i][j] = sqrt(error[i][j])*sqrt(2*(pow(hRecoCos[i][j]->Integral(bin_min,bin_mid_for_backward),2)+pow(hRecoCos[i][j]->Integral(bin_mid,bin_max),2)))/(pow(hRecoCos[i][j]->Integral(),2));
      error[i][j] = sqrt(error[i][j]*error[i][j]+error2[i][j]*error2[i][j]);
    }
  }
  /* fsr
  for (int i=0;i<nb;i++){
    for (int j=0;j<nb_Y;j++){
      hRecoCos_fsr_up[i][j] = (TH1D*) unfold_dilution_fsr_up[i][j]->Hreco();
      TMatrixD cov_fsr_up = unfold_dilution_fsr_up[i][j]->Ereco();
      //sprintf(matrix_name,"cov_mat1__%i_%i",i,j);
      //cov.Print();
      //     cov.Write(matrix_name);
      cov_a_fsr_up = cov_fsr_up.GetMatrixArray();
      for (int l=0;l<nbcos;l++){
	for (int m=0;m<nbcos;m++){
	  int k = nbcos*l+m;
 
	}
      }
    }
  }	
  
  for (int i=0;i<nb;i++){
    for (int j=0;j<nb_Y;j++){
      hRecoCos_fsr_down[i][j] = (TH1D*) unfold_dilution_fsr_down[i][j]->Hreco();
      TMatrixD cov_fsr_down = unfold_dilution_fsr_down[i][j]->Ereco();
      //sprintf(matrix_name,"cov_mat1__%i_%i",i,j);
      //cov.Print();
      //     cov.Write(matrix_name);
      cov_a_fsr_down = cov_fsr_down.GetMatrixArray();
      for (int l=0;l<nbcos;l++){
	for (int m=0;m<nbcos;m++){
	  int k = nbcos*l+m;
     
	}
      }
    }
  }		
  
  */ //fsr

  cout<<"UNFOLDED DILUTION"<<endl;

  float scale[nb][nb_Y],scale2[nb][nb_Y];
  TCanvas *c_rap[nb_Y];
  char name_outfile[100];
  for (int j=0;j<nb_Y;j++){
    c_rap[j] = new TCanvas();
    c_rap[j]->Divide(4,3);
    for (int i=0;i<nb;i++){
      c_rap[j]->cd(i+1);
      hRecoCos2[i][j]->SetMarkerStyle(8);
      scale2[i][j] = hNoFsrTruthCos_M_Y[i][j]->Integral()*1./hRecoCos2[i][j]->Integral()*1.;
      cout<<"scale 2   "<<i<<"   "<<j<<"   "<<scale2[i][j]<<endl;
      scale[i][j] = hTrueCos_M_Y[i][j]->Integral()*1./hRecoCos[i][j]->Integral()*1.;//!!!
      //      hRecoCos2[i][j]->Scale(scale2[i][j]);
      //hNoFsrTruthCos_M_Y[i][j]->Scale(1./scale2[i][j]);
      for (int k=0;k<nbcos;k++){ 
	cout<<"UNFOLDED(fsr)/MEASURED ERROR  = "<<hRecoCos2[i][j]->GetBinError(k+1)/hMeasCos_M_Y[i][j]->GetBinError(k+1)<<endl;
	cout<<"==>UNFOLDED(dilut)/UNFOLDED(fsr) = "<<hRecoCos[i][j]->GetBinError(k+1)/hRecoCos2[i][j]->GetBinError(k+1)<<endl;
      }
      hRecoCos2[i][j]->DrawNormalized("e1");
      
      hRecoCos[i][j]->SetMarkerStyle(8);
      hRecoCos[i][j]->SetMarkerColor(8);
      hRecoCos[i][j]->Scale(scale[i][j]);
      hRecoCos[i][j]->DrawNormalized("e1sames");
      hRecoCos[i][j]->Fit(fitfunc[i][j],"MQ","",-1.,1.);      

      hMeasCos_M_Y[i][j]->SetMarkerColor(2);
      hMeasCos_M_Y[i][j]->SetLineColor(2);
      hMeasCos_M_Y[i][j]->SetMarkerStyle(25);
      hMeasCos_M_Y[i][j]->DrawNormalized("e1SAMES");//!!!!
      hNoFsrTruthCos_M_Y[i][j]->SetLineColor(4);
      hNoFsrTruthCos_M_Y[i][j]->DrawNormalized("SAMES");
      
      hTrueCos_M_Y[i][j]->SetLineColor(4);
      hTrueCos_M_Y[i][j]->DrawNormalized("SAMES"); 
    }
    sprintf(name_outfile,"Data_and_Unfolded_Y%i_%i_costheta.C",j,j+1);
    c_rap[j]->SaveAs(name_outfile);
  }


  
  
  TCanvas *c_AFB2 = new TCanvas();
  c_AFB2->Divide(2,2);

  for (int j=0;j<nb_Y;j++){
    c_AFB2->cd(j+1);
    gPad->SetLogx();
    sprintf(name_outhisto2,"h_Unfold_Y2_%i",j+1);
    h_Unfold_Y2[j] = new TH1D(name_outhisto2,name_outhisto2,nb, xAxis_AFB);

    sprintf(name_outhisto2,"h_Unfold_Y2_fsr_up_%i",j+1);
    h_Unfold_Y2_fsr_up[j] = new TH1D(name_outhisto2,name_outhisto2,nb, xAxis_AFB);
    sprintf(name_outhisto2,"h_Unfold_Y2_fsr_down_%i",j+1);
    h_Unfold_Y2_fsr_down[j] = new TH1D(name_outhisto2,name_outhisto2,nb, xAxis_AFB);

    sprintf(name_outhisto2,"h_Unfold_Y2_fsr_%i",j+1);
    h_Unfold_Y2_fsr[j] = new TH1D(name_outhisto2,name_outhisto2,nb, xAxis_AFB);

    sprintf(name_outhisto2,"h_Meas_Y2_%i",j+1);
    h_Meas_Y2[j] = new TH1D(name_outhisto2,name_outhisto2,nb, xAxis_AFB);
    sprintf(name_outhisto2,"h_True_Y2_%i",j+1);
    h_True_Y2[j] = new TH1D(name_outhisto2,name_outhisto2,nb, xAxis_AFB);

    sprintf(name_outhisto2,"AFB_%i",j+1);
    AFB[j] = new TH1D(name_outhisto2,name_outhisto2,nb, xAxis_AFB);


    sprintf(name_outhisto2,"FSR_Error_1_UP_Y_%i",j+1);
    FSR_Error_1_UP_Y[j] = new TH1D(name_outhisto2,name_outhisto2, nb, xAxis_AFB);
    sprintf(name_outhisto2,"FSR_Error_1_DOWN_Y_%i",j+1);
    FSR_Error_1_DOWN_Y[j] = new TH1D(name_outhisto2,name_outhisto2, nb, xAxis_AFB);

    sprintf(name_outhisto2,"AFB_err_unfold_meas_1__%i",j+1);
    AFB_err_unfold_meas_1[j] = new TH1D(name_outhisto2,name_outhisto2, nb, xAxis_AFB);

    for (int i=0;i<nb;i++){
      for (int l=0;l<nbcos;l++){
	AFB1_err_M_Y[i][j]->SetBinContent(l+1,hRecoCos2[i][j]->GetBinError(l+1)/hMeasCos_M_Y[i][j]->GetBinError(l+1));
      }
      NF_unfold = hRecoCos2[i][j]->Integral(bin_mid,bin_max);
      NB_unfold = hRecoCos2[i][j]->Integral(bin_min,bin_mid_for_backward);
      h_Unfold_Y2[j]->SetBinContent(i+1,(NF_unfold-NB_unfold)/(NF_unfold+NB_unfold));
      h_Unfold_Y2[j]->SetBinError(i+1,error2[i][j]);
      /* fsr
      NF_unfold_fsr_up = hRecoCos2_fsr_up[i][j]->Integral(bin_mid,bin_max);
      NB_unfold_fsr_up = hRecoCos2_fsr_up[i][j]->Integral(bin_min,bin_mid_for_backward);
      h_Unfold_Y2_fsr_up[j]->SetBinContent(i+1,(NF_unfold_fsr_up-NB_unfold_fsr_up)/(NF_unfold_fsr_up+NB_unfold_fsr_up));
      h_Unfold_Y2_fsr_up[j]->SetBinError(i+1,afb_unf_error);

      NF_unfold_fsr_down = hRecoCos2_fsr_down[i][j]->Integral(bin_mid,bin_max);
      NB_unfold_fsr_down = hRecoCos2_fsr_down[i][j]->Integral(bin_min,bin_mid_for_backward);
      h_Unfold_Y2_fsr_down[j]->SetBinContent(i+1,(NF_unfold_fsr_down-NB_unfold_fsr_down)/(NF_unfold_fsr_down+NB_unfold_fsr_down));
      h_Unfold_Y2_fsr_down[j]->SetBinError(i+1,afb_unf_error);

      float diff1 = h_Unfold_Y2_fsr_down[j]->GetBinContent(i+1) - h_Unfold_Y2[j]->GetBinContent(i+1);
      float diff2 = h_Unfold_Y2_fsr_up[j]->GetBinContent(i+1) - h_Unfold_Y2[j]->GetBinContent(i+1);
      float fsr_cent = (diff1+diff2)/2.;
      float fsr_err = 0.;
      if (diff1-fsr_cent > 0.) fsr_err = diff1 - fsr_cent;
      if (diff2-fsr_cent > 0.) fsr_err = diff2 - fsr_cent;
      h_Unfold_Y2_fsr[j]->SetBinContent(i+1,(h_Unfold_Y2_fsr_down[j]->GetBinContent(i+1)+h_Unfold_Y2_fsr_up[j]->GetBinContent(i+1))/2.);
      h_Unfold_Y2_fsr[j]->SetBinError(i+1,fsr_err);
      */ //fsr
      NF_meas = hMeasCos_M_Y[i][j]->Integral(bin_mid,bin_max);//!!!!
      NB_meas = hMeasCos_M_Y[i][j]->Integral(bin_min,bin_mid_for_backward);//!!!!
      /*
      NF_meas = hNoFsrCos_M_Y[i][j]->Integral(bin_mid,bin_max);
      NB_meas = hNoFsrCos_M_Y[i][j]->Integral(bin_min,bin_mid_for_backward);
      */
      afb = (NF_meas-NB_meas)/(NF_meas+NB_meas);
      h_Meas_Y2[j]->SetBinContent(i+1,afb);
      h_Meas_Y2[j]->SetBinError(i+1,sqrt((1-afb*afb)/(binw*(NF_meas+NB_meas))));
      //   cout<<"afb_unf_error, afb_error, afb_unf_error/afb_error"<<i<<"  "<<j<<"  "<<afb_unf_error_central<<"  "<<h_Meas_Y2[j]->GetBinError(i+1)<<"   "<<afb_unf_error_central/h_Meas_Y2[j]->GetBinError(i+1)<<endl;
      AFB_err_unfold_meas_1[j]->SetBinContent(i+1,afb_unf_error_central/h_Meas_Y2[j]->GetBinError(i+1));
      NF_true = hNoFsrTruthCos_M_Y[i][j]->Integral(bin_mid,bin_max);
      NB_true = hNoFsrTruthCos_M_Y[i][j]->Integral(bin_min,bin_mid_for_backward);
      afb = (NF_true-NB_true)/(NF_true+NB_true);
      h_True_Y2[j]->SetBinContent(i+1,afb);
      //h_True_Y2[j]->SetBinError(i+1,sqrt((1-afb*afb)/(binw*(NF_true+NB_true))));
    }
    h_Meas_Y2[j]->SetLineColor(4);
    h_Meas_Y2[j]->SetMarkerStyle(8);
    h_Meas_Y2[j]->SetMarkerColor(4);
    //    h_Meas_Y[j]->SetFillColor(4);
    h_Meas_Y2[j]->GetYaxis()->SetRangeUser(-1,1);
    h_Meas_Y2[j]->GetXaxis()->SetTitle("M(#mu^{+}#mu^{-}) [GeV]");
    h_Meas_Y2[j]->GetYaxis()->SetTitle("A_{FB}");
    h_Meas_Y2[j]->Draw("");
    h_True_Y2[j]->SetLineColor(2);
    h_True_Y2[j]->SetLineWidth(2);
    //    h_True_Y[j]->SetFillColor(2);
    h_True_Y2[j]->Draw("sames");
    /*
    h_Unfold_Y2_fsr_up[j]->SetFillColor(7);
    h_Unfold_Y2_fsr_up[j]->Draw("e3sames");
    h_Unfold_Y2_fsr_down[j]->SetFillColor(7); 
    h_Unfold_Y2_fsr_down[j]->Draw("e3sames");  
    */

    /* fsr
    h_Unfold_Y2_fsr[j]->SetFillColor(8); 
    h_Unfold_Y2_fsr[j]->Draw("e2sames");  
    */ //fsr

    h_Unfold_Y2[j]->SetMarkerStyle(8);
    h_Unfold_Y2[j]->Draw("e1sames");  
  
    
  }
  
  c_AFB2->SaveAs("AFB_NoFsr_Level_CLOSURE_AND_SYSTEMATICS.C");
  

  ///

  TCanvas *c_AFB = new TCanvas();
  c_AFB->Divide(2,2);
  char name_outhisto[100];
  TH1D *h_Unfold_Y[nb_Y]; 
  TH1D *h_Unfold_Y_fsr_up[nb_Y]; 
  TH1D *h_Unfold_Y_fsr_down[nb_Y]; 
  TH1D *h_Unfold_Y_fsr[nb_Y];
  TH1D *h_Meas_Y[nb_Y]; 
  TH1D *h_True_Y[nb_Y]; 
  TH1D *FSR_Error_2_UP_Y[5];
  TH1D *FSR_Error_2_DOWN_Y[5];
  TH1D *AFB_err_unfold_meas_2[5];
  for (int j=0;j<nb_Y;j++){
    c_AFB->cd(j+1);
    gPad->SetLogx();
    sprintf(name_outhisto,"h_Unfold_Y%i",j+1);
    h_Unfold_Y[j] = new TH1D(name_outhisto,name_outhisto,nb, xAxis_AFB);

    sprintf(name_outhisto,"h_Unfold_Y_fsr_up%i",j+1);
    h_Unfold_Y_fsr_up[j] = new TH1D(name_outhisto,name_outhisto,nb, xAxis_AFB);
    sprintf(name_outhisto,"h_Unfold_Y_fsr_down%i",j+1);
    h_Unfold_Y_fsr_down[j] = new TH1D(name_outhisto,name_outhisto,nb, xAxis_AFB);

    sprintf(name_outhisto,"h_Unfold_Y_fsr_%i",j+1);
    h_Unfold_Y_fsr[j] = new TH1D(name_outhisto,name_outhisto,nb, xAxis_AFB);

    sprintf(name_outhisto,"h_Meas_Y%i",j+1);
    h_Meas_Y[j] = new TH1D(name_outhisto,name_outhisto,nb, xAxis_AFB);
    sprintf(name_outhisto,"h_True_Y%i",j+1);
    h_True_Y[j] = new TH1D(name_outhisto,name_outhisto,nb, xAxis_AFB);

    sprintf(name_outhisto,"FSR_Error_2_UP_Y_%i",j+1);
    FSR_Error_2_UP_Y[j] = new TH1D(name_outhisto,name_outhisto, nb, xAxis_AFB);
    sprintf(name_outhisto,"FSR_Error_2_DOWN_Y_%i",j+1);
    FSR_Error_2_DOWN_Y[j] = new TH1D(name_outhisto,name_outhisto, nb, xAxis_AFB);

    sprintf(name_outhisto,"AFB_err_unfold_meas_2__%i",j+1);
    AFB_err_unfold_meas_2[j] = new TH1D(name_outhisto,name_outhisto, nb, xAxis_AFB);




    for (int i=0;i<nb;i++){

      AFB[j]->SetBinContent(i+1,fitfunc[i][j]->GetParameter(1));
      AFB[j]->SetBinError(i+1,error[i][j]);


      for (int l=0;l<nbcos;l++){
	AFB2_err_M_Y[i][j]->SetBinContent(l+1,hRecoCos[i][j]->GetBinError(l+1)/hRecoCos2[i][j]->GetBinError(l+1));
      }
      NF_unfold = hRecoCos[i][j]->Integral(bin_mid,bin_max);
      NB_unfold = hRecoCos[i][j]->Integral(bin_min,bin_mid_for_backward);

      h_Unfold_Y[j]->SetBinContent(i+1,(NF_unfold-NB_unfold)/(NF_unfold+NB_unfold));
      h_Unfold_Y[j]->SetBinError(i+1,error[i][j]);

      /* fsr
      NF_unfold = hRecoCos_fsr_up[i][j]->Integral(bin_mid,bin_max);
      NB_unfold = hRecoCos_fsr_up[i][j]->Integral(bin_min,bin_mid_for_backward);
      afb_unf_error =  numerator_fsr_up[i][j]/pow((NF_unfold+NB_unfold),2);
      afb_unf_error = sqrt(afb_unf_error);
      afb_unf_error = scale[i][j]*afb_unf_error;
      h_Unfold_Y_fsr_up[j]->SetBinContent(i+1,(NF_unfold-NB_unfold)/(NF_unfold+NB_unfold));
      h_Unfold_Y_fsr_up[j]->SetBinError(i+1,afb_unf_error);

      NF_unfold = hRecoCos_fsr_down[i][j]->Integral(bin_mid,bin_max);
      NB_unfold = hRecoCos_fsr_down[i][j]->Integral(bin_min,bin_mid_for_backward);
      h_Unfold_Y_fsr_down[j]->SetBinContent(i+1,(NF_unfold-NB_unfold)/(NF_unfold+NB_unfold));
      h_Unfold_Y_fsr_down[j]->SetBinError(i+1,afb_unf_error);

      float diff1 = h_Unfold_Y_fsr_down[j]->GetBinContent(i+1) - h_Unfold_Y[j]->GetBinContent(i+1);
      float diff2 = h_Unfold_Y_fsr_up[j]->GetBinContent(i+1) - h_Unfold_Y[j]->GetBinContent(i+1);
      float fsr_cent = (diff1+diff2)/2.;
      float fsr_err = 0.;
      if (diff1-fsr_cent > 0.) fsr_err = diff1 - fsr_cent;
      if (diff2-fsr_cent > 0.) fsr_err = diff2 - fsr_cent;
      h_Unfold_Y_fsr[j]->SetBinContent(i+1,(h_Unfold_Y_fsr_down[j]->GetBinContent(i+1)+h_Unfold_Y_fsr_up[j]->GetBinContent(i+1))/2.);
      h_Unfold_Y_fsr[j]->SetBinError(i+1,fsr_err);
      */ //fsr

      NF_meas = hRecoCos2[i][j]->Integral(bin_mid,bin_max);//!!!!
      NB_meas = hRecoCos2[i][j]->Integral(bin_min,bin_mid_for_backward);//!!!!
      
      //NF_meas = hNoFsrCos_M_Y[i][j]->Integral(bin_mid,bin_max);
      //NB_meas = hNoFsrCos_M_Y[i][j]->Integral(bin_min,bin_mid_for_backward);
      
      afb = (NF_meas-NB_meas)/(NF_meas+NB_meas);
      h_Meas_Y[j]->SetBinContent(i+1,afb);
      h_Meas_Y[j]->SetBinError(i+1,sqrt((1-afb*afb)/(binw*(NF_meas+NB_meas))));
      //cout<<"afb_unf_error, afb_error, afb_unf_error/afb_error"<<i<<"  "<<j<<"  "<<afb_unf_error_central<<"  "<<h_Meas_Y[j]->GetBinError(i+1)<<"   "<<afb_unf_error_central/h_Meas_Y[j]->GetBinError(i+1)<<endl;
      AFB_err_unfold_meas_2[j]->SetBinContent(i+1,afb_unf_error_central/h_Meas_Y2[j]->GetBinError(i+1));
      NF_true = hTrueCos_M_Y[i][j]->Integral(bin_mid,bin_max);
      NB_true = hTrueCos_M_Y[i][j]->Integral(bin_min,bin_mid_for_backward);
      afb = (NF_true-NB_true)/(NF_true+NB_true);
      h_True_Y[j]->SetBinContent(i+1,afb);
      //h_True_Y[j]->SetBinError(i+1,sqrt((1-afb*afb)/(binw*(NF_true+NB_true))));
    }
    h_Meas_Y[j]->SetLineColor(4);
    h_Meas_Y[j]->SetMarkerStyle(8);
    h_Meas_Y[j]->SetMarkerColor(4);
    //    h_Meas_Y[j]->SetFillColor(4);
    h_Meas_Y[j]->GetYaxis()->SetRangeUser(-1,1);
    h_Meas_Y[j]->GetXaxis()->SetTitle("M(#mu^{+}#mu^{-}) [GeV]");
    h_Meas_Y[j]->GetYaxis()->SetTitle("A_{FB}");
    h_Meas_Y[j]->Draw("");
    h_True_Y[j]->SetLineColor(2);
    h_True_Y[j]->SetLineWidth(2);
    //    h_True_Y[j]->SetFillColor(2);
    h_True_Y[j]->Draw("sames");
    /*
    h_Unfold_Y_fsr_up[j]->SetFillColor(7);
    h_Unfold_Y_fsr_up[j]->Draw("e3sames");  
    h_Unfold_Y_fsr_down[j]->SetFillColor(7);
    h_Unfold_Y_fsr_down[j]->Draw("e3sames");  
    */

    /*fsr
    h_Unfold_Y_fsr[j]->SetFillColor(8); 
    h_Unfold_Y_fsr[j]->Draw("e2sames");  
    */ //fsr
    h_Unfold_Y[j]->SetMarkerStyle(8);
    h_Unfold_Y[j]->Draw("sames");    

    AFB[j]->SetMarkerColor(7);
    AFB[j]->Draw("e1sames");
  }
  
  c_AFB->SaveAs("AFB_NonDiluted_Level_CLOSURE_AND_SYSTEMATICS.C");


  /* fsr

  cout<<"STEP 1 FSR SYSTEMATICS"<<endl;
  cout<<"---------------------------"<<endl;
  cout<<"Y bin        Mass  bin     (central-up)/central (%)      (central-down)/central (%)"<<endl;
  for (int j=0;j<nb_Y;j++){
    for (int i=0;i<nb;i++){
      float centr = h_Unfold_Y2[j]->GetBinContent(i+1);
      float up =  h_Unfold_Y2_fsr_up[j]->GetBinContent(i+1);
      float down =  h_Unfold_Y2_fsr_down[j]->GetBinContent(i+1);
      cout<<Y_bin_limits[j]<<"-"<<Y_bin_limits[j+1]<<" &  "<<xAxis_AFB[i]<<"-"<<xAxis_AFB[i+1]<<"  &  "<<setprecision(3)<<100*(centr-up)/centr<<"  &  "<<setprecision(3)<<100*(centr-down)/centr<<"\\\\"<<endl;
      FSR_Error_1_UP_Y[j]->SetBinContent(i+1,100*(centr-up)/centr);
      FSR_Error_1_DOWN_Y[j]->SetBinContent(i+1,100*(centr-down)/centr);
    }
  }

  cout<<"STEP 2 FSR SYSTEMATICS"<<endl;
  cout<<"---------------------------"<<endl;
  cout<<"Y bin        Mass  bin     (central-up)/central (%)      (central-down)/central (%)"<<endl;
  for (int j=0;j<nb_Y;j++){
    for (int i=0;i<nb;i++){
      float centr = h_Unfold_Y[j]->GetBinContent(i+1);
      float up =  h_Unfold_Y_fsr_up[j]->GetBinContent(i+1);
      float down =  h_Unfold_Y_fsr_down[j]->GetBinContent(i+1);
      //      cout<<Y_bin_limits[j]<<"-"<<Y_bin_limits[j+1]<<" &  "<<xAxis_AFB[i]<<"-"<<xAxis_AFB[i+1]<<"  &  "<<setprecision(3)<<100*(centr-up)/centr<<"  &  "<<setprecision(3)<<100*(centr-down)/centr<<"\\\\"<<endl;
      FSR_Error_2_UP_Y[j]->SetBinContent(i+1,100*(centr-up)/centr);
      FSR_Error_2_DOWN_Y[j]->SetBinContent(i+1,100*(centr-down)/centr);
    }
  }


  TCanvas *c_fsr1 = new TCanvas();
  c_fsr1->Divide(2,2);
  for (int j=0;j<nb_Y;j++){
    c_fsr1->cd(j+1);
    FSR_Error_1_UP_Y[j]->SetLineColor(4);
    FSR_Error_1_UP_Y[j]->Draw();
    FSR_Error_1_DOWN_Y[j]->SetLineColor(2);
    FSR_Error_1_DOWN_Y[j]->Draw("sames");    
  }
  c_fsr1->SaveAs("fsr1_uncertainty_perc.C");

  TCanvas *c_fsr2 = new TCanvas();
  c_fsr2->Divide(2,2);
  for (int j=0;j<nb_Y;j++){
    c_fsr2->cd(j+1);
    FSR_Error_2_UP_Y[j]->SetLineColor(4);
    FSR_Error_2_UP_Y[j]->Draw();
    FSR_Error_2_DOWN_Y[j]->SetLineColor(2);
    FSR_Error_2_DOWN_Y[j]->Draw("sames");    
  }
  c_fsr2->SaveAs("fsr2_uncertainty_perc.C");

  
  TCanvas *c_afbe1 = new TCanvas();
  c_afbe1->Divide(2,2);
  for (int j=0;j<nb_Y;j++){ 
    c_afbe1->cd(j+1);
    AFB_err_unfold_meas_1[j]->Draw();
  }
  c_afbe1->SaveAs("afb1_error_unfold_to_meas.C");

  TCanvas *c_afbe2 = new TCanvas();
  c_afbe2->Divide(2,2);
  for (int j=0;j<nb_Y;j++){ 
    c_afbe2->cd(j+1);
    AFB_err_unfold_meas_2[j]->Draw();
  }
  c_afbe2->SaveAs("afb2_error_unfold_to_meas.C"); 

  TCanvas *c_afb_err_my1[nb_Y];
  char name_o[100];
  for (int j=0;j<nb_Y;j++){
    c_afb_err_my1[j] = new TCanvas();
    c_afb_err_my1[j]->Divide(4,3);
    for (int i=0;i<nb;i++){
      c_afb_err_my1[j]->cd(i+1);
      AFB1_err_M_Y[i][j]->Draw();
    }
    sprintf(name_o,"afb1_err_M_Y__%i.C",j);
    c_afb_err_my1[j]->SaveAs(name_o);
  }

  TCanvas *c_afb_err_my2[nb_Y];
  for (int j=0;j<nb_Y;j++){
    c_afb_err_my2[j] = new TCanvas();
    c_afb_err_my2[j]->Divide(4,3);
    for (int i=0;i<nb;i++){
      c_afb_err_my2[j]->cd(i+1);
      AFB2_err_M_Y[i][j]->Draw();
    }
    sprintf(name_o,"afb2_err_M_Y__%i.C",j);
    c_afb_err_my2[j]->SaveAs(name_o);
  }

  */ //fsr


 
  
  

}




#ifndef __CINT__
int main () { RooUnfoldClosure_and_Systematics(); return 0; }  // Main program when run stand-alone
#endif
