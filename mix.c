/*
  mix, wersja 0.6
  (c) 1998, 1999 wojtek kaniewski <wojtekka@irc.pl>
  
  fixy: marcin kami�ski <maxiu@px.pl>

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
#define DEVICES SOUND_MIXER_NRDEVICES

#ifndef MINI
#  define VERSION "mix-0.6"
#else
#  define VERSION "mix-0.6-mini"
#endif

#define COPYRITE "(c) 1998, 1999 by wojtek kaniewski (wojtekka@irc.pl)"

/* deklarujemy co trzeba */
void merror(char *format, ...);
void usage(char *argv0);
void slprintf(char *format, ...);
void finish();

int mixfd, ile = 0, dev = 0, pl;
mixer_info minfo;
#ifndef MINI
int mini = 0;
#else
int mini = 1;
#endif

static struct option const longopts[] = {
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'v' },
  { "device", required_argument, 0, 'd' },
  { "mini", no_argument, 0, 'm' },
  { "big", no_argument, 0, 'b' },
  { 0, 0, 0, 0 }
};

/* i wziuuum... do dzie�a :) */
int main(int argc, char **argv) {
  int x, tmp, ch, update = 0, finito = 0, optc;
  int oldvol[DEVICES], vol[DEVICES], devs[DEVICES];
  char *labels[DEVICES] = SOUND_DEVICE_LABELS, *devmixer = DEVMIXER, *lang;
  unsigned long devmask;

  /* piszemy po polsku? locale ss� (SEGFAULT fix by maxiu) */
  lang = getenv("LANG");
  pl = (!strncasecmp(lang, "pl", 2)) ? 1 : 0;
  
  /* sprawd� parametry */
  while ((optc = getopt_long(argc, argv, "hvd:mb", longopts, NULL)) != EOF) {
    switch (optc) {
      case 'd':
        devmixer = strdup(optarg);
	break;
      case 'm':
        mini = 1;
	break;
      case 'b':
        mini = 0;
	break;
      case 'v':
        printf(VERSION ", " COPYRITE "\n");
        exit(0);
      default:
        usage(argv[0]);
    }
  }

  /* inicjalizacja biblioteki slang */
  SLtt_get_terminfo();
  if ((SLang_init_tty(3, 0, 0) == -1) || (SLkp_init() == -1))
    merror(pl ? "B��d inicjalizacji terminala" : "Terminal initialization error");

  /* inicjalizacja miksera */
  if ((mixfd = open(devmixer, O_RDWR)) == -1)
    merror(pl ? "Nie mo�na otworzy� %s" : "Can't open %s", devmixer);
  if(ioctl(mixfd, SOUND_MIXER_READ_DEVMASK, &devmask) == -1 || !devmask)
    merror("ioctl: SOUND_MIXER_READ_DEVMASK");
  if(ioctl(mixfd, SOUND_MIXER_INFO, &minfo) == -1)
    merror("ioctl: SOUND_MIXER_INFO");

  /* wizyt�wka */
  if (!mini) slprintf("%s /%s/\r\n", VERSION, minfo.name);
  
  /* ustaw reakcj� na sygna�y */
  signal(SIGTERM, finish);

  /* odczytanie aktualnych ustawien miksera */
  for (x = 0; x < DEVICES; x++) {
    if (!(devmask & (1 << x))) continue;
    devs[ile] = x;
    if (ioctl(mixfd, MIXER_READ(x), &oldvol[ile]) == -1)
      merror("ioctl: MIXER_READ");
    vol[ile] = ((oldvol[ile] & 255) + (oldvol[ile] >> 8)) / 20;
    if (vol[ile] > 10) vol[ile] = 10;
    if (vol[ile] < 0) vol[ile] = 0;
    if (!mini) slprintf("%s%s <-----------> %d%%\r%s%s#%s%s", ile ? "\r\n" : "", labels[devs[ile]], vol[ile] * 10, right(vol[ile] + 7), bold(), normal(), left(1));
    ile++;
  }
  if (!mini) slprintf("%s", up(ile - 1));
  
  while (!finito) {
    /* poka� aktualne ustawienie miksera */
    slprintf("\r%s <-----------> %d%%  \r%s%s#%s%s",
      labels[devs[dev]], vol[dev] * 10, right(vol[dev] + 7), bold(), normal(), left(1));

    /* jaki� klawisz? rozpatrzmy podanie... */
    ch = SLkp_getkey();
    switch (ch) {
      case SL_KEY_UP:
        if (!dev) break;
	dev--;
	if (!mini) slprintf("%s", up(1));
	break;
      case SL_KEY_DOWN:
        if (dev == (ile - 1)) break;
        dev++;
	if (!mini) slprintf("%s", down(1));
	break;
      case '[': /* min */
        vol[dev] = 0;
	update = 1;
	break;
      case SL_KEY_LEFT: case '-': case ',': case '<':
	if (vol[dev]) vol[dev]--;
	update = 1;
	break;
      case ']': /* max */
        vol[dev] = 10;
	update = 1;
	break;
      case SL_KEY_RIGHT: case '+': case '.': case '>':
        if (vol[dev] < 10) vol[dev]++;
	update = 1;
	break;
      /* zako�cz i zachowaj */
      case 13:
	finito = 1;
        break;
      /* i tak nie dzia�a :) */
      case SL_KEY_ERR:
        for (x = 0; x < ile; x++)
	  ioctl(mixfd, MIXER_WRITE(devs[x]), &oldvol[x]);
	finito = 1;
	update = 0;
	break;
      /* jaki� klawisz, kt�ry nam nie pasuje */
      default:
        SLtt_beep();
    }
    /* czy musimy zaktualizowa� ustawienia miksera? */
    if (!update)
      continue;
    for (x = 0; x < ile; x++) {
      tmp = vol[x] * 10 + ((vol[x] * 10) << 8);
      if (ioctl(mixfd, MIXER_WRITE(devs[x]), &tmp) == -1)
        merror("ioctl: SOUND_MIXER_WRITE_VOLUME");
    }
    update = 0;
  }

  /* ko�czymy zabaw� */
  finish();
  return 0;  
}

/* zamyka wszystko */
void finish() 
{
  close(mixfd);
  if (mini) {
    slprintf("\r");
    SLtt_del_eol();
    slprintf("%s /%s/\r\n", minfo.name, VERSION);
  } else {
    slprintf("%s\r\n", down(ile - dev - 1));
    SLtt_del_eol();
  }
  SLang_reset_tty();
  exit(1);
}

/* wywala komunikat o b��dzie i sprz�ta po sobie */
void merror(char *format, ...)
{
  char buf[256];
  va_list va;
  
  va_start(va, format);
  vsnprintf(buf, 255, format, va);
  fprintf(stderr, "\r");
  SLtt_del_eol();
  fprintf(stderr, "%s: %s\r\n", pl ? "B��D" : "ERROR", buf);

  SLang_reset_tty();
  close(mixfd);
  exit(1);
}

/* informacje o sposobie u�ycia */
void usage(char *argv0) 
{
  if(pl) printf("\
U�ycie: %s [OPCJE]\n\
  -d, --device=URZ�DZENIE  okre�la u�ywane urz�dzenie miksera\n\
  -m, --mini               uruchamia program w trybie jednolinijkowym...\n\
  -b, --big                ...lub normalnym (przydane w wersji 'mini')\n\
      --version            pokazuje wersj� programu\n\
      --help               wy�wietla list� dost�pnych opcji\n\
", argv0); else printf("\
Usage: %s [OPTIONS]\n\
  -d, --device=DEVICE  tells which mixer device should be used\n\
  -m, --mini           runs program in one-line mode...\n\
  -b, --big            ...or in normal mode (useful for 'mini' version)\n\
      --version        shows program version\n\
      --help           prints list of avaiable options\n\
", argv0);
  exit(0);
}

void slprintf(char *format, ...)
{
  char buf[256];
  va_list va;

  va_start(va, format);
  vsnprintf(buf, 256, format, va);
  va_end(va);
  SLtt_write_string(buf);
  SLtt_flush_output();
}
