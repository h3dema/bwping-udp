/*
 * bwping-udp, Copyright (c) 2014-2017, UFMG/DCC/WINET
 * All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact damacedo@dcc.ufmg.br.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 4. Neither the name of The UFMG/DCC/WINET nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UFMG/DCC/WINET AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 returns total idle time and total non-idle time, in percent of total time

read a line like this -->
     user    nice   system  idle      iowait irq   softirq  steal  guest  guest_nice
cpu  74608   2520   24433   1117073   6176   4054  0        0      0      0

The meanings of the columns are as follows, from left to right:
    0 cpuid: number of cpu
    1 user: normal processes executing in user mode
    2 nice: niced processes executing in user mode
    3 system: processes executing in kernel mode
    4 idle: twiddling thumbs
    5 iowait: waiting for I/O to complete
    6 irq: servicing interrupts
    7 softirq: servicing softirqs

ref: http://man7.org/linux/man-pages/man5/proc.5.html
*/

#include <stdio.h>
#include <string.h>
#include "cpu_info.h"

void get_cpu_info(cpu_info * info) {
    FILE * f;
    char line[MAX_BUFF];
    line[MAX_BUFF-1] = '\0';
    char * l;
    if ( (f = fopen(CPU_STATS_FILE, "r")) ) {
        while (!feof(f)) {
            if ( (l = fgets (line , MAX_BUFF-1 , f)) != NULL ) {
                char cpu[50];
                long user_t, nice_t, system_t, idle_t, iowait_t, irq_t, softirq_t, steal_t, guest_t, guest_nice_t;
                int num = sscanf(line, "%s %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", (char *)&cpu, &user_t, &nice_t, &system_t, &idle_t, &iowait_t, &irq_t, &softirq_t, &steal_t, &guest_t, &guest_nice_t);
                if (num == 11 && strcmp(cpu, "cpu") == 0) {
                    info->idlealltime = idle_t + iowait_t;
                    info->systemalltime = system_t + irq_t + softirq_t;
                    info->virtalltime = guest_t + guest_nice_t;
                    info->nonidle = user_t + nice_t + steal_t + info->systemalltime;
                    info->totaltime = info->nonidle + info->idlealltime + info->virtalltime;

                    info->idle_time_perc = (double) info->idlealltime / (double) info->totaltime;
                    info->non_idle_time_perc = (double) info->nonidle / (double) info->totaltime;
                    break; // don't need to continue to read the file
                }
            }
        }
        fclose(f);
    }
}

#ifdef MAIN
int main() {
    cpu_info info;
    get_cpu_info(&info);

    printf("Informações de CPU\n");
    printf("idlealltime  : %ld\n", info.idlealltime);
    printf("systemalltime: %ld\n", info.systemalltime);
    printf("virtalltime  : %ld\n", info.virtalltime);
    printf("nonidle      : %ld\n", info.nonidle);
    printf("totaltime    : %ld\n", info.totaltime);
    printf("idle_time_perc     : %f\n", info.idle_time_perc);
    printf("non_idle_time_perc : %f\n", info.non_idle_time_perc);
}
#endif