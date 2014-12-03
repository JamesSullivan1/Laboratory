require(ggplot2)
require(reshape2)
require(scales)

dat = read.table("results.dat", header=TRUE, sep="")
datm = melt(dat, c("Customers", "Average", "StdDev", "Min", "Max"))

p = ggplot(datm, aes(x=Customers, y=Average, color=value)) +
    labs(x = "Total Number of Customers", 
         y = "Avg. Avg Turnaround Time (ms)",
         title = "Average Average Turnaround Time for each Customer 
         Type (over 10 trials, FIFO queue)",
         color = "Customer Type + Store Type") + 
    geom_line() +
    geom_point(size=3) +
    geom_errorbar(aes(ymin=Average-StdDev, ymax=Average+StdDev), 
            width=.1) +
    scale_x_continuous(trans=log_trans(),
            breaks=c(1,10,100,1000,10000)) + 
    scale_y_continuous(trans=log_trans(), 
            breaks=c(0.001, 0.004, 0.016, 0.064, 0.25, 1, 4))

ggsave(p, file="results.pdf", width=11, height=8)

