library(ggplot2)
library(plyr)
library(purrr)

calc_data <- function(data, variables, col = "unit") {
  cdata <- ddply(data, .variables = variables,
                 .fun = function(d) {
                   N = length (d[[col]])
                   sd = sd(d[[col]])
                   c(N,
                     mean = mean(d[[col]]),
                     sd,
                     se = sd / sqrt(N)
                   )
                 }
  )
  cdata
}

color_palette <- function()
{
  c("singly" = "#D55E00",
    "doubly" = "#009E43",
    "singly_cursor" = "#0072B2",
    "doubly_cursor" = "#E69F00",
    "draconic" = "#56B4E9")
}

bar_plot <- function(plot, title, x, y, palette = color_palette(), text_size=10)
{
  plot +
  geom_bar(position = position_dodge(width=0.9), width=0.8, stat="identity") +
  geom_errorbar(aes(ymin=mean-se, ymax=mean+se), position=position_dodge(width=0.9), width=0.8) +
  scale_fill_manual(values = palette) +
  labs(x=x, y=y) +
  theme(legend.position = "bottom",
      legend.title = element_blank(),
      text = element_text(size=text_size),
      legend.text = element_text(size=text_size),
      plot.title = element_text(size=text_size + 0.5),
      axis.text.x = element_text(size=text_size),
      axis.title.x = element_text(size=text_size),
      axis.text.y = element_text(size=text_size),
      axis.title.y = element_text(size=text_size)) +
  guides(fill=guide_legend(nrow=1, byrow=TRUE))
}

read_file <- function(file)
{
  read.csv(file=file, head=TRUE, sep=";")
}

benchmarks <- function() { c("draconic", "singly", "doubly", "singly_cursor", "doubly_cursor") }

plot_threads <- function(file, title)
{
  data <- read_file(file)
  data <- within(data, benchmark <- factor(benchmark, 
                                           levels=benchmarks()))
  data$unit <- data[["Throughput..Kops.s."]]
  
  cdata <- calc_data(data, c("threads", "benchmark"))
  cdata$threads <- as.ordered(cdata$threads)
  plot <- ggplot(data=cdata, aes(threads, mean, fill=benchmark))
  bar_plot(plot, title=title, x="threads", y="Kops/sec")
}

calc_speedup <- function(file, base = "draconic") {
  data <- read_file(file)
  data$unit <- data[["Throughput..Kops.s."]]
  cdata <- calc_data(data, c("threads", "benchmark"))
  base <- cdata[cdata$benchmark == base,]
  map(benchmarks(), .f = function(b) {
    c(b, mean(cdata[cdata$benchmark == b,]$mean / base$mean))
  })
}

plot <- plot_threads("results.csv", "Steady")
ggsave("threads.pdf", plot, width=300, height=95, units="mm", device=cairo_pdf)
