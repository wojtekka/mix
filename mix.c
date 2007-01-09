/*
  mix, wersja 0.3
  (c) 1998 wojtek kaniewski <wojtekka@dione.ids.pl>

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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/soundcard.h>

#define DEVMIXER "/dev/mixer"
#define VERSION "mix-0.3"
#define DEVICES SOUND_MIXER_NRDEVICES

/* deklarujemy co trzeba */
void merror(char *msg);
int mixfd;

/* i wziuuum... do dzie³a :) */
void main(int argc, char **argv) {
  int x, ile = 0, dev = 0, tmp, ch, update = 0, finito = 0;
  int oldvol[DEVICES], vol[DEVICES], devs[DEVICES];
  char *labels[DEVICES] = SOUND_DEVICE_LABELS;
  unsigned long devmask;
  mixer_info minfo;
  char ctmp[256];

  /* inicjalizacja biblioteki slang */
  SLtt_get_terminfo();
  if(SLang_init_tty(3, 0, 0) == -1) merror("SLang_init_tty()");
  if(SLkp_init() == -1) merror("SLkp_init()");

  /* inicjalizacja miksera */
  if((mixfd=open(DEVMIXER, O_RDWR)) < 0)
    merror("Nie mo¿na otworzyæ " DEVMIXER);
  if(ioctl(mixfd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1)
    merror("SOUND_MIXER_READ_DEVMASK");
  if(!devmask)
    merror("Mikser nie zezwala na regulacjê jakichkolwiek ustawieñ");
  if(ioctl(mixfd, SOUND_MIXER_INFO, &minfo) == -1)
    merror("SOUND_MIXER_INFO");

  /* wizytówka */
  printf(VERSION " (urz±dzenie: %s)\r\n", minfo.name);
  
  /* odczytanie aktualnych ustawien miksera */
  for(x=0; x<SOUND_MIXER_NRDEVICES; x++) {
    if(!(devmask & (1 << x))) continue;
    devs[ile]=x;
    if(ioctl(mixfd, MIXER_READ(x), &oldvol[ile]) == -1) merror("MIXER_READ");
    vol[ile]=((oldvol[ile] & 255)+(oldvol[ile] >> 8))/20;
    if(vol[ile] > 10) vol[ile]=10;
    if(vol[ile] < 0) vol[ile]=0;
    snprintf(ctmp, 255, "%s%s <-----------> %d%%  \r\033[%dC", ile ? "\r\n" : "", labels[devs[ile]], vol[ile]*10, vol[ile]+7);
    SLtt_write_string(ctmp);
    SLtt_bold_video();
    SLtt_write_string("#\033[D");
    SLtt_normal_video();
    SLtt_flush_output();
    ile++;
  }
  snprintf(ctmp, 255, "\033[%dA", ile-1);
  SLtt_write_string(ctmp);
  
  /* infinite loop? naaah... ;) */
  do {
    /* poka¿ aktualne ustawienie miksera */
    snprintf(ctmp, 255, "\r%s <-----------> %d%%  \r\033[%dC", labels[devs[dev]], vol[dev]*10, vol[dev]+7);
    SLtt_write_string(ctmp);
    SLtt_bold_video();
    SLtt_write_string("#\033[D");
    SLtt_normal_video();
    SLtt_flush_output();

    /* jaki¶ klawisz? rozpatrzmy podanie... */
    ch=SLkp_getkey();
    switch(ch) {
      /* poprzednia wartosc */
      case SL_KEY_UP:
        if(dev==0) break;
	dev--;
	SLtt_write_string("\033[A");
	SLtt_flush_output();
	break;
      case SL_KEY_DOWN:
        if(dev==ile-1) break;
        dev++;
	SLtt_write_string("\033[B");
	SLtt_flush_output();
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
      for(x=0; x<ile; x++) {
	tmp=vol[x]*10+((vol[x]*10) << 8);
        if(ioctl(mixfd, MIXER_WRITE(devs[x]), &tmp) == -1) merror("SOUND_MIXER_WRITE_VOLUME");
      }
      update=0;
    }
  } while(!finito);

  /* koñczymy zabawê */
  close(mixfd);
  printf("\033[%dB\r\n", ile-dev);
  SLtt_del_eol();
  SLang_reset_tty();
}

/* wywala komunikat o b³êdzie i sprz±ta po sobie */
void merror(char *msg) {
  fprintf(stderr, "\nB£¡D: %s\n", msg);
  close(mixfd);
  SLang_reset_tty();
}
