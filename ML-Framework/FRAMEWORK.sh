#!/bin/bash
# Alessandro Pellegrini <pellegrini@dis.uniroma1.i>
# Pierangelo Di Sanzo <disanzo@dis.uniroma1.it>

# Framework Configuration
database_file=db.txt
lambdas=(100)
#lambdas=(0 0.1  1  10  100  1000  10000  100000  1000000 10000000 100000000 1000000000)
WEKA_PATH="c:/Programmi/Weka-3-7/weka.jar"

# What do we have to run?
run_datapoint_aggregation=true
run_lasso=true
apply_lasso=true
plot_original_parameters=true
plot_lasso_as_predictor=true
run_weka_linear=true
run_weka_m5p=true
run_weka_svm=true
run_weka_svm2=true
run_weka_neural=true
evaluate_error=true

# This is a convenience step, not required for the actual learning
replot_models=false






##### DO NOT MODIFY BELOW THIS LINE ####


mkdir -p csv
mkdir -p data
mkdir -p error



if [ "$run_datapoint_aggregation" = true ] ; then
	echo "***************************"
	echo "***  AGGREGATING DATA   ***"
	echo "***************************"
	./aggregate.exe $database_file
fi



if [ "$run_lasso" = true ] ; then
	echo "***************************"
	echo "***   LASSO GRAFTING    ***"
	echo "***************************"
	
	echo "" > beta-vectors.txt
	unlink beta-vectors.txt
	
	for lambda in "${lambdas[@]}"; do
		echo Generating beta vector for $lambda;
		matlab -sd . -nodesktop -nosplash -wait -r "lasso($lambda)"
	done
fi
	
	
if [ "$apply_lasso" = true ] ; then
	echo "***************************"
	echo "***   APPLYING WEIGHTS  ***"
	echo "***************************"
	
	cp beta-vectors.txt working
	
	while [[ -s working ]] ; do
		lambda_val=$(head working -n 8 | tail -n 5 | grep lambda_val | sed -e 's/.*=//g')
		beta_vector=$(head working -n 8 | tail -n 5 | grep beta | sed -e 's/.*=//g')

		./applyBeta.py $lambda_val $beta_vector
		
		tail -n +8 working > wo
		mv wo working
	done
	
	unlink working
fi



if [ "$plot_original_parameters" = true ] ; then
	echo "***************************"
	echo "*** PLOTTING PARAMETERS ***"
	echo "***************************"
	
	for lambda in "${lambdas[@]}"; do
	
		echo "Plotting parameters for lambda $lambda"
		
		gnuplot -e "the_title='filtered-lambda-$lambda'" plot_parameters.plt
	done
	
	gnuplot plot_all_parameters.plt
fi


if [ "$plot_lasso_as_predictor" = true ] ; then
	echo "***************************"
	echo "***   PLOTTING LASSO    ***"
	echo "***************************"
	
	mkdir -p gnuplot/lasso
	
	for lambda in "${lambdas[@]}"; do
	
		echo "Plotting Lasso as a predictor for lambda $lambda"
		
		gnuplot -e "the_title='lasso-lambda-$lambda'" plotLasso.plt
	done
fi



######## DO STUFF FOR LINEAR MODELS

if [ "$run_weka_linear" = true ] ; then

	echo "***************************"
	echo "***       LINEAR        ***"
	echo "***************************"

	mkdir -p gnuplot/linear

	echo "Computing error for linear"
	
	java -classpath "$WEKA_PATH" weka.classifiers.functions.LinearRegression -t aggregated.csv -S 0 -R 1.0E-8 -c first -d model > error/linear-error.txt
	
	echo "Generating datapoints for linear"

	java -classpath "$WEKA_PATH"  weka.classifiers.functions.LinearRegression -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > data.dat
	
	unlink model
	
	echo "Generating plot for linear"
	
	gnuplot -e "the_title='linear/linear'" plot.plt
	
	mv data.dat data/linear.dat
fi




######## DO STUFF FOR M5P

if [ "$run_weka_m5p" = true ] ; then
    
	echo "***************************"
	echo "***         M5P         ***"
	echo "***************************"

	mkdir -p gnuplot/m5p

	echo "Computing error for M5P"
	
	java -classpath "$WEKA_PATH"  weka.classifiers.trees.M5P -t aggregated.csv -M 4.0 -c first -d model > error/m5p-error.txt
	
	echo "Generating datapoints for M5P"

	java -classpath "$WEKA_PATH"  weka.classifiers.trees.M5P -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > data.dat
	
	unlink model

	echo "Generating plot for M5P"

	gnuplot -e "the_title='m5p/m5p'" plot.plt

	mv data.dat data/m5p.dat

fi



if [ "$run_weka_svm" = true ] ; then
	echo "***************************"
	echo "***        SVM          ***"
	echo "***************************"

	mkdir -p gnuplot/svm

	echo "Computing error for SVM"
	
	java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -t aggregated.csv -C 1.0 -N 0 -I "weka.classifiers.functions.supportVector.RegSMOImproved -T 0.001 -V -P 1.0E-12 -L 0.001 -W 1" -K "weka.classifiers.functions.supportVector.PolyKernel -E 1.0 -C 250007" -c first -d model > error/svm-error.txt 
	
	echo "Generating datapoints for SVM"

	java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > data.dat
	
	unlink model

	echo "Generating plot for SVM"

	gnuplot -e "the_title='svm/svm'" plot.plt

	mv data.dat data/svm.dat
fi



if [ "$run_weka_svm2" = true ] ; then
	echo "***************************"
	echo "***       SVM 2         ***"
	echo "***************************"

	mkdir -p gnuplot/svm2

	echo "Computing error for SVM2"
			
	java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -t aggregated.csv -C 1.0 -N 0 -I "weka.classifiers.functions.supportVector.RegSMOImproved -T 0.001 -P 1.0E-12 -L 0.001 -W 1" -K "weka.classifiers.functions.supportVector.PolyKernel -E 1.0 -C 250007" -c first -d model > error/svm2-error.txt 
	
	echo "Generating datapoints for SVM2"

	java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > data.dat
	
	unlink model

	echo "Generating plot for SVM2"

	gnuplot -e "the_title='svm2/svm2'" plot.plt

	mv data.dat data/svm2.dat
fi


if [ "$run_weka_neural" = true ] ; then
	echo "***************************"
	echo "***   NEURAL NETWORKS   ***"
	echo "***************************"

	echo "" > data.dat
	echo "" > error/neural-error.txt
	
	mkdir -p gnuplot/neural

	echo "Computing error for Neural Networks"
	
	java -classpath "$WEKA_PATH"  weka.classifiers.functions.MultilayerPerceptron -L 0.3 -M 0.2 -N 500 -V 0 -S 0 -E 20 -H a -G -R -t aggregated.csv -c first -d model >> error/neural-error.txt 
	
	echo "Generating datapoints for Neural Networks"

	java -classpath "$WEKA_PATH"  weka.classifiers.functions.MultilayerPerceptron -l model -T aggregated.csv -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText > data.dat
	
	unlink model

	echo "Generating plot for Neural Networks"

	gnuplot -e "the_title='neural/neural'" plot.plt

	mv data.dat data/neural.dat
fi


if [ "$evaluate_error" = true ] ; then
	echo "***************************"
	echo "***       ERROR         ***"
	echo "***************************"
	
	echo -e "Algorithm\tMAE\tSMAE\tSMAE_ABS" > error/errors.txt
	
	for f in data/*.dat; do
		name=$(echo $f | sed -e 's/.*data\///g')
		name=$(echo $name | sed -e 's/\.dat//g')
		
		error=$(./evaluateErrors.py $f)
		
		echo -e "$name\t$error" >> error/errors.txt
	done


	for f in csv/lasso-lambda*.csv; do
		name=$(echo $f | sed -e 's/.*data\///g')
		name=$(echo $name | sed -e 's/\.csv//g')
		
		error=$(./evaluateLassoErrors.py $f)
		
		echo -e "$name\t$error" >> error/errors.txt
	done
	
	
	
	echo "All errors computed"
	
fi





if [ "$replot_models" = true ] ; then
	echo "***************************"
	echo "***  REPLOTTING MODELS  ***"
	echo "***************************"
	
	for f in data/*.dat
	do
		name=$(echo $f | sed -e 's/.*\///')
		name=$(echo $name | sed -e 's/-/\//')
		name=$(echo $name | sed -e 's/\.dat//')
	
		cp $f data.dat
		
		echo "Generating plot for $name"
		gnuplot -e "the_title='$name'" plot.plt
		
	done
	
	unlink data.dat
	
fi
