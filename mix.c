/*
  mix, wersja 0.2
  (c) 1998 wojtek kaniewski <wojtekka@logonet.com.pl>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
                
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
                               
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "slang.h"

#ifdef linux
#	include <sys/ioctl.h>
#	include <sys/types.h>
#	include <sys/soundcard.h>
#	define MIXER
#endif

#ifdef __FreeBSD__
#	include "sys/ioctl.h"
#	include <sys/types.h>
#	include "machine/soundcard.h"
#	define MIXER
#endif

#ifndef MIXER
** sorry, your machine does not support OSS mixer
#endif

#define VERSION "mix-0.2"

/* deklarujemy co trzeba */
void merror(char *msg);
mixer_info minfo;
char *devices[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_LABELS;
int mixfd, dev = -1, oldvol[SOUND_MIXER_NRDEVICES], vol[SOUND_MIXER_NRDEVICES];
unsigned long devmask;

/* i wziuuum... do dzie³a :) */
void main(int argc, char **argv) {
  int x, tmp, ch, update = 0, finito = 0;
  char ctmp[256];

  /* inicjalizacja biblioteki slang */
  SLtt_get_terminfo();
  if(SLang_init_tty(3, 0, 0) == -1) merror("SLang_init_tty()");
  if(SLkp_init() == -1) merror("SLkp_init()");

  /* inicjalizacja miksera */
  if((mixfd=open("/dev/mixer", O_RDWR)) < 0)
    merror("Nie mo¿na otworzyæ /dev/mixer");
  if(ioctl(mixfd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1)
    merror("SOUND_MIXER_READ_DEVMASK");
  if(!devmask)
    merror("Mikser nie pozwala na regulacjê jakichkolwiek ustawieñ");
  if(ioctl(mixfd, SOUND_MIXER_INFO, &minfo) == -1)
    merror("SOUND_MIXER_INFO");

  /* odczytanie aktualnych ustawien miksera */
  while(!(devmask & (1 << ++dev)));
  for(x=0; x<SOUND_MIXER_NRDEVICES; x++) {
    if(!(devmask & (1 << x))) continue;
    if(ioctl(mixfd, MIXER_READ(x), &oldvol[x]) == -1) merror("MIXER_READ");
    vol[x]=((oldvol[x] & 255)+(oldvol[x] >> 8))/20;
    if(vol[x] > 10) vol[x]=10;
    if(vol[x] < 0) vol[x]=0;
  }
  
  /* infinite loop? naaah... ;) */
  do {
    /* poka¿ aktualne ustawienie miksera */
    sprintf(ctmp, "\r%s <-----------> %d%%  \r\x1B[%dC", devices[dev], vol[dev]*10, vol[dev]+7);
    SLtt_write_string(ctmp);
    SLtt_bold_video();
    SLtt_write_string("#\x1B[D");
    SLtt_normal_video();
    SLtt_flush_output();

    /* jaki¶ klawisz? rozpatrzmy podanie... */
    ch=SLkp_getkey();
    switch(ch) {
      /* poprzednia wartosc */
      case SL_KEY_UP:
        if(dev==0) break;
	while(!(devmask & (1 << --dev)));
	break;
      case SL_KEY_DOWN:
        if(dev==SOUND_MIXER_NRDEVICES-1) break;
	x=dev;
	while(!((devmask & (1 << ++dev)) || (dev == SOUND_MIXER_NRDEVICES)));
	if(dev == SOUND_MIXER_NRDEVICES) dev=x;
	break;
      /* minimalna g³o¶no¶æ */
      case '[':
        vol[dev]=0;
	update=1;
	break;
      /* zmniejszenie g³o¶no¶ci */
      case SL_KEY_LEFT: case '-': case ',': case '<':
	if(vol[dev]>0) vol[dev]--;
	update=1;
	break;
      /* maksymalna g³o¶no¶æ */
      case ']':
        vol[dev]=10;
	update=1;
	break;
      /* zwiêkszenie g³o¶no¶ci */
      case SL_KEY_RIGHT: case '+': case '.': case '>':
        if(vol[dev]<10) vol[dev]++;
	update=1;
	break;
      /* klawisze, które powoduj± zakoñczenie programu */
      case 13:
	finito=1;
        break;
      /* jaki¶ klawisz, który nam nie pasuje */
      case SL_KEY_ERR:
      default:
        SLtt_beep();
    }
    /* czy musimy zaktualizowaæ ustawienia miksera? */
    if(update) {
      for(x=0; x < SOUND_MIXER_NRDEVICES; x++) {
        if(!(devmask & (1 << x))) continue;
	tmp=vol[x]*10+((vol[x]*10) << 8);
        if(ioctl(mixfd, MIXER_WRITE(x), &tmp) == -1) merror("SOUND_MIXER_WRITE_VOLUME");
      }
      update=0;
    }
  } while(!finito);

  /* koñczymy zabawê */
  close(mixfd);
  printf("\r");
  SLtt_del_eol();
  printf(VERSION " (%s)\r\n", minfo.name);
  SLang_reset_tty();
}

/* wywala komunikat o b³êdzie i sprz±ta po sobie */
void merror(char *msg) {
  fprintf(stderr, "\nB£¡D: %s\n", msg);
  close(mixfd);
  SLang_reset_tty();
}
