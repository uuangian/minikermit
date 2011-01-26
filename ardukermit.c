// a terminal program for serial communication and upload programs at microcontroller
// especially for arduino boards
// bin file uploading with stripped down stk500v1 protocol
/*
* See the file "license.terms" for information on usage and redistribution
* of this file.
*/
/* ardukermit - version 0.1 110126 */
/* mini-kermit	Version 0.6	040615*/

// how to use:
// set MACROS TARGET BAUDRATE MODEMDEVICE and SCANSERIALDEVICES in this file
// g++ ardukermit.c -ardukermit -lncurses
// connect with target and start: #>./ardukermit

/* ardukermit works in 2 modes*/
/* ECHOMODE 	- write out all typed chars , break with CTRL-C only */
/* NONECHOMODE 	- for monitor (bamo)		*/
/*		- q-quit , w download file	*/
/* it starts in NONECHOMODE*/
/* target-system can change mode with TOGGLEECHOMODE (echomode/nonechomode)*/
/* improve file select for uploading: Yves Vinzing 2007 */

/**************************************************************************************************************/

#define	ARDUINOSWRESET		// send sw reset command before bootloading

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
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <curses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/file.h>
#include <errno.h>

using namespace std;

/*  RS323 control-lines */
void setAllOnHigh		(int fdSerial);
void resetInExpandedMode	(int fdSerial);
void resetAndSetBootMode	(int fdSerial);
void resetAndSetExpandedMode	(int fdSerial);

void upLoadFile		(int fdSerial, char order);
bool testEmptyPage	(FILE* file);
ssize_t mywrite(int fd, const void *buf, size_t count);

#define NAMELENGTH	48       /* max. length of filename*/

#define TOGGLEECHOMODE	0x03
#define SUPRESSSERINPUT	0x04
#define ACCEPTSERINPUT	0x05
#define Resp_STK_INSYNC	0x14
#define Resp_STK_OK	0x10 

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
#ifdef SCANSERIALDEVICES	// open serial device
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
  newTio.c_lflag	= 0;	/* non canonical mode (char mode)*/ // 1 stop bit ???
  newTio.c_cc[VTIME]	= 0;	/* blocked*/
  newTio.c_cc[VMIN]	= 1;	/* return, when 1 byte available*/
  tcflush(fdSerial,TCIFLUSH);
  tcsetattr(fdSerial,TCSANOW,&newTio);
 // I think it makes arduino reset also
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
		continue;					} /* end of (if cFromKeypad=='w'...) */
	  mywrite(fdSerial,&cFromKeypad,1); 			  /* write 1 byte to serial */
 				} 			 	  /* end of while(1) */
  			}	 			 /* end of (if (pid=fork() ...) */
  else
	while (1)		{		/* child */
		while(read(fdSerial,&cFromSerial,1)!=1);		/* read serial 1 byte */
		if (cFromSerial == Resp_STK_INSYNC )	continue;	// wait for 0x10 ; stk500 sync byte
		if (cFromSerial == TOGGLEECHOMODE)	{ kill(getppid(),SIGHUP); continue; 	}
		if (cFromSerial == SUPRESSSERINPUT)	{ supressSerInput=true;   continue; 	}
		if (cFromSerial == Resp_STK_OK)		{ 
		    kill(getppid(),SIGUSR1);/* ready now */continue;
		    /* stk500v1 sync byte */		}
//		kill(getppid(),SIGUSR1);	/* ready now */
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
    usleep(1000);
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
	      cout << "name of binary -file for uploading:\t";
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
		fileName[i]='\0';
		cout.flush();	
		usleep(100);
		/* try to open file and send it to serial, abort on error*/
		if ((file=fopen(fileName,"r")) == NULL)	{
		    perror(fileName);
		    c='Q';
mywrite(fdSerial,&c,1);
c=' ';
mywrite(fdSerial,&c,1);		// leave bootloading mode
		return;			}

unsigned long pos;
fseek(file,0L,SEEK_END);
pos=ftell(file);
int pages=pos/(128*2);
int bytesInLastPage=pos%256;
if (pages >=(64*8-4*8)||(pos==0))	{	/*??????????*/
	cout << " program too long,  overwrites bootsection! - or perhaps no bytes in file??\n";
	cout.flush();
	fclose(file);	
		    c='Q';
mywrite(fdSerial,&c,1);
c=' ';
mywrite(fdSerial,&c,1);		// leave bootloading mode

	return;				}
rewind(file);

unsigned short int address=0;
cout << "\r\n";
#ifdef ARDUINOSWRESET
resetInExpandedMode(fdSerial);
usleep(80000);		// adjust it for your arduino board - reset pulse about 3,5 ms
#endif
int p;
for (p=0;p<pages;p++)			{
  c='U';	// write flash start address
mywrite(fdSerial,&c,1);
 usleep((1000000*2)/BAUDRATE);
mywrite(fdSerial,&address,1);		// little endian
mywrite(fdSerial,(unsigned char*)&address+1,1);	// big endian startaddress is 0000 in words
address+=0x80;
readyNow=false;
c=' ';
mywrite(fdSerial,&c,1);
while (!readyNow)sched_yield();		// wait for 0x14 0x10
c='d';				// write flash 
mywrite(fdSerial,&c,1);
c=0x1;
mywrite(fdSerial,&c,1);		// big endian
c=0;
mywrite(fdSerial,&c,1);		// little endian page size in bytes
c='F';				// i think its so for flash
mywrite(fdSerial,&c,1);
 usleep((1000000*2)/BAUDRATE);
for (int numByte=0; numByte < 256;numByte++)		{
	fread(&c,1,1,file);	/* read a char from file*/
	mywrite(fdSerial,&c,1);	/* write to serial */
							}
readyNow=false;
c=' '; 				// now write it
mywrite(fdSerial,&c,1);
cout <<"page:\t"<<p<<" from "<<pages<<" pages written\r";
cout.flush();
while (!readyNow)sched_yield();		// wait for 0x14 0x10
}

/* the same for the Rest */
int rest=0;
if	( bytesInLastPage!=0)		{
c='U';	// write flash start address
mywrite(fdSerial,&c,1);
mywrite(fdSerial,&address,1);		// little endian
mywrite(fdSerial,((unsigned char*)&address)+1,1);	// big endian startaddress is 0000 in words
readyNow=false;
c=' ';
mywrite(fdSerial,&c,1);
while (!readyNow);	// wait for 0x14 0x10
c='d';	// write flash 
mywrite(fdSerial,&c,1);
 usleep((1000000*2)/BAUDRATE);
c=0x01;
mywrite(fdSerial,&c,1);	// big endian
c=0;
mywrite(fdSerial,&c,1);	// little endian page size in bytes
c='F';	// i think its so for flash
mywrite(fdSerial,&c,1);
/* even bytes!!! */
 usleep((1000000*2)/BAUDRATE);
for (rest=0; rest < bytesInLastPage;rest++)	{
	if (feof(file)) break;
  	fread(&c,1,1,file); /* read a char from file*/
  	mywrite(fdSerial,&c,1);
 						}
c=0XFF;
for (;rest<256;rest++) mywrite(fdSerial,&c,1);  	
readyNow=false;
c=' ';	//now write it
mywrite(fdSerial,&c,1);
while (!readyNow);	// wait for 0x14 0x10
}
cout << "page:\t" << pages << " with " << bytesInLastPage<<" bytes written (last page)  filelength: " << pos;
cout.flush();
usleep(20000);
c='Q';
mywrite(fdSerial,&c,1);
c=' ';
mywrite(fdSerial,&c,1);
fclose(file);	
cout.flush();	
}


bool testEmptyPage(FILE* file)	{
 unsigned long pos=ftell(file);
 bool empty=true;  
 char c;
 for (int n=0; n< (128*2); n++)	{
	fread(&c,1,1,file);
	if (c!=0) empty=false;	};
  if (!empty) fseek(file,pos,SEEK_SET); /* zurueck*/
  return empty;			}

// linux usb serial problem with blocked write sys calls
ssize_t mywrite(int fd, const void *buf, size_t count)	{
    size_t len = 0;
    int	rc;
 
    //return write(fd, buf, count);
    
    while (count) {
    rc = write(fd, buf, count);
    if (rc < 0) {
      fprintf(stderr, "%s: ser_write(): write error: \n",strerror(errno));
      exit(1);
    }
    len+= rc;
    count-=rc;
  }
 //usleep((1000000*2)/BAUDRATE);	// adjust it???	
    return len;
}