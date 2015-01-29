#include "ml_models.h"


float calculate_lasso(system_features_with_slopes f) {
	return
			13.1487 *  f.gen_time +
			-22.0097 *  f.n_th_slope +
			-0.002  *  f.mem_used_slope +
			0.002  *  f.mem_free_slope +
			0.002  *  f.swap_used_slope +
			-0.002  *  f.swap_free_slope +
			-1.5356 *  f.cpu_user_slope +
			-9.2307 *  f.cpu_system_slope +
			-5.5353 *  f.cpu_idle_slope +
			-0.4934 *  f.n_th +
			-0.0004 *  f.mem_used +
			0.019  *  f.mem_buffers +
			-0.0006 *  f.swap_used +
			-6.3364 *  f.cpu_user +
			-16.4923 *  f.cpu_nice +
			-19.4085 *  f.cpu_system +
			-4.7375 *  f.cpu_iowait +
			-10.4713 *  f.cpu_idle +
			1856.7264;
}

float calculate_linear_regression(system_features_with_slopes f) {
	return
			13.1487 *  f.gen_time +
			-22.0097 *  f.n_th_slope +
			-0.002  *  f.mem_used_slope +
			0.002  *  f.mem_free_slope +
			0.002  *  f.swap_used_slope +
			-0.002  *  f.swap_free_slope +
			-1.5356 *  f.cpu_user_slope +
			-9.2307 *  f.cpu_system_slope +
			-5.5353 *  f.cpu_idle_slope +
			-0.4934 *  f.n_th +
			-0.0004 *  f.mem_used +
			0.019  *  f.mem_buffers +
			-0.0006 *  f.swap_used +
			-6.3364 *  f.cpu_user +
			-16.4923 *  f.cpu_nice +
			-19.4085 *  f.cpu_system +
			-4.7375 *  f.cpu_iowait +
			-10.4713 *  f.cpu_idle +
			1856.7264;
}

float get_predicted_rttc(int ml_model, system_features last_features, system_features current_features) {

	system_features_with_slopes f;
	f.gen_time=current_features.time-last_features.time;
	f.n_th_slope=current_features.n_th-last_features.n_th;
	f.mem_used_slope=current_features.mem_used-last_features.mem_used;
	f.mem_free_slope=current_features.mem_free-last_features.mem_free;
	f.swap_used_slope=current_features.swap_used-last_features.swap_used;
	f.swap_free_slope=current_features.swap_free-last_features.swap_free;
	f.cpu_user_slope=current_features.cpu_user-last_features.cpu_user;
	f.cpu_system_slope=current_features.cpu_system-last_features.cpu_system;
	f.cpu_idle_slope=current_features.cpu_idle-last_features.cpu_idle;
	f.n_th=last_features.n_th;
	f.mem_used=last_features.mem_used;
	f.mem_buffers=last_features.mem_buffers;
	f.swap_used=last_features.swap_used;
	f.cpu_user=last_features.cpu_user;
	f.cpu_nice=last_features.cpu_nice;
	f.cpu_system=last_features.cpu_system;
	f.cpu_iowait=last_features.cpu_iowait;
    f.cpu_idle=last_features.cpu_idle;

    return calculate_lasso(f);

/*
	switch (ml_model) {
	case LASSO_MODEL:

		break;
	case LINEAR_REGRESSION_MODEL:
		break;
	case M5P_MODEL:
		break;
	case REPTREE_MODEL:
		break;
	case SVM_MODEL:
		break;
	case SVM2_MODEL:
		break;
	default:
		printf ("\nError: No machine learning model selected");
		break;
	}
	*/
}