/*
 * NEEMP - discard.c
 *
 * by Tomas Racek (tom@krab1k.net)
 * 2013
 *
 * */

#include <stdio.h>
#include <stdlib.h>

#include "discard.h"
#include "kappa.h"
#include "subset.h"
#include "structures.h"

extern struct training_set ts;

struct subset *discard_iterative(const struct subset * const initial) {

	struct subset *current, *old;

	/* We guarantee that we won't free subset pointed by intial */
	old = (struct subset *) initial;
	current = old;

	for(int i = 0; i < 3; i++) {
		current = (struct subset *) calloc(1, sizeof(struct subset));
		current->parent = old;

		/* Flip just one molecule from the parent */
		b_init(&current->molecules, ts.molecules_count);
		b_set_as(&current->molecules, &old->molecules);
		int bno = rand() % ts.molecules_count;
		b_flip(&current->molecules, bno);

		fprintf(stderr, "Iteration no. %d. Molecule discarded: %s Molecules used: %d\n",\
			i, ts.molecules[bno].name, b_count_bits(&current->molecules));

		find_the_best_parameters_for_subset(current);
		fprintf(stderr, "R = %6.4f\n", current->best->R);

		/* Check if the new subset is better than the old one */
		if(kd_sort_by_is_better(current->best, old->best)) {

			fprintf(stderr, "We found better solution.\n");
			/* Do not free initial subset! */
			if(old != initial) {
				ss_destroy(old);
				free(old);
			}

			old = current;
		}
		else {
			fprintf(stderr, "We didn't find better solution.\n");
			ss_destroy(current);
			free(current);

			/* If this is the last iteration, we want to return the best result so far */
			current = old;
		}
	}

	return current;
}

struct subset *discard_simple(const struct subset * const initial) {

	struct subset *best = (struct subset *) initial;
	struct subset *current;

	for(int i = 0; i < ts.molecules_count && i < 3; i++) {

		current = (struct subset *) calloc(1, sizeof(struct subset));
		current->parent = best;

		b_init(&current->molecules, ts.molecules_count);
		b_set_as(&current->molecules, &best->molecules);
		b_flip(&current->molecules, i);

		fprintf(stderr, "Iteration no. %d. Molecule discarded: %s Molecules used: %d\n",\
			i, ts.molecules[i].name, b_count_bits(&current->molecules));

		find_the_best_parameters_for_subset(current);

		if(kd_sort_by_is_better(current->best, best->best)) {
			fprintf(stderr, "We found better solution.\n");

			if(best != initial) {
				ss_destroy(best);
				free(best);
			}

			best = current;
		}
		else {
			fprintf(stderr, "We didn't find better solution.\n");
			ss_destroy(current);
			free(current);
		}
	}

	return best;
}
