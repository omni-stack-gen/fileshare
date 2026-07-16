#ifndef __FH_SYS_CONFIG_MPI_H__
#define __FH_SYS_CONFIG_MPI_H__

int fh_clk_enable(char* clk_name);
int fh_clk_disable(char* clk_name);
int fh_clk_set_rate(char* clk_name, unsigned long freq);
int fh_clk_get_rate(char* clk_name, unsigned long *freq);


#endif //ifndef __FH_SYS_CONFIG_MPI_H__

