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
  BWPing is a tool to measure bandwidth and response times
  between two hosts using UDP request/reply mechanism.
  It requires a client and a server program.

  This is the client side.

  First developement by professor Daniel Macedo.
  Adapted by Henrique Moura and Erik de Britto e Silva


  TODO list:
  1-) Online calculation of 0/50/95th quantile of one way delay and number of loss periods;
  2-) improve format presentation: using Kbits, Mbits, KBytes, MBytes
  3-) allow the user to define the period between periodic bandwidth reports
  4-) calcular o tempo de ida e o tempo de volta (é possível? sincronismo?)
  5-) user, system, and wall-clock time

  xxx) transmitter and receiver CPU utilization - ok - just compile with -DCPU_INFO

 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h> // stdout
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <netdb.h>
#include <sys/errno.h>
#include <time.h>
#include <unistd.h> // getopt
#include <stdbool.h>

#include "ping.h"
#include "cpu_info.h"
#include "wireless.h"


// if VERBOSE is defined, display sending and receiving packets
#undef VERBOSE

// Turn on DEBUG 1 to display wait times and timeouts
//#define DEBUG 1

uint32_t length = DEFAULT_MINIMUM_PAYLOAD_SIZE;
uint32_t global_sent = 0, global_rcvd = 0;

long double delay_m, delay_std, jitter_m, jitter_std, lastdelay;
unsigned long long sent, recvd;
long double idle_lt_m, nidle_lt_m, idle_rt_m, nidle_rt_m;
long double link_m, level_m, noise_m;

struct timeval next_print;

struct timeval print_interval; // time between reports

#define MICROSECONDS 1000000

void timeval_add (result, x, y)
   struct timeval *result, *x, *y;
{
    long nsec = y->tv_usec + x->tv_usec;
    result->tv_usec = nsec % MICROSECONDS;
    result->tv_sec = y->tv_sec + x->tv_sec + nsec / MICROSECONDS;
}

int timeval_subtract (result, x, y)
   struct timeval *result, *x, *y;
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    long nsec = (y->tv_usec - x->tv_usec) / MICROSECONDS + 1;
    y->tv_usec -= MICROSECONDS * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > MICROSECONDS) {
    long nsec = (x->tv_usec - y->tv_usec) / MICROSECONDS;
    y->tv_usec += MICROSECONDS * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

double getTV(struct timeval *tv) {
  return ((double)tv->tv_sec) + ((double)tv->tv_usec)*1e-6;
}

void clear_stats() {
  delay_m = 0;
  delay_std = 0;
  jitter_m = 0;
  jitter_std = 0;
  sent = 0;
  recvd = 0;
  idle_lt_m = nidle_lt_m = idle_rt_m = nidle_rt_m = 0; // clear cpu times
  link_m = level_m = noise_m = 0; // wireless info

  gettimeofday(&next_print,NULL);
  timeval_add(&next_print, &next_print, &print_interval);
}

void make_accounting(header *hdr, bool cpu_usage, bool rssi) {

  struct timeval now, diff;
  gettimeofday(&now, NULL);

    struct timeval hdr_time;
    hdr_time.tv_sec = hdr->sent_time.tv_sec;
    hdr_time.tv_usec = hdr->sent_time.tv_usec;
    timeval_subtract(&diff, &now, &hdr_time);
    long double delay = getTV(&diff)/2.0;

    //Counter of received packets
    recvd++;

    //Delay calculation
    delay_m += delay;
    delay_std += delay * delay;

    //Jitter calculation
    double jitter = fabs(delay - lastdelay);
    jitter_m += jitter;
    jitter_std += jitter * jitter;

    // cpu times calculation
    if (cpu_usage) {
      cpu_info info;
      get_cpu_info(&info);

      idle_lt_m += info.idle_time_perc;
      nidle_lt_m += info.non_idle_time_perc;

      idle_rt_m += hdr->idle_time_perc;
      nidle_rt_m += hdr->non_idle_time_perc;
    }

    if (rssi) {
      link_m += hdr->link;
      level_m += hdr->level;
      noise_m += hdr->noise;
    }

    lastdelay = delay;
 }

void report(bool csv, bool cpu_usage, bool rssi, FILE * fp) {
  struct timeval now;
  gettimeofday(&now, NULL);

  // struct timeval add_t;
  // timeval_add(&add_t, &last_print, &print_interval);

  // // check if it is time to print the report
  // if ((now.tv_sec > add_t.tv_sec) || ((now.tv_sec = add_t.tv_sec) && (now.tv_usec > add_t.tv_usec)) ) {
  //    printf("last_print\tsec %d usec : %d | print_interval sec %d usec : %d\n", (int)last_print.tv_sec, (int)last_print.tv_usec, (int)print_interval.tv_sec, (int)print_interval.tv_usec);
  //    printf("add_t\t\tsec %d usec : %d \nnow\t\tsec %d usec : %d\n", (int)add_t.tv_sec, (int)add_t.tv_usec, (int)now.tv_sec, (int)now.tv_usec);
  //   // contador++;

  if ((now.tv_sec >= next_print.tv_sec) && (now.tv_usec >= next_print.tv_usec)) {
    //time to print the variables...
    timeval_add(&next_print, &now, &print_interval);

    double jitter,jitter_sq, delay, delay_sq;

    if(recvd > 0) {
      double inv_n = 1 / (double)recvd;
      delay = delay_m * inv_n;
      delay_sq = delay_std * inv_n;

      if(recvd >= 2) {
        // in the case, we have one or two packets
        // don't use the unbiased formula
        jitter = jitter_m;
        jitter_sq = jitter_std;
      } else {
        // ok, use the unbiased formula
        jitter = jitter_m/((double)(recvd-1));
        jitter_sq = jitter_std/((double)(recvd-1));
      }

      if (cpu_usage) {
        idle_lt_m *= inv_n;
        nidle_lt_m *= inv_n;
        idle_rt_m *= inv_n;
        nidle_rt_m *= inv_n;
      }

      if (rssi) {
        link_m *= inv_n;
        level_m *= inv_n;
        noise_m *= inv_n;
      }

    } else {

      delay = 0;
      delay_sq = 0;

      jitter = 0;
      jitter_sq = 0;

      if (cpu_usage) {
        idle_lt_m = nidle_lt_m = idle_rt_m = nidle_rt_m = 0;
      }

      if (rssi) {
        link_m = level_m = noise_m = 0;
      }
    }

    double delay_s = sqrt(delay_sq - delay*delay);
    double jitter_s = sqrt(fabs(jitter_sq - jitter*jitter));

    double loss = ((sent == 0 || sent <= recvd) ? 0.0 : 100.0*(sent - recvd)/(double)sent);
    long long int bw = (length * recvd * 8);

    time_t rawtime;
    time(&rawtime);
    struct tm * ti = localtime (&rawtime);
    global_rcvd += recvd;
    global_sent += sent;
    double diff_pct = (global_sent - global_rcvd);
    //double diff_percent =  ((diff_pct * 100) / global_sent);

    char cpu_i[200];
    char * sprintf_format;
    if (cpu_usage) {
      if (csv)
        sprintf_format = ", %f, %f, %f, %f";
      else
        sprintf_format = "local[idle: %10.6f non-idle: %10.6f] remote[idle: %10.6f non-idle: %10.6f] ";
      sprintf((char *)&cpu_i, sprintf_format, (double) idle_lt_m*100.0, (double) nidle_lt_m*100.0, (double) idle_rt_m*100.0, (double) nidle_rt_m*100.0);
    } else {
      cpu_i[0] = '\0';
    }

    char rssi_i[400];
    if (rssi) {
      float link, level, noise;
      get_rssi(&link, &level, &noise);
      if (csv)
        sprintf_format = ", %f, %f, %f, %f, %f, %f";
      else
        sprintf_format = "local[link quality: %5.1f signal: %5.1f noise: %5.1f] remote[link quality: %5.1f signal: %5.1f noise: %5.1f] ";
      sprintf((char *)&rssi_i, sprintf_format, link, level, noise, (double) link_m, (double) level_m, (double) noise_m);
    } else {
      rssi_i[0] = '\0';
    }

    if (csv) {
      fprintf(fp,"%04d%02d%02d%02d%02d%02d, %.6lu.%.6lu, %llu, %f, %f, %f, %f, %lf, %llu, %llu, %d, %d, %.0lf%s%s\n",
             (1900+ti->tm_year), (1+ti->tm_mon), ti->tm_mday, ti->tm_hour, ti->tm_min, ti->tm_sec,
             now.tv_sec, now.tv_usec, bw, delay,delay_s, jitter, jitter_s, loss, sent, recvd, global_sent, global_rcvd , diff_pct, cpu_i, rssi_i);
    } else {
      fprintf(fp,"[%04d%02d%02d%02d%02d%02d]%.6lu.%.6lu BWPING: Bw %llu bps Delay (%f,%f)s Jitter (%f,%f)s Loss %.2lf%% LSent %llu LRecv %llu TSent %d Trcvd %d TDiff %.0lf %s%s\n",
              (1900+ti->tm_year), (1+ti->tm_mon), ti->tm_mday, ti->tm_hour, ti->tm_min, ti->tm_sec,
              now.tv_sec, now.tv_usec, bw, delay,delay_s, jitter, jitter_s, loss,
              sent, recvd, global_sent, global_rcvd, diff_pct, cpu_i, rssi_i);
      fflush(fp);
    }
    clear_stats();
  }
}

void version(char ** argv) {
  printf("%s version %s\n", argv[0], BWPING_VERSION);
}

void usage(char ** argv) {
  version(argv);
  printf("Usage: %s -a <IP address> [-p <port>] [-l <length>] [-t <interval_in_us>] [-d duration] [-6] [-c] [-h] [-v] [-o <filename>] [-s wlan-if] [-r]\n", argv[0]);
  printf("\t-a ip      : server IPv4 address (required)\n");
  printf("\t-p port    : server UDP port in use, default = 5001\n");
  printf("\t-l length  : payload byte length of each packet\n");
  printf("\t-t interval: time between bwping requests in microseconds - default 10 ms\n");
  printf("\t-d duration: total execution time in seconds - Number of collected samples = duration/interval\n");
  printf("\t-i interval: time between periodic bandwidth reports in seconds - default 1 second - minimum 0.01 seconds (=10ms)\n");
  printf("\t-o outfile : record output to file name outfile - overwrite \n");
  printf("\t-s wlan-if : print received signal and noise power in dBm of interface wlan-if (don't work with APs)\n");
  printf("\t-r         : print CPU idle and non-idle times\n");
  printf("\t-6: connect using IPv6\n");
  printf("\t-c: output in CSV format - Comma Separated Values \n");
  printf("\t    print the following fields in each line, one line per sample\n");
  printf("\t    YYYYMMDDHHMMSS, tv_sec.tv_usec, bw, delay,delay_s, jitter, jitter_s, loss, packets sent, packets recv, total_sent, total_rcvd, total_diff\n");
  printf("\t    cpuidle_t_local, cpu_nonidle_t_local, cpuidle_t_remote, cpu_nonidle_t_remote\n");
  printf("\t    link_quality_local, signal_local, noise_local, link_quality_remote, signal_remote, noise_remote\n");
  printf("\nMiscellaneous:\n");
  printf("\tbwping with no -d duration runs forever!\n");
  printf("\t-h: print this help message and quit\n");
  printf("\t-v: print version information and quit\n");

  // TODO:
  /*
    -f, --format    [kmKM]   format to report: Kbits, Mbits, KBytes, MBytes
  */
}

int main(int argc, char**argv) {
  int sockfd;
  header recv, send;
  double duration = -1.0;
  time_t start_time;

  FILE * fp = stdout;  /* Output file handler */

  char out_filename[255];

  bool csv = false;
  bool outfile = false;
  bool ipv6 = false;
  bool cpu_usage = false;
  bool rssi = false;

  int port = 5001;
  long long int intusec = 10000; // 10ms

  // default: print each second
  print_interval.tv_sec = 1;
  print_interval.tv_usec = 0;

  int c = 0;
  char * hostname = NULL;
  while ((c = getopt(argc, argv, "a:p:l:t:d:o:i:s:6chvr")) != -1) {
    switch(c){
      case 'a':
        hostname = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'l':
        length = atoi(optarg);
        if (length < DEFAULT_MINIMUM_PAYLOAD_SIZE) length = DEFAULT_MINIMUM_PAYLOAD_SIZE;
        break;
      case 't':
        sscanf(optarg,"%lld",&intusec);
        break;
      case 'd':
        sscanf(optarg,"%lf",&duration);
        break;
      case 'o':
        sscanf(optarg,"%s",out_filename);
        outfile = true;
        break;
      case 'i': {
        float print_time;
        sscanf(optarg,"%f",&print_time);
        if (print_time < 0.01) print_time = 0.01; // if error or less then 10ms, sets to 10ms
        print_interval.tv_sec = (int) trunc(print_time);
        print_interval.tv_usec = (int) trunc((print_time - print_interval.tv_sec)*1000)*1000; // in microseconds
        break;
      }
      case 's':
        rssi = true;
        int err;
        if ((err = open_wireless_info(optarg)) < 0) {
          switch (err) {
            case -1:
              fprintf(stderr,"Info about wireless interface not found! Please check the wlan interface name.\n");
              break;
            case -2:
            case -3:
              fprintf(stderr,"Error reading wireless interface data!\n");
              break;
          }
          exit(1); // error
        }
        break;
      case 'c':
        csv = true;
        break;
      case '6':
        ipv6 = true;
        break;
      case 'r':
        cpu_usage = true;
        break;
      case 'v':
        version(argv);
        exit(1);
      case 'h':
        usage(argv);
        exit(1);
      default:
        usage(argv);
        exit(1);
    }
  }
  if (length < sizeof(header)) length = sizeof(header);
  if (NULL == hostname){
     fprintf(stderr,"Server IP Address is required! Exiting...\n");
     fprintf(stderr,"Call %s -h to see some help.\n", argv[0]);
     exit(1);
  }

  if (outfile) {
     fp = fopen(out_filename,"w"); // sent to a file
     if (fp == NULL){
        fprintf(stderr,"Cannot create local file %s\n",out_filename);
        exit(1);
     }
  } else {
    fp = stdout; // send to screen
  }

  struct sockaddr_storage servaddr;
  socklen_t addr_size;
  bzero(&servaddr, sizeof(servaddr));

  if (ipv6) {
    sockfd=socket(AF_INET6, SOCK_DGRAM, 0);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo * servinfo = NULL;
    int rv;
    if ((rv = getaddrinfo(hostname, NULL, &hints, &servinfo)) != 0) {
       fprintf(stderr,"Could not resolve hostname %s\n",hostname);
       exit(1);
    }

    memcpy(&servaddr, servinfo->ai_addr, servinfo->ai_addrlen);
    struct sockaddr_in6 * addr = (struct sockaddr_in6 *)&servaddr;
    addr->sin6_port   = htons(port);
    addr_size = servinfo->ai_addrlen;

    if (servinfo) freeaddrinfo(servinfo);
  } else {
    sockfd=socket(AF_INET, SOCK_DGRAM, 0);

    struct hostent * hp = gethostbyname(hostname);
    if (hp == NULL) {
       fprintf(stderr,"Could not resolve hostname %s\n",hostname);
       exit(1);
    }
    struct sockaddr_in * addr = (struct sockaddr_in *)&servaddr;
    addr->sin_family = AF_INET;
    memcpy(&(addr->sin_addr.s_addr), hp->h_addr, hp->h_length);
    addr->sin_port = htons(port);
    addr_size = sizeof(*addr);
  }
  //  setsockopt(sockfd,SOL_SOCKET, SO_SNDBUF,&length,sizeof(int));

  char outmsg[length];
  bzero(outmsg, sizeof(char)*length);
  send.seqnum = 0;

  struct timeval interval;
  interval.tv_sec = intusec / 1e6;
  interval.tv_usec = intusec - interval.tv_sec * 1e6;

  clear_stats();

  //start_time = time(NULL);
  time(&start_time);

  // Print the RSSI - Received signal and noise power level in dBm of the selected interface
  bool inf_loop = true;
  while (inf_loop) {
    struct timeval send_time;
    gettimeofday(&send_time,NULL);
    send.sent_time.tv_sec = send_time.tv_sec;
    send.sent_time.tv_usec = send_time.tv_usec;

    bcopy(&send, &outmsg, sizeof(header)); // copy header contents to payload

    #ifdef VERBOSE
      printf("Sending packet number %d\n",send.seqnum);
    #endif
    send.seqnum++;
    sent++;
    int success = sendto(sockfd, outmsg, sizeof(char)*length, 0, (struct sockaddr *)&servaddr, addr_size);

    if(success == -1 && errno != 55) {
      // Error number 55 is "no space available". In this case, we just wait
      // for packets during the interval...
      fprintf(stderr,"Unable to call sendto, returned %d: ",errno);
      perror("");
      exit(1);
    }

    struct timeval timeout, last_wait;
    timeout.tv_sec = interval.tv_sec;
    timeout.tv_usec = interval.tv_usec;

    // *select* waiting until next packet timeout
    double toutdouble = 0;
    do {

      gettimeofday(&last_wait, NULL);

      //select during specificed interval
      fd_set socketReadSet;
      FD_ZERO(&socketReadSet);
      FD_SET(sockfd,&socketReadSet);

      struct timeval tstart, tfinish, diff;

      gettimeofday(&tstart,NULL);

      struct timeval copy;
      copy.tv_sec = timeout.tv_sec;
      copy.tv_usec = timeout.tv_usec;

      int retVal = select(sockfd+1, &socketReadSet, 0, 0, &copy);
      #ifdef DEBUG
        printf("Timeout after select: %.6lu.%.6lu\n",timeout.tv_sec,timeout.tv_usec);
        printf("retVal = %d\n",retVal);
      #endif

      gettimeofday(&tfinish, NULL);
      timeval_subtract(&diff, &tfinish, &tstart);
      struct timeval temp;
      timeval_subtract(&temp, &timeout, &diff);

      timeout.tv_sec = temp.tv_sec;
      timeout.tv_usec = temp.tv_usec;

      toutdouble = getTV(&timeout);

      if(retVal > 0) {
          recvfrom(sockfd, &recv, sizeof(recv), 0, NULL, NULL);
          make_accounting(&recv, cpu_usage, rssi);
          #ifdef VERBOSE
            printf("Received Reply from packet %d\n", recv.seqnum);
          #endif
      }
      report(csv, cpu_usage, rssi, fp); // report information
    } while(toutdouble > 0);
    time_t  just_now;
    time(&just_now);
    double dif_t = difftime(just_now, start_time);
    if ((duration > 0) && (dif_t >= duration )) {
      inf_loop = false;
    }

  }
  close_wireless_info();
}
