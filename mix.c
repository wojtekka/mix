/*
  mix, wersja 0.4
  (c) 1998, 1999 wojtek kaniewski <wojtekka@dione.ids.pl>

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
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include "slang.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/soundcard.h>

#define DEVMIXER "/dev/mixer"
#define VERSION "mix-0.4"
#define DEVICES SOUND_MIXER_NRDEVICES

#define slprintf(x...) { snprintf(pftmp, 255, x); \
  SLtt_write_string(pftmp); SLtt_flush_output(); }
  
/* deklarujemy co trzeba */
void merror(char *msg);
void usage(char *argv0);
void finish();
char pftmp[256];
int mixfd, opt_help = 0, opt_version = 0, mini = 0, ile = 0, dev = 0, pl;
mixer_info minfo;

static struct option const longopts[] = {
  { "help", no_argument, &opt_help, 1 },
  { "version", no_argument, &opt_version, 1 },
  { "device", required_argument, 0, 'd' },
  { "mini", no_argument, 0, 'm' },
  { 0, 0, 0, 0 }
};

/* i wziuuum... do dzie³a :) */
void main(int argc, char **argv) {
  int x, tmp, ch, update = 0, finito = 0, optc;
  int oldvol[DEVICES], vol[DEVICES], devs[DEVICES];
  char *labels[DEVICES] = SOUND_DEVICE_LABELS, *devmixer = NULL;
  unsigned long devmask;

  /* piszemy po polsku? */
  pl = !strncasecmp(getenv("LANG"), "pl", 2);
  
  /* sprawd¼ parametry */
  while((optc=getopt_long(argc, argv, "hvd:m", longopts, (int*) 0)) != EOF) {
    switch(optc) {
      case 'd':
        devmixer = strdup(optarg);
	break;
      case 'm':
        mini = 1;
	break;
      default:
        usage(argv[0]);
    }
  }
  if(opt_version) {
    printf(VERSION ", (c) 1998, 1998 by wojtek kaniewski (wojtekka@dione.ids.pl)\n");
    exit(0);
  }
  if(opt_help) {
    usage(argv[0]);
  }

  /* inicjalizacja biblioteki slang */
  SLtt_get_terminfo();
  if(SLang_init_tty(3, 0, 0) == -1) merror("slang: SLang_init_tty()");
  if(SLkp_init() == -1) merror("slang: SLkp_init()");

  /* inicjalizacja miksera */
  if((mixfd=open(devmixer ? devmixer : DEVMIXER, O_RDWR)) == -1) {
    snprintf(pftmp, 255, pl ? "Nie mo¿na otworzyæ %s" : "Can't open %s",
      devmixer ? devmixer : DEVMIXER);
    merror(pftmp);
  }
  if(ioctl(mixfd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1 || !devmask)
    merror("ioctl: SOUND_MIXER_READ_DEVMASK");
  if(ioctl(mixfd, SOUND_MIXER_INFO, &minfo) == -1)
    merror("ioctl: SOUND_MIXER_INFO");

  /* wizytówka */
  if(!mini) slprintf("%s (%s)\r\n", minfo.name, VERSION);
  
  /* ustaw reakcjê na sygna³y */
  signal(SIGTERM, finish);
  signal(SIGINT, finish);

  /* odczytanie aktualnych ustawien miksera */
  for(x=0; x<DEVICES; x++) {
    if(!(devmask & (1 << x))) continue;
    devs[ile]=x;
    if(ioctl(mixfd, MIXER_READ(x), &oldvol[ile]) == -1)
      merror("ioctl: MIXER_READ");
    vol[ile]=((oldvol[ile] & 255)+(oldvol[ile] >> 8))/20;
    if(vol[ile] > 10) vol[ile]=10;
    if(vol[ile] < 0) vol[ile]=0;
    if(!mini) slprintf("%s%s <-----------> %d%%  \r\033[%dC\033[1m#\033[0m"
    "\033[D", ile ? "\r\n" : "", labels[devs[ile]], vol[ile]*10, vol[ile]+7);
    ile++;
  }
  if (!mini) slprintf("\033[%dA", ile-1);
  
  /* g³ówna pêtla */
  do {
    /* poka¿ aktualne ustawienie miksera */
    slprintf("\r%s <-----------> %d%%  \r\033[%dC\033[1m#\033[0m\033[D",
      labels[devs[dev]], vol[dev]*10, vol[dev]+7);

    /* jaki¶ klawisz? rozpatrzmy podanie... */
    ch=SLkp_getkey();
    switch(ch) {
      /* poprzednia wartosc */
      case SL_KEY_UP:
        if(dev==0) break;
	dev--;
	if (!mini) slprintf("\033[A");
	break;
      case SL_KEY_DOWN:
        if(dev==ile-1) break;
        dev++;
	if (!mini) slprintf("\033[B");
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
      /* zakoñcz i zachowaj */
      case 13:
	finito=1;
        break;
      /* SLang jest gupi i nie ma klawisza esc, wiec wykorzystamy to... */
      case SL_KEY_ERR:
        for(x=0; x<ile; x++) ioctl(mixfd, MIXER_WRITE(devs[x]), &oldvol[x]);
	finito=1;
	update=0;
	break;
      /* jaki¶ klawisz, który nam nie pasuje */
      default:
        SLtt_beep();
    }
    /* czy musimy zaktualizowaæ ustawienia miksera? */
    if(update) {
      for(x=0; x<ile; x++) {
	tmp=vol[x]*10+((vol[x]*10) << 8);
        if(ioctl(mixfd, MIXER_WRITE(devs[x]), &tmp) == -1)
	  merror("ioctl: SOUND_MIXER_WRITE_VOLUME");
      }
      update=0;
    }
  } while(!finito);

  /* koñczymy zabawê */
  finish();
}

/* zamyka wszystko */
void finish() {
  close(mixfd);
  if(mini) {
    slprintf("\r");
    SLtt_del_eol();
    slprintf("%s (%s)\r\n", minfo.name, VERSION);
  } else {
    slprintf("\033[%dB\r\n", ile-dev);
    SLtt_del_eol();
  }
  SLang_reset_tty();
  exit(1);
}

/* wywala komunikat o b³êdzie i sprz±ta po sobie */
void merror(char *msg) {
  close(mixfd);
  fprintf(stderr, "\r");
  SLtt_del_eol();
  fprintf(stderr, "%s: %s\r\n", pl ? "B£¡D" : "ERROR", msg);
  SLang_reset_tty();
  exit(1);
}

/* informacje o sposobie u¿ycia */
void usage(char *argv0) {
  if(pl) printf("\
U¿ycie: %s [OPCJE]\n\
  -d, --device=URZ¡DZENIE  okre¶la u¿ywane urz±dzenie miksera\n\
  -m, --mini               uruchamia program w trybie jednolinijkowym\n\
      --version            pokazuje wersjê programu\n\
      --help               wy¶wietla listê dostêpnych opcji\n\
", argv0); else printf("\
Usage: %s [OPTIONS]\n\
  -d, --device=DEVICE  specifies mixer device that should be used\n\
  -m, --mini           runs program in one-line mode\n\
      --version        shows program version\n\
      --help           prints list of avaiable options\n\
", argv0);
  exit(0);
}