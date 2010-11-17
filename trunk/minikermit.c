// a terminal program for serial communication and upload programs at microcontroller
/*
* See the file "license.terms" for information on usage and redistribution
* of this file.
*/
/* minikermit Version 0.9.7	101101 -> works with arduino bootloader */
/* minikermit Version 0.9.6	091126 -> atmega1280 isp sync problem*/
/* minikermit Version 0.9.5	090601 -> test usb device number*/
/* minikermit Version 0.9.4	090105 -> empty page error*/
/* minikermit Version 0.9.3	080331 -> problems with upload sync*/
/* minikermit Version 0.9.2	061228*/
/* mini-kermit	Version 0.9	061203*/
/* mini-kermit	Version 0.7	040615*/
/* mini-kermit	Version 0.6	040615*/

// how to use:
// set MACROS TARGET BAUDRATE MODEMDEVICE and SCANSERIALDEVICES in this file
// g++ minikermit.c -o minikermit -lncurses
// connect with target and start: #>./minikermit

/* for Z80-Mini and 68HC11-Zwerg (EVB) and CharonII and ArduinoMega and MRT54g, NGW100, STK1000, EVK1100 and ...*/
/* adapted for z80-mini hex-file downloading*/
/* and 6811 s19-File downloading*/
/* and AVR cob(bin) file downloading*/

/* minikermit works in 2 modes*/
/* ECHOMODE 	- write out all typed chars , break with CTRL-C only */
/* NONECHOMODE 	- for monitor (bamo)		*/
/*		- q-quit , w download file	*/
/* it starts in NONECHOMODE*/
/* target-system can change mode with TOGGLEECHOMODE (echomode/nonechomode)*/
/* corrected by: Yves Vinzing*/
/* what: correction of filename possible by pressing 'Backspace'*/
/*	bh: excl-lock on serial device  	051107	*/
/*	bh: charonII und mrt54g    		051229	*/
/**************************************************************************************************************/
// select your target processor or better your board 
// #define CPU68HC11
// #define CPUZ80
// #define AVR	 // diverse atmega, xmega, UC3, AP7000 working in cooperation with bamo128 or bamo32 and minikermit
		// or any processor with bajos and minikermit
#define ARDUINOBOOTLOADER	 // any processor with arduino bootloader

#define SCANSERIALDEVICES	// test for /dev/ttyUSB0,1,2,..9 or ACM0,1,2,..9

// select your serial device
//#define MODEMDEVICE "/dev/ttyACM0"	/* 11-th position is 0*/
//#define MODEMDEVICE 	"/dev/ttyS0"	/* first RS232 port*/
#define MODEMDEVICE 	"/dev/ttyUSB0"	/* USB port*/
//#define MODEMDEVICE 	"/dev/ttyUSB1"	/* USB port*/
//#define MODEMDEVICE 	"/dev/ttyS1"	/* second RS232 port*/

// select your baudrate
//#define BAUDRATE	B1200
//#define BAUDRATE	B9600
//#define BAUDRATE	B19200
//#define BAUDRATE	B38400
#define BAUDRATE 	B57600
//#define BAUDRATE	B115200
/**************************************************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <curses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/file.h>

using namespace std;

/*  RS323 control-lines */
void setAllOnHigh		(int fdSerial);
void resetInExpandedMode	(int fdSerial);
void resetAndSetBootMode	(int fdSerial);
void resetAndSetExpandedMode	(int fdSerial);

void upLoadFile		(int fdSerial, char order);
bool testEmptyPage	(FILE* file);

#define NAMELENGTH	48       /* max. length of filename*/
#define	DELAY		100	/* atmega1280 sync problem */

#define TOGGLEECHOMODE	0x03
#define SUPRESSSERINPUT	0x04
#define ACCEPTSERINPUT	0x05

#define _POSIX_SOURCE 	1

#define LF		0x0A
#define CR		0x0D
#define ESC		0x1B
#define BS           	0x7F

bool 	echoMode 	= false;
bool	readyNow	= false;
bool	supressSerInput = false;

struct sigaction  act;

void catchSIGHUP(int signo)	{ echoMode = !echoMode;	}
void catchSIGUSR1(int signo)	{ readyNow = true;	}

int main(int argc, char * argv[])	{
  int		fdSerial;					/* for serial*/
  termios 	oldTio,newTio;
  char		cFromSerial,cFromKeypad,deviceNumber;
  pid_t		pid;

  sigemptyset(&act.sa_mask);	/* prepare signal processing */
  act.sa_flags	= SA_RESTART;
  act.sa_handler= catchSIGHUP;
  sigaction(SIGHUP,&act,NULL);
  act.sa_handler= catchSIGUSR1;
  sigaction(SIGUSR1,&act,NULL);
#ifdef SCANSERIALDEVICES	// open serail device
  char modem[12];
  strcpy(modem,MODEMDEVICE);
  for (deviceNumber=0; deviceNumber < 10; deviceNumber++) 	{
// test for usb-modemdevice 0 .. 9
// remove it for serial and other fix modem device strings
	modem[11]=deviceNumber+'0';
  	if ((fdSerial=open(modem,O_RDWR|O_NOCTTY))>0) break;
	printf("%s\n",modem);
								}
  modem[11]='?';
  if (deviceNumber == 10)	{perror(modem); exit(-1);}
#else
if ((fdSerial=open(MODEMDEVICE,O_RDWR|O_NOCTTY))>0){perror(MODEMDEVICE); exit(-1);}
#endif
  if (flock(fdSerial, LOCK_EX|LOCK_NB) <0)
	{printf("can't LOCK_EX -> perhaps runs another %s\n",argv[0]);exit(-6);	}
											 /* Locking routines for 4.3BSD.	*/

  tcgetattr(fdSerial,&oldTio);	// prepare terminal
  newTio		= oldTio;
  newTio.c_cflag	= BAUDRATE|CS8|CLOCAL|CREAD;
  newTio.c_iflag	= IGNPAR;
  newTio.c_oflag	= 0;
  newTio.c_lflag	= 0;	/* non canonical mode (char mode)*/
  newTio.c_cc[VTIME]	= 0;	/* blocked*/
  newTio.c_cc[VMIN]	= 1;	/* return, when 1 byte available*/
  tcflush(fdSerial,TCIFLUSH);
  tcsetattr(fdSerial,TCSANOW,&newTio);

#ifdef CPU68HC11
  setAllOnHigh(fdSerial);
  resetInExpandedMode(fdSerial);
#endif

  WINDOW* mywin = new WINDOW;	/* !ncurses library!*/
  mywin		= initscr();
  refresh();

  if (pid = fork())	{             /* create process	*/
      while (1)		       {         /* the parent */
	  cin.get(cFromKeypad);	      	/* get 1 byte from pc-console */
	  if (((cFromKeypad=='q')||(cFromKeypad=='Q')) && !echoMode)	{
	  	kill(pid,SIGKILL);	/* kill child */
	      	wait(NULL);
	      	break;							} // terminate
	  if ((cFromKeypad=='w') && !echoMode)			{
		upLoadFile(fdSerial,'l');
		continue;					} /* end of (if cFromKeypad='w'...) */
	  write(fdSerial,&cFromKeypad,1); 			  /* write 1 byte to serial */
 				} 			 	  /* end of while(1) */
  			}	 			 /* end of (if (pid=fork() ...) */
  else
	while (1)		{		/* child */
		read(fdSerial,&cFromSerial,1);		/* read serial 1 byte */
#ifdef ARDUINOBOOTLOADER
		if (cFromSerial == 0x14)		continue;	// wait for 0x10
#endif
		if (cFromSerial == TOGGLEECHOMODE)	{ kill(getppid(),SIGHUP); continue; 	}
		if (cFromSerial == SUPRESSSERINPUT)	{ supressSerInput=true;   continue; 	}

#ifdef ARDUINOBOOTLOADER
		if (cFromSerial == 0x10)		{kill(getppid(),SIGUSR1);/* ready now */continue;}
#else
		kill(getppid(),SIGUSR1);	/* ready now */
#endif
		if (!supressSerInput)			{ cout << cFromSerial; cout.flush(); 	}  /* write in pc-window,  flush buffer */
		if (cFromSerial ==  ACCEPTSERINPUT)	{ supressSerInput=false;  continue;	}
				}	// end while(1)	
  tcsetattr(fdSerial,TCSANOW,&oldTio);	// 
  endwin();				/* !ncurses library!*/
  if (flock(fdSerial, LOCK_UN|LOCK_NB) <0)	{ printf("can't UNLOCK\n");exit(-6);	}
  close(fdSerial);
} /* end of int main(...)*/


/* High is low and Low is High !!!*/
/* das ist das kleine Hexeneinmaleins*/
void setAllOnHigh(int fdSerial)		{
int iostatus;
    ioctl(fdSerial,TIOCMGET,&iostatus);
    iostatus&=~TIOCM_DTR&~TIOCM_RTS;	/* DTR high, RTS high*/
    ioctl(fdSerial,TIOCMSET,&iostatus);	}

void resetInExpandedMode(int fdSerial)	{
    setAllOnHigh(fdSerial);
    resetAndSetExpandedMode(fdSerial);	}

void resetAndSetBootMode(int fdSerial)	{
int iostatus;
    ioctl(fdSerial,TIOCMGET,&iostatus);
    iostatus|=TIOCM_DTR|TIOCM_RTS;	/*DTR low, RTS low*/
    ioctl(fdSerial,TIOCMSET,&iostatus);
    usleep(100);
    setAllOnHigh(fdSerial);		}

void resetAndSetExpandedMode(int fdSerial)	{
int iostatus;
    ioctl(fdSerial,TIOCMGET,&iostatus);
    iostatus|=TIOCM_DTR;		/* DTR low, RTS high*/
    iostatus&=~TIOCM_RTS;
    ioctl(fdSerial,TIOCMSET,&iostatus);
    usleep(100);
    setAllOnHigh(fdSerial);			}

void upLoadFile(int fdSerial, char order)	{
FILE* 	file;			/* for upload of file*/
char  	fileName[NAMELENGTH+1];
char 	c;
char*	command;

#ifdef CPU68HC11
	      command="LOAD T";
	      write(fdSerial,command,6);
	      c=CR;
	      write(fdSerial,&c,1);
	      sleep(1);   
	      cout << "name of s19 file for uploading:\t";
#endif
#ifdef CPUZ80
	      write(fdSerial,&order,1); /* send order to serial*/
	      c=CR;
	      write(fdSerial,&c,1);
	      sleep(1);   
	      /*gendert fr den BAMO80*/
	      cout << "name of hex-file for uploading:\t";
#endif
#ifdef AVR
	      c='w';							/* order*/
	      write(fdSerial,&c,1);
	      sleep(1);   
	      cout << "name of cob (bin) -file for uploading:\t";
#endif
#ifdef ARDUINOBOOTLOADER
	      cout << "name of binary -file for uploading:\t";
#endif
	      cout.flush();
	      int i;
	      for (i=0;i<NAMELENGTH;i++) {
		cin.get(c);	    	/* *** new: get 1 byte from console*/
		if (c == CR ) break;  /* *** new: Break on Enter*/
		if ((c == BS) && (i>=1)) /* *** new: Delete one character if i>0 ****/
		           {i-=2;c=8;cout<<c<<' '<<c;continue;}
		if ((c == ' ') || (c == BS)) {i--;cout<< c;continue;} 		/* *** new: Handling for Space and BS ****/
		cout << c;
		cout.flush();
		fileName[i]=c;		}
	      c=LF;
	      cout << c;
	      c=CR;
#if  CPU68HC11 || CPUZ80
		write(fdSerial,&c,1);
		cout << c;
#endif
		fileName[i]='\0';
		cout.flush();	
		usleep(100);
		/* try to open file and send it to serial, abort on error*/
		if ((file=fopen(fileName,"r")) == NULL)	{
		    perror(fileName);
		    c=CR;
		    cout << c;
		    cout.flush();
		    c=ESC;	
		    write(fdSerial,&c,1);
		    return;			}

#if ((!defined(AVR)) && (!defined(ARDUINOBOOTLOADER)))
		    while (1)	{
			if (feof(file)) break;
			fread(&c,1,1,file); /* read a char from file*/
			write(fdSerial,&c,1);	    /* write to serial*/
				}
#endif

#if defined(AVR) || defined(ARDUINOBOOTLOADER)
long  pos;
fseek(file,0L,SEEK_END);
pos=ftell(file);
int pages=pos/(128*2);
int bytesInLastPage=pos%256;
if (pages >=(64*8-4*8)||(pos==0))	{	/*??????????*/
	cout << " program too long,  overwrites bootsection! - or perhaps no bytes in file??\n";
	cout.flush();
	fclose(file);	
    	c=ESC;
	 write(fdSerial,&c,1);
	return;				}
rewind(file);
#endif

#ifdef ARDUINOBOOTLOADER
unsigned short int address=0;
cout << "\r\n";
resetInExpandedMode(fdSerial);
usleep(100);
int p;
for (p=0;p<pages;p++)			{
  c='U';	// write flash start address
write(fdSerial,&c,1);
write(fdSerial,&address,1);		// little endian
write(fdSerial,(unsigned char*)&address+1,1);	// big endian startaddress is 0000 in words
address+=0x80;
readyNow=false;
c=' ';
write(fdSerial,&c,1);
while (!readyNow);		// wait for 0x14 0x10
c='d';				// write flash 
write(fdSerial,&c,1);
c=0x1;
write(fdSerial,&c,1);		// big endian
c=0;
write(fdSerial,&c,1);		// little endian page size in bytes
c='F';				// i think its so for flash
write(fdSerial,&c,1);
for (int numByte=0; numByte < 256;numByte++)		{
	fread(&c,1,1,file);	/* read a char from file*/
	write(fdSerial,&c,1);	/* write to serial */
							}
readyNow=false;
c=' '; 				// now write it
write(fdSerial,&c,1);
cout <<"page:\t"<<p<<" from "<<pages<<" pages written\r";
cout.flush();
while (!readyNow);		// wait for 0x14 0x10
}

/* the same for the Rest */
int rest=0;
if	( bytesInLastPage!=0)		{
c='U';	// write flash start address
write(fdSerial,&c,1);
write(fdSerial,&address,1);		// little endian
write(fdSerial,((unsigned char*)&address)+1,1);	// big endian startaddress is 0000 in words
readyNow=false;
c=' ';
write(fdSerial,&c,1);
while (!readyNow);	// wait for 0x14 0x10
c='d';	// write flash 
write(fdSerial,&c,1);
c=0x01;
write(fdSerial,&c,1);	// big endian
c=0;
write(fdSerial,&c,1);	// little endian page size in bytes
c='F';	// i think its so for flash
write(fdSerial,&c,1);
/* even bytes!!! */
for (rest=0; rest < bytesInLastPage;rest++)	{
	if (feof(file)) break;
  	fread(&c,1,1,file); /* read a char from file*/
  	write(fdSerial,&c,1);
 						}
c=0XFF;
for (;rest<256;rest++) write(fdSerial,&c,1);  	
readyNow=false;
c=' ';	//now write it
write(fdSerial,&c,1);
while (!readyNow);	// wait for 0x14 0x10
}
cout << "page:\t" << pages << " with " << bytesInLastPage<<" bytes written (last page)  filelength: " << pos;
cout.flush();
#endif

#ifdef AVR
c='s';	 	
write(fdSerial,&c,1);		/* cmd*/
int p;
cout << "\r";
char first=1;
for (p=0;p<pages;p++)			{
 if (first&&testEmptyPage(file)) continue;
 first=0;
  readyNow=false;
  c='p'; 	write(fdSerial,&c,1);		/* cmd */
  usleep(DELAY);
  c=p/256;	write(fdSerial,&c,1);		/* high endian first*/
  usleep(DELAY);
  c=p%256;	write(fdSerial,&c,1);
  usleep(DELAY);	
  while (!readyNow) sched_yield();
  for (int numByte=0; numByte < 256;numByte++)		{
	readyNow=false;
	fread(&c,1,1,file);	/* read a char from file*/
	write(fdSerial,&c,1);		/* write to serial */
	usleep(2*DELAY);
//if (numByte%2)while (!readyNow) sched_yield();	
							}
/*write(fdSerial,&c,1); /* zur synchronisation*/
//  readyNow=false;	
  cout <<"page:\t"<<p<<" from "<<pages<<" pages written \r";
  cout.flush();
  while (!readyNow) sched_yield();
  usleep(5*DELAY);
					}
/* der Rest*/
int rest=0;
if	( bytesInLastPage!=0)		{
  readyNow=false;
  c='p';	write(fdSerial,&c,1);		/* cmd*/
  c=p/256;	write(fdSerial,&c,1);		/* high endian first*/
  usleep(DELAY);
  c=p%256;	write(fdSerial,&c,1);
  usleep(DELAY);
  while (!readyNow) sched_yield();
/* even bytes!!! */
  for (rest=0; rest < bytesInLastPage;rest++)	{
	readyNow=false;
	if (feof(file)) break;
  	fread(&c,1,1,file); /* read a char from file*/
  	write(fdSerial,&c,1);
//if (rest%2) while (!readyNow) sched_yield();
	usleep(3* DELAY);
 						}
  c=0XFF;
  for (;rest<256;rest++)	{
	readyNow=false; 
	write(fdSerial,&c,1); 
//if (rest%2) while (!readyNow) sched_yield();	
	usleep(3*DELAY); 	}
					}

cout << "page:\t" << pages << " with " << bytesInLastPage<<" bytes written (last page)  filelength: " << pos << "-> 0x" << (pos>>16);
cout.flush();
//while (!readyNow) sched_yield();
usleep(5*DELAY);
c='e';	 	write(fdSerial,&c,1);		/* cmd*/
usleep(15*DELAY);
char str[12];
sprintf(str," %05x\n",(pos%(1<<16)));

for (rest=0; rest < 7; rest++)	{
	write(fdSerial,str+rest,1);
	usleep(5*DELAY);	}
#endif					/* ATMEGA128*/

fclose(file);	
#ifdef CPUZ80
		    sleep(2);
		    cout << endl;
		    c=ESC;
		    write(fdSerial,&c,1);
#endif
cout.flush();	
#ifndef AVR
		    c=0; 
		    write(fdSerial,&c,1); /* send 0 to serial, z80 is waiting for it*/
#endif
}


bool testEmptyPage(FILE* file)	{
 long pos=ftell(file);
 bool empty=true;  
 char c;
 for (int n=0; n< (128*2); n++)	{
	fread(&c,1,1,file);
	if (c!=0) empty=false;	};
  if (!empty) fseek(file,pos,SEEK_SET); /* zurueck*/
  return empty;			}
