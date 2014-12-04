PANACEA Machine Learning Toolkit
================================

Introduction
------------

This package contains the PANACEA Machine Learning Toolkit,
which can be used to proactively predict the Remaining Time
to Crash of virtualized web-based application.




Architecture of the Toolkit
---------------------------

The toolkit is composed of three different software packages,
which are related to the respective Framework's execution stage.

Since the Framework relies on offline learning, the first step
is to collect hardware usage statistics from the involved machine.
This can be done using the tools in the feature_collection/
subfolder. A client/server architecture is provided there.

Utilities to generate artificial memory leaks and unterminated
threads are provided in utilities/

The actual generation of the model is done by the toolkit
in ML-Framework/ and can be run on a machine different from
the one(s) involved in the collection of data.

Finally, once the model has been built, the actual prediction
and rejuvenation can be enforced by the tools in controller/. 




Installation Notes
------------------

### Dependencies

Most of the tools just require a standard C compiler of a POSIX
system, with pthreads. Any Linux distribution will likely meet
these dependencies.

On the other hand, concerning the ML Framework, an additional set
of dependecies are required, namely:
  - gnuplot
  - WEKA (at least version 3.7)
  - Matlab
  
Failing to satisfy any of these dependencies will not allow the
ML Framework to work properly.


### Compilation and Configuration

To compile the tools, simply issue `make` in the main folder
of the package.

The only additional installation step requires setting
the variable WEKA_PATH in the ML-Framework/FRAMEWORK.sh script
to the correct Weka path for the system where the training
is performed.


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