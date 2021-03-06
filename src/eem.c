/* Copyright 2013-2016 Tomas Racek (tom@krab1k.net)
 *
 * This file is part of NEEMP.
 *
 * NEEMP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NEEMP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NEEMP. If not, see <http://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <omp.h>

#ifdef USE_MKL
#include <mkl.h>
#else
extern void dspsvx_(char *fact, char *uplo, int *n, int *nrhs, const double *ap, double *afp, int *ipiv, const double *b, int *ldb, double *x, int *ldx, double *rcond, double *ferr, double *berr, double *work, int *iwork, int *info);
extern void dspsv_(char *uplo, int *n, int *nrhs, double *ap, int *ipiv, double *b, int *ldb, int *info);
#endif /* USE_MKL */

#include "eem.h"
#include "neemp.h"
#include "settings.h"
#include "subset.h"
#include "structures.h"

extern const struct training_set ts;
extern const struct settings s;

static int check_matrix_packed(const double * const A, const int n);
static void fill_EEM_matrix_packed(double * const A, const struct molecule * const m, const struct kappa_data * const kd);

#ifdef NOT_USED
static void print_matrix_packed(const double * const A, long int n);
static void print_matrix_full(const double * const A, long int n);

/* Print matrix in packed storage format */
static void print_matrix_packed(const double * const A, long int n) {

	assert(A != NULL);

	#define U_IDX(x, y) (x + (y * (y + 1))/2)
	for(long int i = 0; i < n; i++) {
		for(long int j = 0; j < i; j++)
			printf("       ");
		for(long int j = i; j < n; j++)
			printf("%6.4f ", A[U_IDX(i, j)]);

		printf("\n");
	}
	#undef U_IDX
}

/* Print matrix in packed storage format */
static void print_matrix_full(const double * const A, long int n) {

	assert(A != NULL);

	#define IDX(x, y) (x * n + y)
	for(long int i = 0; i < n; i++) {
		for(long int j = 0; j < n; j++)
			fprintf(stderr, "%6.3f ", A[IDX(i, j)]);

		fprintf(stderr, "\n");
	}
	#undef IDX
}

#endif /* NOT_USED */

/* Use packed storage scheme to fill EEM matrix */
static void fill_EEM_matrix_packed(double * const A, const struct molecule * const m, const struct kappa_data * const kd) {

	assert(A != NULL);
	assert(m != NULL);
	assert(kd != NULL);

	const long int n = m->atoms_count;

	/* Following #define works only for i <= j */
	#define U_IDX(x, y) (x + (y * (y + 1))/2)

	/* Fill the upper half of the n * n block */
	for(long int i = 0; i < n; i++) {
		A[U_IDX(i, i)] = kd->parameters_beta[get_atom_type_idx(&m->atoms[i])];
		for(long int j = i + 1; j < n; j++) {
			if(s.mode == MODE_PARAMS)
				A[U_IDX(i, j)] = kd->kappa * m->atoms[i].rdists[j];
			else
				A[U_IDX(i, j)] = kd->kappa * rdist(&m->atoms[i], &m->atoms[j]);
		}
	}

	/* Fill last column */
	for(long int i = 0; i < n; i++)
		A[U_IDX(i, n)] = 1.0f;

	/* Set the bottom right element to zero */
	A[U_IDX(n, n)] = 0.0f;
	#undef U_IDX
}

/* Check if matrix contains some NaN or Inf values */
static int check_matrix_packed(const double * const A, const int n) {

	long int elements_count = (n + 1) * (n + 2) / 2;

	for(long int i = 0; i < elements_count; i++)
		if(!isfinite(A[i]))
			return 1;

	return 0;
}

/* Calculate charges for a particular kappa_data structure */
void calculate_charges(struct subset * const ss, struct kappa_data * const kd) {

	assert(ss != NULL);
	assert(kd != NULL);

	/* Compute starting indices for storing the charges of each molecule. These are
	 * needed to guarantee the independence of the for loop iterations */
	int starts[ts.molecules_count];
	starts[0] = 0;
	for(int i = 1; i < ts.molecules_count; i++)
		starts[i] = starts[i - 1] + ts.molecules[i - 1].atoms_count;
	int nt = s.max_threads;
	if (s.params_method == PARAMS_DE || s.params_method == PARAMS_GM)
		nt /= s.om_threads;

	int nthreads = ts.molecules_count < nt ? ts.molecules_count : nt;
	#pragma omp parallel for num_threads(nthreads)
	for(int i = 0; i < ts.molecules_count; i++) {
		#define MOLECULE ts.molecules[i]
		const int n = MOLECULE.atoms_count;

		void *tmp1 = NULL;
		void *tmp2 = NULL;
		posix_memalign(&tmp1, 64, ((n + 1) * (n + 2)) / 2 * sizeof(double));
		posix_memalign(&tmp2, 64, (n + 1) * sizeof(double));
		double *Ap = (double *) tmp1;
		double *b = (double *) tmp2;
		if(!Ap || !b)
			EXIT_ERROR(MEM_ERROR, "%s", "Cannot allocate memory for EEM system.\n");

		fill_EEM_matrix_packed(Ap, &ts.molecules[i], kd);

		/* Fill vector b */
		for(int j = 0; j < n; j++)
			b[j] = - kd->parameters_alpha[get_atom_type_idx(&MOLECULE.atoms[j])];

		b[n] = MOLECULE.sum_of_charges;

		/* Solve EEM system */

		double rcond;
		int ipiv[n + 1];
		char uplo = 'U';
		int nn = n + 1;
		int nrhs = 1;
		int ldb = n + 1;

		double *Afp = NULL;
		double *x = NULL;

		if(check_matrix_packed(Ap, n) && s.mode == MODE_CHARGES) {
			fprintf(stderr, "Invalid EEM system for molecule %s. Setting charges to NaN.\n", MOLECULE.name);
			for(int j = 0; j < n; j++)
				kd->charges[starts[i] + j] = (float) 0.0 / 0.0;

			goto out;
		}

		if(s.extra_precise) {
			char fact = 'N';
			double ferr, berr;
			int ldx = nn;

			tmp1 = NULL;
			tmp2 = NULL;
			posix_memalign(&tmp1, 64, ((n + 1) * (n + 2)) / 2 * sizeof(double));
			posix_memalign(&tmp2, 64, (n + 1) * sizeof(double));
			Afp = (double *) tmp1;
			x = (double *) tmp2;

			if(!Afp || !x)
				EXIT_ERROR(MEM_ERROR, "%s", "Cannot allocate memory for EEM system.\n");

			int info;
			#ifdef USE_MKL
			info = LAPACKE_dspsvx(LAPACK_COL_MAJOR, fact, uplo, nn, nrhs, Ap, Afp, ipiv, b, ldb, x, ldx, &rcond, &ferr, &berr);
			#else
			int iwork[n + 1];
			double work[3 * (n + 1)];

			dspsvx_(&fact, &uplo, &nn, &nrhs, Ap, Afp, ipiv, b, &ldb, x, &ldx, &rcond, &ferr, &berr, work, iwork, &info);
			#endif /* USE_MKL */

			kd->per_molecule_stats[i].cond = (float) (1.0 / rcond);

			if(s.mode == MODE_CHARGES && (1 / rcond) > WARN_MAX_COND)
				fprintf(stderr, "Ill-conditioned EEM system for molecule %s. Charges might be inaccurate.\n", MOLECULE.name);


			if(info) {
				fprintf(stderr, "Cannot solve EEM system for molecule %s. Setting charges to NaN.\n", MOLECULE.name);
				for(int j = 0; j < n; j++)
					kd->charges[starts[i] + j] = (float) 0.0 / 0.0;
			} else {
				/* Store computed charges */
				for(int j = 0; j < n; j++)
					kd->charges[starts[i] + j] = (float) x[j];
			}

			free(Afp);
			free(x);
		} else {
			int info;
			#ifdef USE_MKL
			info = LAPACKE_dspsv(LAPACK_COL_MAJOR, uplo, nn, nrhs, Ap, ipiv, b, nn);
			#else
			dspsv_(&uplo, &nn, &nrhs, Ap, ipiv, b, &nn, &info);
			#endif /* USE_MKL */

			if(info) {
				fprintf(stderr, "Cannot solve EEM system for molecule %s. Setting charges to NaN.\n", MOLECULE.name);
				for(int j = 0; j < n; j++)
					kd->charges[starts[i] + j] = (float) 0.0 / 0.0;
			} else {
				/* Store computed charges */
				for(int j = 0; j < n; j++)
					kd->charges[starts[i] + j] = (float) b[j];
			}

			kd->per_molecule_stats[i].cond = 0.0f;
		}

		#undef MOLECULE

		out:
		/* Clean remaining things up */
		free(Ap);
		free(b);
	}
}
