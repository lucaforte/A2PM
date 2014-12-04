function lasso(LAMBDA)

X=dlmread('aggregated.csv',',',10,0);
Y=X(:,1);
X=X(:,[2:size(X,2)]);
size(Y);
size(X);

setpath
weights = LassoGrafting(X, Y, LAMBDA);
weights_str = sprintf('%.15f,', weights);
weights_str = weights_str(1:end-1);% strip final comma

% Save output on text file, for later processing
diary beta-vectors.txt
diary on

disp(sprintf('\n\n***\n\nlambda_val=%s\n\nbeta=[%s]', num2str(LAMBDA), weights_str));

diary off

quit

end