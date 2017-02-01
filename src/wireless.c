#include <stdio.h> // FILE
#include <stdbool.h> // boolean
#include <stdlib.h> // exit
#include <string.h> // strncmp, strlen

FILE * __frssi = NULL;
char * __wlan_if = NULL; // save the wireless interface to use inside get_rssi()

#define WIRELESS_INFO_FILE "/proc/net/wireless"
#define __FRSSI_BUFF_SIZE 256

/**
  skips the first two lines of the file, they don't contain data
  */
int skip_first_two_lines() {
  if (__frssi == NULL ) return -1;

  fseek(__frssi, 0, SEEK_SET );
  /* Starting two lines are text header */
  char __frssi_buff[__FRSSI_BUFF_SIZE];
  if ( fgets(__frssi_buff,__FRSSI_BUFF_SIZE,__frssi) == NULL ) return -2;
  #ifdef DEBUG
  printf("Buffer 1 = %s",__frssi_buff);
  #endif
  if ( fgets(__frssi_buff,__FRSSI_BUFF_SIZE,__frssi) == NULL ) return -3;
  #ifdef DEBUG
  printf("Buffer 2 = %s",__frssi_buff);
  #endif

  return 0;
}

int open_wireless_info(char * wlan_if) {

  __frssi=fopen(WIRELESS_INFO_FILE,"r");

  int err = skip_first_two_lines();
  if (err < 0) return err;

  if (!wlan_if || (strlen(wlan_if) == 0)) return -1;
  __wlan_if = malloc(sizeof(char)*(strlen(wlan_if)+1));
  strcpy(__wlan_if, wlan_if);
  return 1; // ok!
}

void close_wireless_info() {
  if (__frssi) fclose(__frssi);
}

int get_rssi(float * link, float * level, float * noise) {
  char wlan[10], status[4], link_str[6], level_str[6], noise_str[6];
  wlan[0] = '\0';

  *link = *level = *noise = -1; // error, if interface isn't found

  skip_first_two_lines();

  bool found_wlan_if = false;
  /* Check if is the there is a wlan interface = wlan_if */
  while ( ! feof(__frssi) && ! found_wlan_if ) {
    if ( fscanf(__frssi,"%s %s %s %s %s", wlan, status, link_str, level_str, noise_str) != 5 ){
       return -1; // conversion error
    }
    if (strncmp(__wlan_if, wlan, strlen(__wlan_if)) == 0 ) {
       found_wlan_if = true;
       *link = atof(link_str);
       *level = atof(level_str);
       *noise = atof(noise_str);
       return 0;
    }
  }
  return -2; // wireless interface not found
}