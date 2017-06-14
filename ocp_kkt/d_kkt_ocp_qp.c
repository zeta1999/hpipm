/**************************************************************************************************
*                                                                                                 *
* This file is part of HPIPM.                                                                     *
*                                                                                                 *
* HPIPM -- High Performance Interior Point Method.                                                *
* Copyright (C) 2017 by Gianluca Frison.                                                          *
* Developed at IMTEK (University of Freiburg) under the supervision of Moritz Diehl.              *
* All rights reserved.                                                                            *
*                                                                                                 *
* HPMPC is free software; you can redistribute it and/or                                          *
* modify it under the terms of the GNU Lesser General Public                                      *
* License as published by the Free Software Foundation; either                                    *
* version 2.1 of the License, or (at your option) any later version.                              *
*                                                                                                 *
* HPMPC is distributed in the hope that it will be useful,                                        *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                                  *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                                            *
* See the GNU Lesser General Public License for more details.                                     *
*                                                                                                 *
* You should have received a copy of the GNU Lesser General Public                                *
* License along with HPMPC; if not, write to the Free Software                                    *
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA                  *
*                                                                                                 *
* Author: Gianluca Frison, gianluca.frison (at) imtek.uni-freiburg.de                             *
*                                                                                                 *
**************************************************************************************************/



#include <math.h>

#include <blasfeo_target.h>
#include <blasfeo_common.h>
#include <blasfeo_d_aux.h>
#include <blasfeo_d_blas.h>

#include "../include/hpipm_d_ocp_qp.h"
#include "../include/hpipm_d_ocp_qp_sol.h"
#include "../include/hpipm_d_ipm_hard_ocp_qp.h"
#include "../include/hpipm_d_ipm_hard_core_qp.h"
#include "../include/hpipm_d_aux_ipm_hard.h"



void d_init_var_hard_ocp_qp(struct d_ocp_qp *qp, struct d_ocp_qp_sol *qp_sol, struct d_ipm_hard_ocp_qp_workspace *ws)
	{

//	struct d_ipm_hard_core_qp_workspace *cws = ws->core_workspace;
	
	// loop index
	int ii, jj;

	//
	int N = qp->N;
	int *nx = qp->nx;
	int *nu = qp->nu;
	int *nb = qp->nb;
	int *ng = qp->ng;

	//
	double *ux, *pi, *d_lb, *d_ub, *d_lg, *d_ug, *lam_lb, *lam_ub, *lam_lg, *lam_ug, *t_lb, *t_ub, *t_lg, *t_ug;
	int *idxb;

	double thr0 = 0.1;
	double mu0 = ws->mu0;

	// warm start TODO

	// cold start

	// ux
	for(ii=0; ii<=N; ii++)
		{
		ux = qp_sol->ux[ii].pa;
		for(jj=0; jj<nu[ii]+nx[ii]; jj++)
			{
			ux[jj] = 0.0;
			}
		}
	
	// pi
	for(ii=0; ii<N; ii++)
		{
		pi = qp_sol->pi[ii].pa;
		for(jj=0; jj<nx[ii+1]; jj++)
			{
			pi[jj] = 0.0;
			}
		}
	
	// box constraints
	for(ii=0; ii<=N; ii++)
		{
		ux = qp_sol->ux[ii].pa;
		d_lb = qp->d_lb[ii].pa;
		d_ub = qp->d_ub[ii].pa;
		lam_lb = qp_sol->lam_lb[ii].pa;
		lam_ub = qp_sol->lam_ub[ii].pa;
		t_lb = qp_sol->t_lb[ii].pa;
		t_ub = qp_sol->t_ub[ii].pa;
		idxb = qp->idxb[ii];
		for(jj=0; jj<nb[ii]; jj++)
			{
			t_lb[jj] = - d_lb[jj] + ux[idxb[jj]];
			t_ub[jj] =   d_ub[jj] - ux[idxb[jj]];
			if(t_lb[jj]<thr0)
				{
				if(t_ub[jj]<thr0)
					{
					ux[idxb[jj]] = 0.5*(d_lb[jj]-d_ub[jj]);
					t_lb[jj] = thr0;
					t_ub[jj] = thr0;
					}
				else
					{
					t_lb[jj] = thr0;
					ux[idxb[jj]] = d_lb[jj] + thr0;
					}
				}
			else if(t_ub[jj]<thr0)
				{
				t_ub[jj] = thr0;
				ux[idxb[jj]] = d_ub[jj] - thr0;
				}
			lam_lb[jj] = mu0/t_lb[jj];
			lam_ub[jj] = mu0/t_ub[jj];
			}
		}
	
	// general constraints
	for(ii=0; ii<=N; ii++)
		{
		t_lg = qp_sol->t_lg[ii].pa;
		t_ug = qp_sol->t_ug[ii].pa;
		lam_lg = qp_sol->lam_lg[ii].pa;
		lam_ug = qp_sol->lam_ug[ii].pa;
		d_lg = qp->d_lg[ii].pa;
		d_ug = qp->d_ug[ii].pa;
		ux = qp_sol->ux[ii].pa;
		dgemv_t_libstr(nu[ii]+nx[ii], ng[ii], 1.0, qp->DCt+ii, 0, 0, qp_sol->ux+ii, 0, 0.0, qp_sol->t_lg+ii, 0, qp_sol->t_lg+ii, 0);
		for(jj=0; jj<ng[ii]; jj++)
			{
			t_ug[jj] = - t_lg[jj];
			t_lg[jj] -= d_lg[jj];
			t_ug[jj] += d_ug[jj];
//			t_lg[jj] = fmax(thr0, t_lg[jj]);
//			t_ug[jj] = fmax(thr0, t_ug[jj]);
			t_lg[jj] = thr0>t_lg[jj] ? thr0 : t_lg[jj];
			t_ug[jj] = thr0>t_ug[jj] ? thr0 : t_ug[jj];
			lam_lg[jj] = mu0/t_lg[jj];
			lam_ug[jj] = mu0/t_ug[jj];
			}
		}


	return;

	}



void d_compute_res_hard_ocp_qp(struct d_ocp_qp *qp, struct d_ocp_qp_sol *qp_sol, struct d_ipm_hard_ocp_qp_workspace *ws)
	{

	// loop index
	int ii;

	//
	int N = qp->N;
	int *nx = qp->nx;
	int *nu = qp->nu;
	int *nb = qp->nb;
	int *ng = qp->ng;

	int nbt = ws->core_workspace->nb;
	int ngt = ws->core_workspace->ng;

	struct d_strvec *ux = qp_sol->ux;
	struct d_strvec *pi = qp_sol->pi;
	struct d_strvec *lam_lb = qp_sol->lam_lb;
	struct d_strvec *lam_ub = qp_sol->lam_ub;
	struct d_strvec *lam_lg = qp_sol->lam_lg;
	struct d_strvec *lam_ug = qp_sol->lam_ug;
	struct d_strvec *t_lb = qp_sol->t_lb;
	struct d_strvec *t_ub = qp_sol->t_ub;
	struct d_strvec *t_lg = qp_sol->t_lg;
	struct d_strvec *t_ug = qp_sol->t_ug;

	int nx0, nx1, nu0, nu1, nb0, ng0;

	//
	double mu = 0.0;

	// loop over stages
	for(ii=0; ii<=N; ii++)
		{

		nx0 = nx[ii];
		nu0 = nu[ii];
		nb0 = nb[ii];
		ng0 = ng[ii];

		dveccp_libstr(nu0+nx0, qp->rq+ii, 0, ws->res_g+ii, 0);

		if(ii>0)
			daxpy_libstr(nx0, -1.0, pi+(ii-1), 0, ws->res_g+ii, nu0, ws->res_g+ii, nu0);

		dsymv_l_libstr(nu0+nx0, nu0+nx0, 1.0, qp->RSQrq+ii, 0, 0, ux+ii, 0, 1.0, ws->res_g+ii, 0, ws->res_g+ii, 0);

		if(nb0>0)
			{

			daxpy_libstr(nb0, -1.0, lam_lb+ii, 0, lam_ub+ii, 0, ws->tmp_nbM, 0);
			dvecad_sp_libstr(nb0, 1.0, ws->tmp_nbM, 0, qp->idxb[ii], ws->res_g+ii, 0);

			dvecex_sp_libstr(nb0, -1.0, qp->idxb[ii], ux+ii, 0, ws->res_d_lb+ii, 0);
			dveccp_libstr(nb0, ws->res_d_lb+ii, 0, ws->res_d_ub+ii, 0);
			daxpy_libstr(nb0, 1.0, qp->d_lb+ii, 0, ws->res_d_lb+ii, 0, ws->res_d_lb+ii, 0);
			daxpy_libstr(nb0, 1.0, qp->d_ub+ii, 0, ws->res_d_ub+ii, 0, ws->res_d_ub+ii, 0);
			daxpy_libstr(nb0, 1.0, t_lb+ii, 0, ws->res_d_lb+ii, 0, ws->res_d_lb+ii, 0);
			daxpy_libstr(nb0, -1.0, t_ub+ii, 0, ws->res_d_ub+ii, 0, ws->res_d_ub+ii, 0);

			}

		if(ng0>0)
			{

			// TODO

			}


		if(ii<N)
			{

			nu1 = nu[ii+1];
			nx1 = nx[ii+1];

			daxpy_libstr(nx1, -1.0, ux+(ii+1), nu1, qp->b+ii, 0, ws->res_b+ii, 0);

			dgemv_nt_libstr(nu0+nx0, nx1, 1.0, 1.0, qp->BAbt+ii, 0, 0, pi+ii, 0, ux+ii, 0, 1.0, 1.0, ws->res_g+ii, 0, ws->res_b+ii, 0, ws->res_g+ii, 0, ws->res_b+ii, 0);

			}

		}

	mu += dvecmuldot_libstr(2*nbt+2*ngt, lam_lb, 0, t_lb, 0, ws->res_m, 0);

	ws->res_mu = mu*ws->nt_inv;

	return;

	}



// backward Riccati recursion
void d_fact_solve_kkt_step_hard_ocp_qp(struct d_ocp_qp *qp, struct d_ipm_hard_ocp_qp_workspace *ws)
	{

	int N = qp->N;
	int *nx = qp->nx;
	int *nu = qp->nu;
	int *nb = qp->nb;
	int *ng = qp->ng;

	//
	int ii;

	struct d_ipm_hard_core_qp_workspace *cws = ws->core_workspace;

	if(nb>0 | ng>0)
		{
		d_compute_Qx_qx_hard_qp(cws);
		}

	// factorization and backward substitution

	// last stage
	dtrcp_l_libstr(nu[N]+nx[N], qp->RSQrq+N, 0, 0, ws->L+N, 0, 0); // TODO dtrcp_l_libstr with m and n, for m>=n
	drowin_libstr(nu[N]+nx[N], 1.0, ws->res_g+N, 0, ws->L+N, nu[N]+nx[N], 0);
	if(nb[N]>0)
		{
		ddiaad_sp_libstr(nb[N], 1.0, ws->Qx_lb+N, 0, qp->idxb[N], ws->L+N, 0, 0);
		drowad_sp_libstr(nb[N], 1.0, ws->qx_lb+N, 0, qp->idxb[N], ws->L+N, nu[N]+nx[N], 0);
		}
	if(ng[N]>0)
		{
		// TODO
		}
	else
		{
		dpotrf_l_mn_libstr(nu[N]+nx[N]+1, nu[N]+nx[N], ws->L+N, 0, 0, ws->L+N, 0, 0);
		}

	// middle stages
	for(ii=0; ii<N; ii++)
		{
		dgecp_libstr(nu[N-ii-1]+nx[N-ii-1], nx[N-ii], qp->BAbt+(N-ii-1), 0, 0, ws->AL0, 0, 0);
		drowin_libstr(nx[N-ii], 1.0, ws->res_b+(N-ii-1), 0, ws->AL0, nu[N-ii-1]+nx[N-ii-1], 0);
		dtrmm_rlnn_libstr(nu[N-ii-1]+nx[N-ii-1]+1, nx[N-ii], 1.0, ws->L+(N-ii), nu[N-ii], nu[N-ii], ws->AL0, 0, 0, ws->AL0, 0, 0);
		drowex_libstr(nx[N-ii], 1.0, ws->AL0, nu[N-ii-1]+nx[N-ii-1], 0, ws->tmp_nxM, 0);
		dtrmv_lnn_libstr(nx[N-ii], nx[N-ii], ws->L+(N-ii), nu[N-ii], nu[N-ii], ws->tmp_nxM, 0, ws->Pb+(N-ii-1), 0);
		dgead_libstr(1, nx[N-ii], 1.0, ws->L+(N-ii), nu[N-ii]+nx[N-ii], nu[N-ii], ws->AL0, nu[N-ii-1]+nx[N-ii-1], 0);

//		d_print_strmat(nu[N-ii]+nx[N-ii]+1, nu[N-ii]+nx[N-ii], ws->L+(N-ii), 0, 0);
//		d_print_strmat(nu[N-ii-1]+nx[N-ii-1]+1, nx[N-ii], ws->AL0, 0, 0);
		dtrcp_l_libstr(nu[N-ii-1]+nx[N-ii-1], qp->RSQrq+(N-ii-1), 0, 0, ws->L+(N-ii-1), 0, 0);
		drowin_libstr(nu[N-ii-1]+nx[N-ii-1], 1.0, ws->res_g+(N-ii-1), 0, ws->L+(N-ii-1), nu[N-ii-1]+nx[N-ii-1], 0);

		if(nb[N-ii-1]>0)
			{
			ddiaad_sp_libstr(nb[N-ii-1], 1.0, ws->Qx_lb+(N-ii-1), 0, qp->idxb[N-ii-1], ws->L+(N-ii-1), 0, 0);
			drowad_sp_libstr(nb[N-ii-1], 1.0, ws->qx_lb+(N-ii-1), 0, qp->idxb[N-ii-1], ws->L+(N-ii-1), nu[N-ii-1]+nx[N-ii-1], 0);
			}

		if(ng[N-ii-1]>0)
			{
			// TODO
			}
		else
			{
			dsyrk_dpotrf_ln_libstr(nu[N-ii-1]+nx[N-ii-1]+1, nu[N-ii-1]+nx[N-ii-1], nx[N-ii], ws->AL0, 0, 0, ws->AL0, 0, 0, ws->L+(N-ii-1), 0, 0, ws->L+(N-ii-1), 0, 0);
			}

//		d_print_strmat(nu[N-ii-1]+nx[N-ii-1]+1, nu[N-ii-1]+nx[N-ii-1], ws->L+(N-ii-1), 0, 0);
		}

	// forward substitution

	// first stage
	ii = 0;
	drowex_libstr(nu[ii]+nx[ii], -1.0, ws->L+(ii), nu[ii]+nx[ii], 0, ws->dux+ii, 0);
	dtrsv_ltn_mn_libstr(nu[ii]+nx[ii], nu[ii]+nx[ii], ws->L+ii, 0, 0, ws->dux+ii, 0, ws->dux+ii, 0);
	dgemv_t_libstr(nu[ii]+nx[ii], nx[ii+1], 1.0, qp->BAbt+ii, 0, 0, ws->dux+ii, 0, 1.0, ws->res_b+ii, 0, ws->dux+(ii+1), nu[ii+1]);
	drowex_libstr(nx[ii+1], 1.0, ws->L+(ii+1), nu[ii+1]+nx[ii+1], nu[ii+1], ws->tmp_nxM, 0);
	dtrmv_ltn_libstr(nx[ii+1], nx[ii+1], ws->L+(ii+1), nu[ii+1], nu[ii+1], ws->dux+(ii+1), nu[ii+1], ws->dpi+ii, 0);
	daxpy_libstr(nx[ii+1], 1.0, ws->tmp_nxM, 0, ws->dpi+ii, 0, ws->dpi+ii, 0);
	dtrmv_lnn_libstr(nx[ii+1], nx[ii+1], ws->L+(ii+1), nu[ii+1], nu[ii+1], ws->dpi+ii, 0, ws->dpi+ii, 0);

//	d_print_tran_strvec(nu[ii]+nx[ii], ws->dux+ii, 0);

	// middle stages
	for(ii=1; ii<N; ii++)
		{
		drowex_libstr(nu[ii], -1.0, ws->L+(ii), nu[ii]+nx[ii], 0, ws->dux+ii, 0);
		dtrsv_ltn_mn_libstr(nu[ii]+nx[ii], nu[ii], ws->L+ii, 0, 0, ws->dux+ii, 0, ws->dux+ii, 0);
		dgemv_t_libstr(nu[ii]+nx[ii], nx[ii+1], 1.0, qp->BAbt+ii, 0, 0, ws->dux+ii, 0, 1.0, ws->res_b+ii, 0, ws->dux+(ii+1), nu[ii+1]);
		drowex_libstr(nx[ii+1], 1.0, ws->L+(ii+1), nu[ii+1]+nx[ii+1], nu[ii+1], ws->tmp_nxM, 0);
		dtrmv_ltn_libstr(nx[ii+1], nx[ii+1], ws->L+(ii+1), nu[ii+1], nu[ii+1], ws->dux+(ii+1), nu[ii+1], ws->dpi+ii, 0);
		daxpy_libstr(nx[ii+1], 1.0, ws->tmp_nxM, 0, ws->dpi+ii, 0, ws->dpi+ii, 0);
		dtrmv_lnn_libstr(nx[ii+1], nx[ii+1], ws->L+(ii+1), nu[ii+1], nu[ii+1], ws->dpi+ii, 0, ws->dpi+ii, 0);

//		d_print_tran_strvec(nu[ii]+nx[ii], ws->dux+ii, 0);
		}



	if(nb>0)
		{
		for(ii=0; ii<=N; ii++)
			dvecex_sp_libstr(nb[ii], 1.0, qp->idxb[ii], ws->dux+ii, 0, ws->dt_lb+ii, 0);
		}

	if(ng>0)
		{
		for(ii=0; ii<=N; ii++)
			dgemv_t_libstr(nu[ii]+nx[ii], ng[ii], 1.0, qp->DCt+ii, 0, 0, ws->dux+ii, 0, 0.0, ws->dt_lg+ii, 0, ws->dt_lg+ii, 0);
		}

	if(nb>0 | ng>0)
		{
		d_compute_lam_t_hard_qp(cws);
		}

	return;

	}

