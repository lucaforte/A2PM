#!/bin/bash
# Alessandro Pellegrini <pellegrini@dis.uniroma1.i>
# Pierangelo Di Sanzo <disanzo@dis.uniroma1.it>

# Framework Configuration
database_file=db.txt
lambdas=(100)
#lambdas=(0 0.1  1  10  100  1000  10000  100000  1000000 10000000 100000000 1000000000)
WEKA_PATH="c:/Programmi/Weka-3-7/weka.jar"

# What do we have to run?
run_datapoint_aggregation=false
run_lasso=false
apply_lasso=false
plot_original_parameters=false
plot_lasso_as_predictor=false
run_weka_linear=true
run_weka_m5p=false
run_weka_svm=false
run_weka_svm2=false
evaluate_error=false

# This is a convenience step, not required for the actual learning
replot_models=false






##### DO NOT MODIFY BELOW THIS LINE ####


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

	echo "" > data.dat
	echo "" > error/linear-error.txt
	
	mkdir -p gnuplot/linear

	for f in csv/filtered-*; do

		name=$(echo $f | sed -e 's/.*filtered-//g')
		name=$(echo $name | sed -e 's/\.csv//g')
	
		echo "Computing error for $name"
	
		echo $name >> error/linear-error.txt
	
		java -classpath "$WEKA_PATH" weka.classifiers.functions.LinearRegression -t $f -S 0 -R 1.0E-8 -c first -d model >> error/linear-error.txt
	
		echo "Generating datapoints for $name"

		java -classpath "$WEKA_PATH"  weka.classifiers.functions.LinearRegression -l model -T $f -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText | sed -n '/=== Predictions under cross-validation ===/,$p' 2>/dev/null | tail -n +4 | sed -e's/  */ /g' | sed -e 's/^ //g' | sed -e 's/ /\t/g' > data.dat
	
		echo "Generating plot for $name"
	
		#gnuplot -e "the_title='linear/$name'" plot.plt
	
		#mv data.dat data/linear-$name.dat
	done
fi




######## DO STUFF FOR M5P

if [ "$run_weka_m5p" = true ] ; then
    
	echo "***************************"
	echo "***         M5P         ***"
	echo "***************************"

	echo "" > data.dat
	echo "" > error/m5p-error.txt
	
	mkdir -p gnuplot/m5p

	for f in csv/filtered-*; do

		name=$(echo $f | sed -e 's/.*filtered-//g')
		name=$(echo $name | sed -e 's/\.csv//g')
	
		echo "Computing error for $name"
		
		echo $name >> error/m5p-error.txt
	
		java -classpath "$WEKA_PATH"  weka.classifiers.trees.M5P -t $f -M 4.0 -c first -d model >> error/m5p-error.txt
		
		echo "Generating datapoints for $name"

		java -classpath "$WEKA_PATH"  weka.classifiers.trees.M5P -l model -T $f -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText | sed -n '/=== Predictions under cross-validation ===/,$p' 2>/dev/null | tail -n +4 | sed -e's/  */ /g' | sed -e 's/^ //g' | sed -e 's/ /\t/g' > data.dat
	
		echo "Generating plot for $name"
	
		gnuplot -e "the_title='m5p/$name'" plot.plt
	
		mv data.dat data/m5p-$name.dat
	done

fi



if [ "$run_weka_svm" = true ] ; then
	echo "***************************"
	echo "***        SVM          ***"
	echo "***************************"

	echo "" > data.dat
	echo "" > error/svm-error.txt
	
	mkdir -p gnuplot/svm

	for f in csv/filtered-*; do

		name=$(echo $f | sed -e 's/.*filtered-//g')
		name=$(echo $name | sed -e 's/\.csv//g')
	
		echo "Computing error for $name"
		
		echo $name >> error/svm-error.txt
	
		java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -t $f -C 1.0 -N 0 -I "weka.classifiers.functions.supportVector.RegSMOImproved -T 0.001 -V -P 1.0E-12 -L 0.001 -W 1" -K "weka.classifiers.functions.supportVector.PolyKernel -E 1.0 -C 250007" -c first -d model >> error/svm-error.txt 
		
		echo "Generating datapoints for $name"

		java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -l model -T $f -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText | sed -n '/=== Predictions under cross-validation ===/,$p' 2>/dev/null | tail -n +4 | sed -e's/  */ /g' | sed -e 's/^ //g' | sed -e 's/ /\t/g' > data.dat
	
		echo "Generating plot for $name"
	
		gnuplot -e "the_title='svm/$name'" plot.plt
	
		mv data.dat data/svm-$name.dat
	done
fi



if [ "$run_weka_svm2" = true ] ; then
	echo "***************************"
	echo "***       SVM 2         ***"
	echo "***************************"

	echo "" > data.dat
	echo "" > error/svm2-error.txt
	
	mkdir -p gnuplot/svm2

	for f in csv/filtered-*; do

		name=$(echo $f | sed -e 's/.*filtered-//g')
		name=$(echo $name | sed -e 's/\.csv//g')
	
		echo "Computing error for $name"
		
		echo $name >> error/svm2-error.txt
	
		java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -t $f -C 1.0 -N 0 -I "weka.classifiers.functions.supportVector.RegSMOImproved -T 0.001 -P 1.0E-12 -L 0.001 -W 1" -K "weka.classifiers.functions.supportVector.PolyKernel -E 1.0 -C 250007" -c first -d model >> error/svm2-error.txt 
		
		echo "Generating datapoints for $name"
	
		java -classpath "$WEKA_PATH"  weka.classifiers.functions.SMOreg -l model -T $f -c first -o -classifications weka.classifiers.evaluation.output.prediction.PlainText | sed -n '/=== Predictions under cross-validation ===/,$p' 2>/dev/null | tail -n +4 | sed -e's/  */ /g' | sed -e 's/^ //g' | sed -e 's/ /\t/g' > data.dat
	
		echo "Generating plot for $name"
	
		gnuplot -e "the_title='svm2/$name'" plot.plt
	
		mv data.dat data/svm2-$name.dat
	done
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
