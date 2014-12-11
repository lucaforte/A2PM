PANACEA Machine Learning / Rejuvenation Toolkit
===============================================


Introduction
------------

This package contains the PANACEA Machine Learning/Rejuvenation Toolkit,
which can be used to proactively predict the Remaining Time to Crash (RTTC) of
virtualized web-based application, and use the models to proactively
rejuvenate virtual machines when the crashing point is approaching,
during the system's operation, so as to prevent possible system failures.

Specifically, with an estimation of the RTTC of a component of the system 
under prediction mode, a proactive action for rejuvenating the system 
(e.g. via early restart of a machine) is performed before the component fails
to reduce, as much as possible, the system unavailability.

The input of the prediction model is a set of variables which are monitored
at runtime and related to utilization of system resources (CPU, memory, active threads, ...).
The actual prediction model is built using offline training using resource data
collected during the training phase.

To speed up the model generation time, the software uses regularization techniques 
to reduce the number of input parameters to the machine learner.



Architecture of the Toolkit
---------------------------

The toolkit is composed of three different software packages,
which are related to the respective Framework's execution stage.

Since the Framework relies on offline learning, the first step
is to collect hardware usage statistics from the involved machine.
This can be done using the tools in the feature_collection/`
subfolder. A client/server architecture is provided there.

Utilities to generate artificial memory leaks and unterminated
threads are provided in `utilities/`, in order to let the monitored
system to crash more quickly, provided that a specific injection
rate of anomalies is a-priori known.

The actual generation of the model is done by the toolkit
in `ML-Framework/` and can be run on a machine different from
the one(s) involved in the collection of data.

Finally, once the model has been built, the actual online prediction
and rejuvenation can be enforced by the tools in `controller/`. 




Installation Notes
------------------

### Dependencies

Most of the tools just require a standard C compiler on a POSIX
system, with pthreads. Any Linux distribution will likely meet
these dependencies.

On the other hand, concerning the ML Framework, an additional set
of dependecies are required, namely:
  - gnuplot (to generate plots of the prediction models)
  - WEKA (at least version 3.7, to run some of the learning algorithm)
  - Matlab (to run data regularization, and to build the Lasso prediction model)
  - LaTeX (for automatic generation of training reports)
  
Failing to satisfy any of these dependencies will not allow the
ML Framework to work properly, or will prevent some steps to
be correctly carried out. Some of these steps can be nevertheless
disabled during the configuration of the framework. The corresponding
steps can be activated as well depending on the dependencies which
are met on a specific machine.


### Compilation and Configuration

To compile the tools, simply issue `make` in the main folder
of the package.

The only additional installation step requires manually
configuring the `ML-Framework/FRAMEWORK.sh` script. Several
variables should be set according to the following meaning.

  - `database_file`
  - `lambdas`
  - `WEKA_PATH`
  - `lasso_algorithm`

Then, the steps to be carried out by the Framework can be specified
with a set of additional variables. Setting a variable to `false`
disables the step, while setting it to `true` enables it. The available
steps are>

  - `purge_old_data`: this step cleans out all results from previous
						executions of the framework
  - `run_datapoint_aggregation`: this step generates aggregated datapoints
						from the initial database file, which can be later
						used for training. The Framework relies on aggregated
						datapoints, so this step is mandatory, and could be
						disabled only if it has been run at least once.
  - `run_lasso`: this step runs the selected Lasso algorithm for all the
				 specified lambda values.
  - `apply_lasso`
  - `plot_original_parameters`
  - `plot_lasso_as_predictor`
  - `run_weka_linear`
  - `run_weka_m5p`
  - `run_weka_REPtree`
  - `run_weka_svm`
  - `run_weka_svm2`
  - `run_weka_neural`
  - `evaluate_error`
  - `generate_report`


### Usage

Collecting data is quite automatic: simply launch the client
on the machine which we want to monitor and the server on
the machine where the data is to be collected.

The controller usage is perfectly similar to that of the
collection utility.

To inject anomalies, simply run either `leaks` or `threads`
in background.

The ML Framework requires quite a lot of steps to complete
its job. We provide in the ML-Framework/ folder the convenience
FRAMEWORK.sh script which executes all the required steps.
To execute only some steps, you can set the variables
corresponding to the steps to exclude at the beginning of
the script to `false`.
