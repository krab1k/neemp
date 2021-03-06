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

#ifndef __SUBSET_H__
#define __SUBSET_H__

#include "bitarray.h"

struct stats {

	float R;
	float R2;
	float R_w;
	float spearman;
	float RMSD;
	float RMSD_avg;
	float D_avg;
	float D_max;
	float cond;
};

struct kappa_data {

	const struct subset *parent_subset;

	float kappa;

	float *charges;

	float *parameters_alpha;
	float *parameters_beta;

	struct stats full_stats;
	struct stats *per_at_stats;
	struct stats *per_molecule_stats;
};

void kd_init(struct kappa_data * const kd);
void kd_copy_parameters(struct kappa_data* from, struct kappa_data* to);
void kd_copy_statistics(struct kappa_data* from, struct kappa_data* to);
void kd_destroy(struct kappa_data * const kd);
void kd_print_stats(const struct kappa_data * const kd);
void kd_print_results(const struct kappa_data * const kd);

float kd_sort_by_return_value(const struct kappa_data * const kd);
float kd_sort_by_return_value_per_atom(const struct kappa_data * const kd, int i);
int kd_sort_by_is_better(const struct kappa_data * const kd1, const struct kappa_data * const kd2);
void kd_sort_by_is_much_better_per_atom(int* results_per_atom, const struct kappa_data * const kd1, const struct kappa_data * const kd2, float threshold);
int kd_sort_by_is_better_per_atom(const struct kappa_data * const kd1, const struct kappa_data * const kd2, int idx);

struct subset {

	struct bit_array molecules;

	int kappa_data_count;
	struct kappa_data *data;

	/* Pointer to the best kappa */
	struct kappa_data *best;

	const struct subset *parent;
};

void ss_init(struct subset * const ss, const struct subset * const parent);
void fill_ss(struct subset * const ss, int size);
void ss_destroy(struct subset * const ss);
void print_results(const struct subset * const ss);
void print_parameters(const struct kappa_data * const kd);
#endif /* __SUBSET_H__ */
