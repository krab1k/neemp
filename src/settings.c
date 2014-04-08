/*
 * NEEMP - settings.c
 *
 * by Tomas Racek (tom@krab1k.net)
 * 2013, 2014
 *
 * */

#include <assert.h>
#include <getopt.h>
#include <string.h>

#include "limits.h"
#include "neemp.h"
#include "settings.h"

extern struct settings s;

static void print_help(void);
static void print_version(void);

static struct option long_options[] = {

	{"help", no_argument, 0, 'h'},
	{"mode", required_argument, 0, 'm'},
	{"fs-only", no_argument, 0, 'f'},
	{"verbose", no_argument, 0, 'v'},
	{"discard", required_argument, 0, 'd'},
	{"sort-by", required_argument, 0, 's'},
	{"version", no_argument, 0, 5},
	{"sdf-file", required_argument, 0, 10},
	{"par-file", required_argument, 0, 11},
	{"chg-file", required_argument, 0, 12},
	{"chg-out-file", required_argument, 0, 13},
	{"chg-stats-out-file", required_argument, 0, 14},
	{"par-out-file", required_argument, 0, 15},
	{"kappa-max", required_argument, 0, 20},
	{"kappa", required_argument, 0, 21},
	{"kappa-preset", required_argument, 0, 22},
	{"fs-precision", required_argument, 0, 23},
	{"atom-types-by", required_argument, 0, 31},
	{"tabu-size", required_argument, 0, 32},
	{"limit-iters", required_argument, 0, 40},
	{"limit-time", required_argument, 0, 41},
	{"check-charges", no_argument, 0, 50},
	{NULL, 0, 0, 0}
};

/* Initialize default settings */
void s_init(void) {

	memset(s.sdf_file, 0x0, MAX_PATH_LEN * sizeof(char));
	memset(s.chg_file, 0x0, MAX_PATH_LEN * sizeof(char));
	memset(s.par_file, 0x0, MAX_PATH_LEN * sizeof(char));
	memset(s.par_out_file, 0x0, MAX_PATH_LEN * sizeof(char));
	memset(s.chg_out_file, 0x0, MAX_PATH_LEN * sizeof(char));
	memset(s.chg_stats_out_file, 0x0, MAX_PATH_LEN * sizeof(char));

	s.mode = MODE_NOT_SET;
	s.full_scan_only = 0;
	s.full_scan_precision = 0.0f;
	s.kappa_max = 0.0f;
	s.full_scan_precision = 0.0f;
	s.sort_by = SORT_R;
	s.at_customization = AT_CUSTOM_ELEMENT_BOND;
	s.discard = DISCARD_OFF;
	s.sort_by = SORT_R;
	s.tabu_size = 0.0f;
	s.limit_iters = NO_LIMIT_ITERS;
	s.limit_time = NO_LIMIT_TIME;
	s.check_charges = 0;
}

/* Prints help if --version is issued */
static void print_version(void) {

	printf("%s %s\n", APP_NAME, APP_VERSION);
	printf("Copyright (C) 2013, 2014 Tomas Racek (tom@krab1k.net)\n");
	printf("Licence MIT: http://opensource.org/licenses/MIT\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
}

/* Prints help if -h/--help is issued */
static void print_help(void) {

	printf("Usage: neemp [OPTION] ...\n");
	printf("Perform EEM (Electronegativity Equalization Method) parameterization. Compute atomic charges using EEM.\n");

	printf("\nOptions:\n");
	printf("  -h, --help			 display this help and exit\n");
	printf("      --version			 display version information and exit\n");
	printf("  -m, --mode MODE		 set mode for the NEEMP. Valid choices are: info, params, charges, cross. (required)\n");
	printf("      --sdf-file FILE		 SDF file (required)\n");
	printf("      --atom-types-by METHOD	 classify atoms according to the METHOD. Valid choices are: element, element_bond.\n");
	printf("Options specific to mode: params\n");
	printf("      --chg-file FILE		 FILE with ab-initio charges (required)\n");
	printf("      --chg-stats-out-file FILE	 output charges statistics to the FILE\n");
	printf("      --kappa-max MAX            set maximum value for kappa (required)\n");
	printf("      --kappa VALUE              use only one kappa VALUE for parameterization\n");
	printf("      --fs-precision VALUE       resolution for the full scan (required)\n");
	printf("      --kappa-preset PRESET      set kappa-max and fs-precision to safe values. Valid choices are: small, protein.\n");
	printf("  -f, --fs-only                  do not use additional accuracy improvement\n");
	printf("      --par-out-file FILE        output the parameters to the FILE\n");
	printf("  -d, --discard METHOD           perform discarding with METHOD. Valid choices are: iterative, simple and off. Default is off.\n");
	printf("  -s, --sort-by STAT             sort solutions by STAT. Valid choices are: R, RMSD, MSE, D_max, D_avg.\n");
	printf("      --limit-iters COUNT        set the maximum number of iterations for discarding.\n");
	printf("      --limit-time HH:MM:SS      set the maximum time for discarding in format hours:minutes:seconds.\n");
	printf("      --check-charges      	 warn about molecules with abnormal differences between QM and EEM charges.\n");
	printf("Options specific to mode: charges\n");
	printf("      --par-file FILE		 FILE with EEM parameters (required)\n");
	printf("      --chg-out-file FILE	 Output charges to the FILE (required)\n");

	printf("\nExamples:\n");
	printf("neemp -m info --sdf-file molecules.sdf --atom-types-by element\n\
		Display information about the training set in the file molecules.sdf. Group atoms according to the elements only.\n");

	printf("neemp -m params --sdf-file molecules.sdf --chg-file charges.chg --kappa-max 1.0 --fs-precision 0.2 --sort-by RMSD --fs-only.\n\
		Compute parameters for the given molecules in file molecules.sdf and ab-initio charges in charges.chg. Set maximum value for kappa to 1.0, step for the full scan to 0.2, no iterative refinement, sort results according to the relative mean square deviation.\n");

	printf("neemp -m charges --sdf-file molecules.sdf --par-file parameters --chg-out-file output.chg\n\
		Calculate and store EEM charges to the file output.chg\n");
}
/* Parse command line options */
void parse_options(int argc, char **argv) {

	int c;
	int option_idx;

	while((c = getopt_long(argc, argv, "fvd:s:hm:", long_options, &option_idx)) != -1)
	{

		switch(c) {
			case 'f':
				s.full_scan_only = 1;
				break;

			case 'h':
				print_help();
				exit(RETURN_OK);

			case 'm': /* mode */
				if(!strcmp(optarg, "info"))
					s.mode = MODE_INFO;
				else if (!strcmp(optarg, "charges"))
					s.mode = MODE_CHARGES;
				else if (!strcmp(optarg, "params"))
					s.mode = MODE_PARAMS;
				else if (!strcmp(optarg, "cross"))
					s.mode = MODE_CROSS;
				else
					EXIT_ERROR(ARG_ERROR, "Invalid mode: %s\n", optarg);
				break;

			case 'v':
				s.verbosity++;
				break;

			case 'd': /* discard */
				if(!strcmp(optarg, "off"))
					s.discard = DISCARD_OFF;
				else if(!strcmp(optarg, "iterative"))
					s.discard = DISCARD_ITER;
				else if(!strcmp(optarg, "simple"))
					s.discard = DISCARD_SIMPLE;
				else
					EXIT_ERROR(ARG_ERROR, "Invalid discarding mode: %s\n", optarg);
				break;

			case 's': /* sort-by */
				if(!strcmp(optarg, "R"))
					s.sort_by = SORT_R;
				else if(!strcmp(optarg, "RMSD"))
					s.sort_by = SORT_RMSD;
				else if(!strcmp(optarg, "MSE"))
					s.sort_by = SORT_MSE;
				else if(!strcmp(optarg, "D_avg"))
					s.sort_by = SORT_D_AVG;
				else if(!strcmp(optarg, "D_max"))
					s.sort_by = SORT_D_MAX;
				else
					EXIT_ERROR(ARG_ERROR, "Invalid sort-by value: %s\n", optarg);
				break;

			case 5:
				print_version();
				exit(RETURN_OK);

			case 10:
				strncpy(s.sdf_file, optarg, MAX_PATH_LEN - 1);
				break;

			case 11:
				strncpy(s.par_file, optarg, MAX_PATH_LEN - 1);
				break;

			case 12:
				strncpy(s.chg_file, optarg, MAX_PATH_LEN - 1);
				break;

			case 13:
				strncpy(s.chg_out_file, optarg, MAX_PATH_LEN - 1);
				break;

			case 14:
				strncpy(s.chg_stats_out_file, optarg, MAX_PATH_LEN - 1);
				break;

			case 15:
				strncpy(s.par_out_file, optarg, MAX_PATH_LEN - 1);
				break;

			case 20:
				s.kappa_max = (float) atof(optarg);
				break;

			case 21:
				s.kappa_set = (float) atof(optarg);
				break;
			case 22:
				if(!strcmp(optarg, "small")) {
					s.kappa_max = 1.5f;
					s.full_scan_precision = 0.001f;
				}
				else if(!strcmp(optarg, "protein")) {
					s.kappa_max = 0.01f;
					s.full_scan_precision = 0.0001f;
				}
				else
					EXIT_ERROR(ARG_ERROR, "Invalid kappa-preset value: %s\n", optarg);
				break;
			case 23:
				s.full_scan_precision = (float) atof(optarg);
				break;

			case 31: /* at-customization */
				if(!strcmp(optarg, "element"))
					s.at_customization = AT_CUSTOM_ELEMENT;
				else if(!strcmp(optarg, "element_bond"))
					s.at_customization = AT_CUSTOM_ELEMENT_BOND;
				else if(!strcmp(optarg, "partner"))
					s.at_customization = AT_CUSTOM_PARTNER;
				else if(!strcmp(optarg, "valence"))
					s.at_customization = AT_CUSTOM_VALENCE;
				else
					EXIT_ERROR(ARG_ERROR, "Invalid atom-type-by value: %s\n", optarg);
				break;

			case 32:
				s.tabu_size = (float) atof(optarg);
				break;
			case 40:
				s.limit_iters =  atoi(optarg);
				break;
			case 41: {
					char *part;
					int hours = 0;
					int mins = 0;
					int secs = 0;

					part = strtok(optarg, ":");
					if(part != NULL)
						hours = atoi(part);
					part = strtok(NULL, ":");
					if(part != NULL)
						mins = atoi(part);
					part = strtok(NULL, ":");
					if(part != NULL)
						secs = atoi(part);

					s.limit_time = 3600 * hours + 60 * mins + secs;
					break;
				}
			case 50:
				s.check_charges = 1;
				break;
			case '?':
				EXIT_ERROR(ARG_ERROR, "%s", "Try -h/--help.\n");
			default:
				EXIT_ERROR(ARG_ERROR, "%s", "We should not be here!\n");
		}

	}
}

/* Check if options are set correctly */
void check_settings(void) {

	if(s.at_customization == AT_CUSTOM_PARTNER || s.at_customization == AT_CUSTOM_VALENCE)
		EXIT_ERROR(ARG_ERROR, "%s", "These atom type customizations are not implemented right now. Stay tuned.\n");

	if(s.mode == MODE_NOT_SET)
		EXIT_ERROR(ARG_ERROR, "%s", "No mode set.\n");

	if(s.sdf_file[0] == '\0')
		EXIT_ERROR(ARG_ERROR, "%s", "No .sdf file provided.\n");

	if(s.mode == MODE_PARAMS) {
		if(s.chg_file[0] == '\0')
			EXIT_ERROR(ARG_ERROR, "%s", "No .chg file provided.\n");

		if(s.kappa_set < 1e-10) {
			if(s.full_scan_precision < 1e-10)
				EXIT_ERROR(ARG_ERROR, "%s", "Full scan precision must be set correctly in params mode.\n");

			if(s.kappa_max < 1e-10)
				EXIT_ERROR(ARG_ERROR, "%s", "Maximum for kappa must be set correctly in params mode.\n");

			if(s.full_scan_precision > s.kappa_max)
				EXIT_ERROR(ARG_ERROR, "%s", "Full scan precision must be less than kappa max.\n");
		}
		else {
			if(s.full_scan_precision > 1e-10 || s.kappa_max > 1e-10)
				EXIT_ERROR(ARG_ERROR, "%s", "Cannot set full scan precision and/or kappa max if --kappa is used.\n");

			if(s.full_scan_only)
				EXIT_ERROR(ARG_ERROR, "%s", "Cannot use full scan if single kappa is selected.\n");
		}

		if(s.tabu_size < 0.0f || s.tabu_size > 1.0f)
			EXIT_ERROR(ARG_ERROR, "%s", "Tabu size has to be number in range [0.0; 1.0]\n");

		if(s.limit_iters != NO_LIMIT_ITERS && s.limit_iters > 100000)
			EXIT_ERROR(ARG_ERROR, "%s", "Number of iterations should be no higher than 1e6.\n");

		if(s.limit_time != NO_LIMIT_TIME && s.limit_time > 36000 * 1000)
			EXIT_ERROR(ARG_ERROR, "%s", "Maximum time should not be higher than 1000 hours.\n");

		if(!s.full_scan_only && s.sort_by != SORT_R)
			EXIT_ERROR(ARG_ERROR, "%s", "Full scan must be used for sort-by other than R.\n");

	} else if(s.mode == MODE_CHARGES) {
		if(s.par_file[0] == '\0')
			EXIT_ERROR(ARG_ERROR, "%s", "No .par file provided.\n");

		if(s.chg_out_file[0] == '\0')
			EXIT_ERROR(ARG_ERROR, "%s", "No .chg output file provided.\n");
	} else if(s.mode == MODE_CROSS) {
		if(s.chg_file[0] == '\0')
			EXIT_ERROR(ARG_ERROR, "%s", "No .chg file provided.\n");

		if(s.par_file[0] == '\0')
			EXIT_ERROR(ARG_ERROR, "%s", "No .par file provided.\n");
	}
}

void print_settings(void) {

	printf("\nSettings:\n\n");

	printf("Mode: ");
	switch(s.mode) {
		case MODE_INFO:
			printf("info (print info about the training set)\n");
			break;
		case MODE_PARAMS:
			printf("params (calculate EEM parameters)\n");
			break;
		case MODE_CHARGES:
			printf("charges (calculate EEM charges)\n");
			break;
		case MODE_CROSS:
			printf("cross (perform cross-validation of the EEM parameters)\n");
			break;
		case MODE_NOT_SET:
			assert(0);
	}

	printf("\nFiles:\n");
	printf(" Structural (.sdf) file: %s\n", s.sdf_file);
	if(s.par_file[0] != '\0')
		printf(" Parameters (.par) file: %s\n", s.par_file);

	if(s.chg_file[0] != '\0')
		printf(" Charges (.chg) file: %s\n", s.chg_file);

	if(s.par_out_file[0] != '\0')
		printf(" Parameters (.par) output file: %s\n", s.par_out_file);

	if(s.chg_out_file[0] != '\0')
		printf(" Charges output (.chg)'file: %s\n", s.chg_out_file);

	if(s.chg_stats_out_file[0] != '\0')
		printf(" Charges stats output (.chgs) file: %s\n", s.chg_stats_out_file);

	printf("\nAtom types grouped by: ");
	switch(s.at_customization) {
		case AT_CUSTOM_ELEMENT:
			printf("element\n");
			break;
		case AT_CUSTOM_ELEMENT_BOND:
			printf("element + bond order\n");
			break;
		case AT_CUSTOM_PARTNER:
			printf("bonding partner\n");
			break;
		case AT_CUSTOM_VALENCE:
			printf("valence state\n");
			break;
	}

	printf("\nVerbosity level: ");
	switch(s.verbosity) {
		case VERBOSE_MINIMAL:
			printf("0 (only minimal info)\n");
			break;
		case VERBOSE_DISCARD:
			printf("1 (include discarding info)\n");
			break;
		case VERBOSE_KAPPA:
			printf("2 (include discarding + kappa search info)\n");
			break;
		default:
			printf("%u (I won't tell you more than on level 2. Sorry about that.)\n", s.verbosity);
			break;
	}

	if(s.mode == MODE_PARAMS) {
		printf("\nSort by: ");
		switch(s.sort_by) {
			case SORT_R:
				printf("R");
				break;
			case SORT_RMSD:
				printf("RMSD");
				break;
			case SORT_MSE:
				printf("MSE");
				break;
			case SORT_D_AVG:
				printf("Avg D");
				break;
			case SORT_D_MAX:
				printf("Max D");
				break;
		}

		if(s.sort_by == SORT_R)
			printf(" (higher is better)\n");
		else
			printf(" (lower is better)\n");

		printf("\nDiscarding:\n");
		printf(" Mode: ");
		switch(s.discard) {
			case DISCARD_OFF:
				printf("off\n");
				break;
			case DISCARD_SIMPLE:
				printf("simple (try each molecule once)\n");
				break;
			case DISCARD_ITER:
				printf("iterative (try molecules at random until limit is reached)\n");
				break;
		}
		printf(" Time limit: ");
		if(s.limit_time == NO_LIMIT_TIME)
			printf("none set\n");
		else {

			unsigned tmp = (unsigned) s.limit_time;
			unsigned secs = tmp % 60;
			tmp /= 60;
			unsigned mins = tmp % 60;
			tmp /= 60;
			unsigned hours = tmp;
			printf("%u hour(s) %u min(s) %u sec(s)\n", hours, mins, secs);


		}
		printf(" Iterations limit: ");
		if(s.limit_iters == NO_LIMIT_ITERS)
			printf("none set\n");
		else
			printf("%d\n", s.limit_iters);

		printf("\nKappa search:\n");
		printf(" Mode: ");
		if(s.kappa_set > 0.0f)
			printf("single kappa value = %5.3f\n", s.kappa_set);
		else {
			printf("full scan from 0.0 to %5.3f with step %5.3f", s.kappa_max, s.full_scan_precision);

			if(!s.full_scan_only)
				printf(" + iterative refinement\n");
			else
				printf("\n");
		}
	}

	printf("\n");
}