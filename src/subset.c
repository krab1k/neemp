/*
 * NEEMP - subset.c
 *
 * by Tomas Racek (tom@krab1k.net)
 * 2013, 2014
 *
 * */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "bitarray.h"
#include "neemp.h"
#include "settings.h"
#include "structures.h"
#include "subset.h"

extern const struct settings s;
extern const struct training_set ts;

/* Allocate memory for the contents of kappa_data structure */
void kd_init(struct kappa_data * const kd) {

	assert(kd != NULL);

	kd->parameters_alpha = (float *) calloc(ts.atom_types_count, sizeof(float));
	kd->parameters_beta = (float *) calloc(ts.atom_types_count, sizeof(float));
	if(!kd->parameters_alpha || !kd->parameters_beta)
		EXIT_ERROR(MEM_ERROR, "%s", "Cannot allocate memory for parameters array.\n");

	kd->charges = (float *) malloc(ts.atoms_count * sizeof(float));
	if(!kd->charges)
		EXIT_ERROR(MEM_ERROR, "%s", "Cannot allocate memory for charges array.\n");

	kd->per_at_stats = (struct stats *) calloc(ts.atom_types_count, sizeof(struct stats));
	kd->per_molecule_stats = (struct stats *) calloc(ts.molecules_count, sizeof(struct stats));
}

/* Destroy contents of the kappa_data structure */
void kd_destroy(struct kappa_data * const kd) {

	assert(kd != NULL);

	free(kd->parameters_alpha);
	free(kd->parameters_beta);
	free(kd->charges);
	free(kd->per_at_stats);
	free(kd->per_molecule_stats);
}

/* Destroy contents of the subset */
void ss_destroy(struct subset * const ss) {

	assert(ss != NULL);

	for(int i = 0; i < ss->kappa_data_count; i++)
		kd_destroy(&ss->data[i]);

	b_destroy(&ss->molecules);
	free(ss->data);
}

/* Print loaded parameters */
void print_parameters(const struct kappa_data * const kd) {

	assert(kd != NULL);

	printf("\nLoaded parameters:\n");
	printf("K: %6.4f\n", kd->kappa);
	printf("Atom type             A       B\n");
	for(int i = 0; i < ts.atom_types_count; i++) {
		char buff[10];
		at_format_text(&ts.atom_types[i], buff);
		printf(" %s   \t%7.4f\t%7.4f\n", buff, kd->parameters_alpha[i], kd->parameters_beta[i]);
	}

	printf("\n");
}

/* Prints the parameters and associated stats */
void print_results(const struct subset * const ss) {

	assert(ss != NULL);
	assert(ss->best != NULL);

	printf("\nUsed molecules: %5d\n", b_count_bits(&ss->molecules));
	kd_print_stats(ss->best);

	printf("Atom type             A       B\t\t     R\t    RMSD       MSE\t  D_avg\t  D_max\n");
	for(int i = 0; i < ts.atom_types_count; i++) {
		char buff[10];
		at_format_text(&ts.atom_types[i], buff);
		printf(" %s   \t%7.4f\t%7.4f\t\t%6.4f  %4.2e  %4.2e\t%7.3f\t%7.3f\n",
			buff, ss->best->parameters_alpha[i], ss->best->parameters_beta[i],
			ss->best->per_at_stats[i].R, ss->best->per_at_stats[i].RMSD, ss->best->per_at_stats[i].MSE,
			ss->best->per_at_stats[i].D_avg, ss->best->per_at_stats[i].D_max);
	}

	printf("\n");
}

/* Return the value which we selected for sorting */
float kd_sort_by_return_value(const struct kappa_data * const kd) {

	switch(s.sort_by) {
		case SORT_R:
			return kd->full_stats.R;
		case SORT_RMSD:
			return kd->full_stats.RMSD;
		case SORT_MSE:
			return kd->full_stats.MSE;
		case SORT_D_AVG:
			return kd->full_stats.D_avg;
		case SORT_D_MAX:
			return kd->full_stats.D_max;
		default:
			/* Something bad happened */
			assert(0);
	}
}

/* Determine if kd1 is better than kd2 in terms of the sort-by value */
int kd_sort_by_is_better(const struct kappa_data * const kd1, const struct kappa_data * const kd2) {

	if(s.sort_by == SORT_R)
		return kd1->full_stats.R > kd2->full_stats.R;
	else
		return kd_sort_by_return_value(kd1) < kd_sort_by_return_value(kd2);
}

/* Print all the statistics for the particular kappa data */
void kd_print_stats(const struct kappa_data * const kd) {

	/* Allocate buffer large enough to hold the entire message */
	char message[100];
	memset(message, 0, 100 * sizeof(char));

	snprintf(message, 100, "K: %6.4f |  R: %6.4f  RMSD: %6.4f  MSE: %6.4f  D_avg: %6.4f  D_max: %6.4f\n",
		kd->kappa, kd->full_stats.R, kd->full_stats.RMSD, kd->full_stats.MSE, kd->full_stats.D_avg, kd->full_stats.D_max);

	/* Print '*' at the position of the stat which is used for sorting */
	switch(s.sort_by) {
		case SORT_R:
			message[12] = '*';
			break;
		case SORT_RMSD:
			message[23] = '*';
			break;
		case SORT_MSE:
			message[39] = '*';
			break;
		case SORT_D_AVG:
			message[54] = '*';
			break;
		case SORT_D_MAX:
			message[71] = '*';
			break;
		default:
			/* Something bad happened */
			assert(0);
	}

	printf("%s", message);
}
