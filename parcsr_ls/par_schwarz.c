/*BHEADER**********************************************************************
 * (c) 1996   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision$
 *********************************************************************EHEADER*/

/******************************************************************************
 *
 * Schwarz functions
 *
 *****************************************************************************/

#include "headers.h"
#include "schwarz.h"

/*--------------------------------------------------------------------------
 * hypre_SchwarzCreate
 *--------------------------------------------------------------------------*/

void *
hypre_SchwarzCreate()
{
   hypre_SchwarzData *schwarz_data;

   int      variant;
   int      domain_type;
   int      overlap;
   int 	    num_functions;
   double   relax_weight;

   /*-----------------------------------------------------------------------
    * Setup default values for parameters
    *-----------------------------------------------------------------------*/

   /* setup params */
   variant = 0;  /* multiplicative Schwarz */
   overlap = 1;  /* minimal overlap */
   domain_type = 2; /* domains generated by agglomeration */
   num_functions = 1;
   relax_weight = 1.0;

   schwarz_data = hypre_CTAlloc(hypre_SchwarzData,1);

   hypre_SchwarzSetVariant(schwarz_data, variant);
   hypre_SchwarzSetDomainType(schwarz_data, domain_type);
   hypre_SchwarzSetOverlap(schwarz_data, overlap);
   hypre_SchwarzSetNumFunctions(schwarz_data, num_functions);
   hypre_SchwarzSetRelaxWeight(schwarz_data, relax_weight);

   hypre_SchwarzDataDomainStructure(schwarz_data) = NULL;
   hypre_SchwarzDataScale(schwarz_data) = NULL;
   hypre_SchwarzDataVtemp(schwarz_data) = NULL;
   hypre_SchwarzDataDofFunc(schwarz_data) = NULL;

   return (void *) schwarz_data;
}

/*--------------------------------------------------------------------------
 * hypre_SchwarzDestroy
 *--------------------------------------------------------------------------*/

int
hypre_SchwarzDestroy( void *data )
{
   int ierr = 0;
   hypre_SchwarzData  *schwarz_data = data;

   if (hypre_SchwarzDataScale(schwarz_data))
      hypre_TFree (hypre_SchwarzDataScale(schwarz_data));
   if (hypre_SchwarzDataDofFunc(schwarz_data))
      hypre_TFree (hypre_SchwarzDataDofFunc(schwarz_data));
   hypre_CSRMatrixDestroy(hypre_SchwarzDataDomainStructure(schwarz_data));
   hypre_ParVectorDestroy(hypre_SchwarzDataVtemp(schwarz_data));

   hypre_TFree(schwarz_data);
   return (ierr);
}

int
hypre_SchwarzSetup(void               *schwarz_vdata,
                   hypre_ParCSRMatrix *A,
                   hypre_ParVector    *f,
                   hypre_ParVector    *u         )
{
   int ierr = 0;
   hypre_SchwarzData   *schwarz_data = schwarz_vdata;
   int *dof_func;
   double *scale;
   hypre_CSRMatrix *domain_structure;
   hypre_ParVector *Vtemp;

   int variant = hypre_SchwarzDataVariant(schwarz_data);
   int domain_type = hypre_SchwarzDataDomainType(schwarz_data);
   int overlap = hypre_SchwarzDataOverlap(schwarz_data);
   int num_functions = hypre_SchwarzDataNumFunctions(schwarz_data);
   double relax_weight = hypre_SchwarzDataRelaxWeight(schwarz_data);

   dof_func = hypre_SchwarzDataDofFunc(schwarz_data);

   Vtemp = hypre_ParVectorCreate(hypre_ParCSRMatrixComm(A),
			hypre_ParCSRMatrixGlobalNumRows(A),
			hypre_ParCSRMatrixRowStarts(A));
   hypre_ParVectorSetPartitioningOwner(Vtemp,0);
   hypre_ParVectorInitialize(Vtemp);
   hypre_SchwarzDataVtemp(schwarz_data) = Vtemp;

   if (variant)
   {
      hypre_ParAMGCreateDomainDof(A,
			     domain_type, overlap,
			     num_functions, dof_func,
			     &domain_structure);

      hypre_ParGenerateScale(A, domain_structure, relax_weight,
	&scale);
      hypre_SchwarzDataScale(schwarz_data) = scale;
   }
   else
   {
      hypre_AMGCreateDomainDof (hypre_ParCSRMatrixDiag(A),
			     domain_type, overlap,
			     num_functions, dof_func,
			     &domain_structure);
   }

   hypre_SchwarzDataDomainStructure(schwarz_data) = domain_structure;

   return ierr;
}

/*--------------------------------------------------------------------
 * hypre_SchwarzSolve
 *--------------------------------------------------------------------*/

int
hypre_SchwarzSolve(void               *schwarz_vdata,
                   hypre_ParCSRMatrix *A,
                   hypre_ParVector    *f,
                   hypre_ParVector    *u         )
{
   hypre_SchwarzData   *schwarz_data = schwarz_vdata;
   int ierr = 0;
   hypre_CSRMatrix *domain_structure = 
		hypre_SchwarzDataDomainStructure(schwarz_data);
   double *scale = hypre_SchwarzDataScale(schwarz_data);
   hypre_ParVector *Vtemp = hypre_SchwarzDataVtemp(schwarz_data);
   int variant = hypre_SchwarzDataVariant(schwarz_data);
   double relax_wt = hypre_SchwarzDataRelaxWeight(schwarz_data);

   if (variant)
   {
      ierr = hypre_ParAdSchwarzSolve(A, f, domain_structure, scale, u, Vtemp);
   }
   else
   {
      ierr = hypre_MPSchwarzSolve(A, hypre_ParVectorLocalVector(f), 
				domain_structure, u, relax_wt, 
				hypre_ParVectorLocalVector(Vtemp));
   }
      
   return ierr;
}

/*--------------------------------------------------------------------------
 * Routines to set various parameters
 *--------------------------------------------------------------------------*/

int
hypre_SchwarzSetVariant( void *data, int variant )
{
   int ierr = 0;
   hypre_SchwarzData  *schwarz_data = data;
 
   hypre_SchwarzDataVariant(schwarz_data) = variant;

   return (ierr);
}

int
hypre_SchwarzSetDomainType( void *data, int domain_type )
{
   int ierr = 0;
   hypre_SchwarzData  *schwarz_data = data;
 
   hypre_SchwarzDataDomainType(schwarz_data) = domain_type;

   return (ierr);
}

int
hypre_SchwarzSetOverlap( void *data, int overlap )
{
   int ierr = 0;
   hypre_SchwarzData  *schwarz_data = data;
 
   hypre_SchwarzDataOverlap(schwarz_data) = overlap;

   return (ierr);
}

int
hypre_SchwarzSetNumFunctions( void *data, int num_functions )
{
   int ierr = 0;
   hypre_SchwarzData  *schwarz_data = data;
 
   hypre_SchwarzDataNumFunctions(schwarz_data) = num_functions;

   return (ierr);
}

int
hypre_SchwarzSetRelaxWeight( void *data, double relax_weight )
{
   int ierr = 0;
   hypre_SchwarzData  *schwarz_data = data;
 
   hypre_SchwarzDataRelaxWeight(schwarz_data) = relax_weight;

   return (ierr);
}

int
hypre_SchwarzSetDomainStructure( void *data, hypre_CSRMatrix *domain_structure )
{
   int ierr = 0;
   hypre_SchwarzData  *schwarz_data = data;
 
   hypre_SchwarzDataDomainStructure(schwarz_data) = domain_structure;

   return (ierr);
}

int
hypre_SchwarzSetScale( void *data, double *scale)
{
   int ierr = 0;
   hypre_SchwarzData  *schwarz_data = data;
 
   hypre_SchwarzDataScale(schwarz_data) = scale;

   return (ierr);
}

int
hypre_SchwarzReScale( void *data, int size, double value)
{
   int ierr = 0;
   int i;
   double *scale;
   hypre_SchwarzData  *schwarz_data = data;

   scale = hypre_SchwarzDataScale(schwarz_data);
   for (i=0; i < size; i++)
      scale[i] *= value;

   return (ierr);
}

int
hypre_SchwarzSetDofFunc( void *data, int *dof_func)
{
   int ierr = 0;
   hypre_SchwarzData  *schwarz_data = data;
 
   hypre_SchwarzDataDofFunc(schwarz_data) = dof_func;

   return (ierr);
}
