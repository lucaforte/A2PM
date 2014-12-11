

# gnuplot -e "filename='foo.data'" foo.plg


#set terminal postscript eps enhanced color font 'Helvetica,14'
#set output sprintf("gnuplot/%s.eps",the_title)

set terminal pngcairo size 1024,768 enhanced font 'Verdana,14'
set output sprintf("gnuplot/%s.png",the_title)

unset key

set yrange [0:*]
set xrange [0:*]

rnd(x) = floor(x/30)*30

set title the_title

set xlabel "RTTC/RTTH (seconds)"
set ylabel "Predicted RTTH (seconds)"

plot	'data.dat'	using (rnd($2)):($3) smooth unique,\
		x
