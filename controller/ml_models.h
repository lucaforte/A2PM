#include "system_features.h"

float get_predicted_rttc(int ml_model, system_features last_features, system_features current_features);

enum ml_models{
	LASSO_MODEL,
	LINEAR_REGRESSION_MODEL,
	M5P_MODEL,
	REPTREE_MODEL,
	SVM_MODEL,
	SVM2_MODEL,
};

