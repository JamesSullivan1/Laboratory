#!/bin/bash

# Runs all of the needed trials, logging the statistical data for each
# sample size automatically.
# Automation is nice.

chaos=$1
if [[ -z "$chaos" ]]; then
    rscript="plot.R"
else
    rscript="plot_chaos.R"
fi

sl_name="starlocks"
cl_name="classic"
out_name="results.dat"
out_fmt="Type\tCustomers\tAverage\tStdDev\tMax\tMin\n"
for i in {10,50,100,500,1000,2000,10000}
do
    echo Running trial $i
    # Collect the data
    ./run_trials.sh 10 $i 0 -q > "$sl_name-$i.dat"
    ./run_trials.sh 10 $i 1 -q > "$cl_name-$i.dat" 
    
    # Statistical analysis
    sl_raw_s=`cat $sl_name-$i.dat | awk '{print $4}' | tail -n +2 | 
        Rscript -e 'd<-scan("stdin", quiet=TRUE); cat(1000*mean(d), \
            1000*sd(d), 1000*max(d), 1000*min(d), sep="\n")'`
    sl_raw_c=`cat $sl_name-$i.dat | awk '{print $5}' | tail -n +2 | 
        Rscript -e 'd<-scan("stdin", quiet=TRUE); cat(1000*mean(d), \
            1000*sd(d), 1000*max(d), 1000*min(d), sep="\n")'`
    cl_raw_s=`cat $cl_name-$i.dat | awk '{print $4}' | tail -n +2 | 
        Rscript -e 'd<-scan("stdin", quiet=TRUE); cat(1000*mean(d), \
            1000*sd(d), 1000*max(d), 1000*min(d), sep="\n")'`
    cl_raw_c=`cat $cl_name-$i.dat | awk '{print $5}' | tail -n +2 | 
        Rscript -e 'd<-scan("stdin", quiet=TRUE); cat(1000*mean(d), \
            1000*sd(d), 1000*max(d), 1000*min(d), sep="\n")'`

    sl_ary_s=($sl_raw_s)
    sl_ary_c=($sl_raw_c)
    cl_ary_s=($cl_raw_s)
    cl_ary_c=($cl_raw_c)
    # Oh god
    sl_avg_s=${sl_ary_s[0]}
    sl_std_s=${sl_ary_s[1]}
    sl_max_s=${sl_ary_s[2]}
    sl_min_s=${sl_ary_s[3]}

    sl_avg_c=${sl_ary_c[0]}
    sl_std_c=${sl_ary_c[1]}
    sl_max_c=${sl_ary_c[2]}
    sl_min_c=${sl_ary_c[3]}

    cl_avg_s=${cl_ary_s[0]}
    cl_std_s=${cl_ary_s[1]}
    cl_max_s=${cl_ary_s[2]}
    cl_min_s=${cl_ary_s[3]}

    cl_avg_c=${cl_ary_c[0]}
    cl_std_c=${cl_ary_c[1]}
    cl_max_c=${cl_ary_c[2]}
    cl_min_c=${cl_ary_c[3]}

    # Append to the data string
    out_fmt+="Simple+Starlocks\t$i\t$sl_avg_s\t$sl_std_s\t$sl_max_s\t$sl_min_s\n"
    out_fmt+="Complex+Starlocks\t$i\t$sl_avg_c\t$sl_std_c\t$sl_max_c\t$sl_min_c\n"
    out_fmt+="Simple+Classic\t$i\t$cl_avg_s\t$cl_std_s\t$cl_max_s\t$cl_min_s\n"
    out_fmt+="Complex+Classic\t$i\t$cl_avg_c\t$cl_std_c\t$cl_max_c\t$cl_min_c\n"
done

# Write to final data file and tidy up 
echo -e "$out_fmt" | column -t  > "$out_name"
sort -k2 -n "$out_name" > tmp
mv tmp "$out_name"

# Now get R to run our plotting script
R CMD BATCH "$rscript"

echo "Done. Raw data in $out_name, Graph in results.pdf"

