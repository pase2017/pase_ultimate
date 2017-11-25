#include <stdio.h>
#include <stdlib.h>
#include "pase_multigrid.h"

#include "HYPRE_utilities.h"
#include "_hypre_parcsr_mv.h"
#include "_hypre_parcsr_ls.h"

PASE_MULTIGRID 
PASE_Multigrid_create(PASE_MATRIX A, PASE_MATRIX B, PASE_PARAMETER param, PASE_MULTIGRID_OPERATOR ops)
{
    PASE_MULTIGRID multigrid = (PASE_MULTIGRID)PASE_Malloc(sizeof(PASE_MULTIGRID_PRIVATE));
    if(NULL != ops) {
	multigrid->ops = (PASE_MULTIGRID_OPERATOR) PASE_Malloc(sizeof(PASE_MULTIGRID_OPERATOR_PRIVATE));
	*(multigrid->ops) = *ops;
    } else {
	multigrid->ops = PASE_Multigrid_operator_create_by_default(A->data_struct);
    }

    void **A_array, **P_array, **R_array; 
    PASE_MATRIX tmp;
    multigrid->ops->get_amg_array(A->matrix_data, 
				  param, 
				  &(A_array),
				  &(P_array),
				  &(R_array),
				  &(multigrid->actual_level),
				  &(multigrid->amg_data));
    multigrid->A        = (PASE_MATRIX*)PASE_Malloc(multigrid->actual_level*sizeof(PASE_MATRIX));
    multigrid->B        = (PASE_MATRIX*)PASE_Malloc(multigrid->actual_level*sizeof(PASE_MATRIX));
    multigrid->P        = (PASE_MATRIX*)PASE_Malloc((multigrid->actual_level-1)*sizeof(PASE_MATRIX));
    multigrid->R        = (PASE_MATRIX*)PASE_Malloc((multigrid->actual_level-1)*sizeof(PASE_MATRIX));
    multigrid->aux_A    = (PASE_AUX_MATRIX*)PASE_Malloc(multigrid->actual_level*sizeof(PASE_AUX_MATRIX));
    multigrid->aux_B    = (PASE_AUX_MATRIX*)PASE_Malloc(multigrid->actual_level*sizeof(PASE_AUX_MATRIX));
    PASE_INT level      = 0;
    multigrid->A[0]     = A;
    multigrid->B[0]     = B;
    multigrid->aux_A[0] = NULL;
    multigrid->aux_B[0] = NULL;
    for(level=1; level<multigrid->actual_level; level++) {
        multigrid->A[level]                = PASE_Matrix_create_by_operator(A_array[level], A->ops);
	multigrid->A[level]->data_struct   = A->data_struct;
	//multigrid->A[level]->is_matrix_data_owner = 1;
        multigrid->P[level-1]              = PASE_Matrix_create_by_operator(P_array[level-1], A->ops);
	multigrid->P[level-1]->data_struct = A->data_struct;
	//multigrid->P[level-1]->is_matrix_data_owner = 1;
        multigrid->R[level-1]              = PASE_Matrix_create_by_operator(R_array[level-1], A->ops);
	multigrid->R[level-1]->is_matrix_data_owner = 1;
	multigrid->R[level-1]->data_struct = A->data_struct;

        /* B1 = R0 * B0 * P0 */
        tmp               = PASE_Matrix_multiply_matrix(multigrid->B[level-1], multigrid->P[level-1]); 
        multigrid->B[level]   = PASE_Matrix_multiply_matrix(multigrid->R[level-1], tmp); 
	PASE_Matrix_destroy(tmp);

	multigrid->aux_A[level] = NULL;
	multigrid->aux_B[level] = NULL;
    }
    //PASE_Free(A_array);
    //PASE_Free(P_array);
    PASE_Free(R_array);

    return multigrid;
}

void 
PASE_Multigrid_destroy(PASE_MULTIGRID multigrid)
{
    PASE_INT i = 0;
    if(NULL != multigrid && multigrid->actual_level > 1) {
	if(multigrid->aux_A) {
	    for(i=0; i<multigrid->actual_level; i++) {
	        PASE_Aux_matrix_destroy(multigrid->aux_A[i]);
	        multigrid->aux_A[i] = NULL;
	    }
	    PASE_Free(multigrid->aux_A);
	    multigrid->aux_A = NULL;
	}
	if(multigrid->aux_B) {
	    for(i=0; i<multigrid->actual_level; i++) {
	        PASE_Aux_matrix_destroy(multigrid->aux_B[i]);
	        multigrid->aux_B[i] = NULL;
	    }
	    PASE_Free(multigrid->aux_B);
	    multigrid->aux_B = NULL;
	}
	if(multigrid->P) {
	    for(i=0; i<multigrid->actual_level-1; i++) {
	        PASE_Matrix_destroy(multigrid->P[i]);
	        multigrid->P[i] = NULL;
	    }
	    PASE_Free(multigrid->P);
	    multigrid->P = NULL;
	}
	if(multigrid->R) {
	    for(i=0; i<multigrid->actual_level-1; i++) {
	        PASE_Matrix_destroy(multigrid->R[i]);
	        multigrid->R[i] = NULL;
	    }
	    PASE_Free(multigrid->R);
	    multigrid->R = NULL;
	}
	if(multigrid->A) {
	    for(i=1; i<multigrid->actual_level; i++) {
	        PASE_Matrix_destroy(multigrid->A[i]);
	        multigrid->A[i] = NULL;
	    }
	    PASE_Free(multigrid->A);
	    multigrid->A = NULL;
	}
	if(multigrid->B) {
	    for(i=1; i<multigrid->actual_level; i++) {
	        PASE_Matrix_destroy(multigrid->B[i]);
	        multigrid->B[i] = NULL;
	    }
	    PASE_Free(multigrid->B);
	    multigrid->B = NULL;
	}    
	if(multigrid->amg_data) {
	    multigrid->ops->destroy_amg_data(multigrid->amg_data);
	    multigrid->amg_data = NULL;
	}
	if(multigrid->ops) {
	    PASE_Multigrid_operator_destroy(multigrid->ops);
	    multigrid->ops = NULL;
	}
	PASE_Free(multigrid);
	multigrid = NULL;
    }        
}            

PASE_MULTIGRID_OPERATOR
PASE_Multigrid_operator_create(void (*get_amg_array)    (void *A, PASE_PARAMETER param, void ***A_array, void ***P_array, void ***R_array, PASE_INT *num_level, void **amg_data),
	                       void (*destroy_amg_data) (void *amg_data))
{
    PASE_MULTIGRID_OPERATOR ops = (PASE_MULTIGRID_OPERATOR) PASE_Malloc(sizeof(PASE_MULTIGRID_OPERATOR_PRIVATE));
    ops->get_amg_array = get_amg_array;
    ops->destroy_amg_data  = destroy_amg_data;
    return ops;
}

             
PASE_MULTIGRID_OPERATOR
PASE_Multigrid_operator_create_by_default(PASE_INT data_struct)
{         
    PASE_MULTIGRID_OPERATOR ops = NULL;
    if(data_struct == 1){
	ops = PASE_Multigrid_operator_create(PASE_Multigrid_get_amg_array_hypre,
		                             PASE_Multigrid_destroy_amg_data_hypre);
    }        
    return ops;
}            

void         
PASE_Multigrid_operator_destroy(PASE_MULTIGRID_OPERATOR ops)
{            
    PASE_Free(ops);
}            
             
void         
PASE_Multigrid_get_amg_array_hypre(void *A, PASE_PARAMETER param, void ***A_array, void ***P_array, void ***R_array, PASE_INT *num_level, void **amg_data)
{            
    HYPRE_Solver amg_solver;
    HYPRE_BoomerAMGCreate(&amg_solver);

    /* Set some parameters (See Reference Manual for more parameters) */
    HYPRE_BoomerAMGSetPrintLevel(amg_solver, 0);         /* print solve info + parameters */
    HYPRE_BoomerAMGSetInterpType(amg_solver, 0 );
    HYPRE_BoomerAMGSetPMaxElmts(amg_solver, 0 );
    HYPRE_BoomerAMGSetCoarsenType(amg_solver, 6);
    HYPRE_BoomerAMGSetMaxLevels(amg_solver, param->max_level);  /* maximum number of levels */
//   HYPRE_BoomerAMGSetRelaxType(amg_solver, 3);          /* G-S/Jacobi hybrid relaxation */
//   HYPRE_BoomerAMGSetRelaxOrder(amg_solver, 1);         /* uses C/F relaxation */
//   HYPRE_BoomerAMGSetNumSweeps(amg_solver, 1);          /* Sweeeps on each level */
//   HYPRE_BoomerAMGSetTol(amg_solver, 1e-7);             /* conv. tolerance */
    HYPRE_BoomerAMGSetMinCoarseSize(amg_solver, param->min_coarse_size);

   /* Now setup */
    HYPRE_ParCSRMatrix parcsr_A     = (HYPRE_ParCSRMatrix)A;
    MPI_Comm           comm         = hypre_ParCSRMatrixComm(parcsr_A);
    PASE_INT           global_size  = hypre_ParCSRMatrixGlobalNumRows(parcsr_A);
    PASE_INT          *partitioning = NULL;
    HYPRE_ParCSRMatrixGetRowPartitioning(parcsr_A, &partitioning);

    HYPRE_ParVector    par_b        = hypre_ParVectorCreate(comm, global_size, partitioning);
    HYPRE_ParVectorInitialize(par_b);
    hypre_ParVectorSetPartitioningOwner(par_b, 1);
    HYPRE_ParVector    par_x        = hypre_ParVectorCreate(comm, global_size, partitioning);
    HYPRE_ParVectorInitialize(par_x);
    hypre_ParVectorSetPartitioningOwner(par_x, 0);

    HYPRE_BoomerAMGSetup(amg_solver, parcsr_A, par_b, par_x);

    hypre_ParAMGData *amg_data_hypre = (hypre_ParAMGData*) amg_solver;

    *num_level = hypre_ParAMGDataNumLevels(amg_data_hypre);
    HYPRE_ParCSRMatrix *A_hypre, *P_hypre;
    A_hypre = hypre_ParAMGDataAArray(amg_data_hypre);
    P_hypre = hypre_ParAMGDataPArray(amg_data_hypre);
    HYPRE_ParCSRMatrix *R_hypre = hypre_CTAlloc(HYPRE_ParCSRMatrix, *num_level-1);

    PASE_INT N_H = hypre_ParCSRMatrixGlobalNumRows(A_hypre[*num_level-1]);
    PASE_INT myid = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    if(myid == 0) {
        printf ( "The number of levels = %d\n", *num_level );
        printf("The dim of the coarsest space is %d.\n", N_H );
    }

    PASE_INT i;
    for(i=0; i<*num_level-1; i++) {
        hypre_ParCSRMatrixTranspose(P_hypre[i], &(R_hypre[i]), 1);
    }
    //HYPRE_ParCSRMatrix *A_copy = hypre_CTAlloc(HYPRE_ParCSRMatrix, *num_level);
    //HYPRE_ParCSRMatrix *P_copy = hypre_CTAlloc(HYPRE_ParCSRMatrix, *num_level-1);
    //HYPRE_ParCSRMatrix *R_copy = hypre_CTAlloc(HYPRE_ParCSRMatrix, *num_level-1);
    //A_copy[0] = parcsr_A; 
    //for(i=1; i<*num_level; i++) {
    //    A_copy[i] = hypre_ParCSRMatrixCompleteClone(A_hypre[i]);
    //    hypre_ParCSRMatrixCopy(A_hypre[i], A_copy[i], 1);
    //    P_copy[i-1] = hypre_ParCSRMatrixCompleteClone(P_hypre[i-1]);
    //    hypre_ParCSRMatrixCopy(P_hypre[i-1], P_copy[i-1], 1);
    //    hypre_ParCSRMatrixTranspose(P_hypre[i-1], &(R_copy[i-1]), 1);
    //}
    *A_array = (void**)A_hypre;
    *P_array = (void**)P_hypre;
    *R_array = (void**)R_hypre;
    *amg_data = (void*)amg_solver;
    
    HYPRE_ParVectorDestroy(par_b);
    HYPRE_ParVectorDestroy(par_x);
    //printf("The num_row of R[0] is %d, num_col of R[0] is %d\n", R_copy[0]->global_num_rows, R_copy[0]->global_num_cols);
    //printf("The num_row of P[0] is %d, num_col of P[0] is %d\n", P_copy[0]->global_num_rows, P_copy[0]->global_num_cols);
    //HYPRE_BoomerAMGDestroy(amg_solver);
}            
             
void
PASE_Multigrid_destroy_amg_data_hypre(void *amg_data)
{
    HYPRE_BoomerAMGDestroy((HYPRE_Solver)amg_data);
}
             
             
             
