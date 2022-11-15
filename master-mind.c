//Last Updated 29/03/22 at 12:44
/* ======================================================= */
/* SECTION: includes                                       */
/* ------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
/* --------------------------------------------------------------------------- */
/* Config settings */
/* you can use CPP flags to e.g. print extra debugging messages */
/* or switch between different versions of the code e.g. digitalWrite() in Assembler */
#define DEBUG
#undef ASM_CODE

// =======================================================
// Tunables
// PINs (based on BCM numbering)
// For wiring see CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
// GPIO pin for green LED
#define GREEN_LED 13
// GPIO pin for red LED
#define RED_LED 5
// GPIO pin for button
#define BUTTON 19
// =======================================================
// delay for loop iterations (mainly), in ms
// in mili-seconds: 0.4s
#define DELAY   400
// in seconds: 
#define TIMEOUT 1
// =======================================================
// APP constants   ---------------------------------
// number of colours and length of the sequence
#define COLS 3
#define SEQL 3
// =======================================================

// generic constants

#ifndef	TRUE
#  define	TRUE	(1==1)
#  define	FALSE	(1==2)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define	INPUT			 0
#define	OUTPUT			 1

#define	LOW			 0
#define	HIGH			 1


// =======================================================
// Wiring (see inlined initialisation routine)

#define STRB_PIN 24
#define RS_PIN   25
#define DATA0_PIN 23
#define DATA1_PIN 10
#define DATA2_PIN 27
#define DATA3_PIN 22

/* ======================================================= */
/* SECTION: constants and prototypes                       */
/* ------------------------------------------------------- */

// =======================================================
// char data for the CGRAM, i.e. defining new characters for the display

/* ======================================================= */
/* SECTION: constants and prototypes                       */
/* ------------------------------------------------------- */

// =======================================================
// char data for the CGRAM, i.e. defining new characters for the display

static unsigned char newChar [8] = 
{
  0b11111,
  0b10001,
  0b10001,
  0b10101,
  0b11111,
  0b10001,
  0b10001,
  0b11111,
} ;

/* Constants */

static const int colors = COLS;
static const int seqlen = SEQL;
static char* color_names[] = { "red", "green", "blue" };
static int colourVal = 0;
static int* theSeq = NULL;
static int *seq1, *seq2, *cpy1, *cpy2;
static int waitingFlag = 0;
static int buttonDown = 0;
static int timed_out = 0;

/* --------------------------------------------------------------------------- */





// Mask for the bottom 64 pins which belong to the Raspberry Pi
//	The others are available for the other devices

#define	PI_GPIO_MASK	(0xFFFFFFC0)

static unsigned int gpiobase ;
static uint32_t *gpio ;

#define GPIO_GPFSEL0    0
#define GPIO_GPFSEL1    1
#define GPIO_GPFSEL2    2
#define GPIO_GPFSEL3    3
#define GPIO_GPFSEL4    4
#define GPIO_GPFSEL5    5

#define GPIO_GPSET0     7
#define GPIO_GPSET1     8

#define GPIO_GPCLR0     10
#define GPIO_GPCLR1     11

#define LED_GPFSEL      GPIO_GPFSEL1
#define LED_GPFBIT      18
#define LED_GPSET       GPIO_GPSET0
#define LED_GPCLR       GPIO_GPCLR0
#define GP_LEV0 13




/* ------------------------------------------------------- */
// misc prototypes

int failure (int fatal, const char *message, ...);
void waitForEnter (void);
void waitForButton (uint32_t *gpio, int button);
void initITimer(uint64_t timeout);
void clearPins(uint32_t *gpio);

/* ======================================================= */
/* SECTION: hardware interface (LED, button, LCD display)  */
/* ------------------------------------------------------- */
/* low-level interface to the hardware */
void delay (unsigned int howLong)
{
    struct timespec sleeper, dummy ;

    sleeper.tv_sec  = (time_t)(howLong / 1000) ;
    sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;

    nanosleep (&sleeper, &dummy) ;
}

/* From wiringPi code; comment by Gordon Henderson
 * delayMicroseconds:
 *	This is somewhat intersting. It seems that on the Pi, a single call
 *	to nanosleep takes some 80 to 130 microseconds anyway, so while
 *	obeying the standards (may take longer), it's not always what we
 *	want!
 *
 *	So what I'll do now is if the delay is less than 100uS we'll do it
 *	in a hard loop, watching a built-in counter on the ARM chip. This is
 *	somewhat sub-optimal in that it uses 100% CPU, something not an issue
 *	in a microcontroller, but under a multi-tasking, multi-user OS, it's
 *	wastefull, however we've no real choice )-:
 *
 *      Plan B: It seems all might not be well with that plan, so changing it
 *      to use gettimeofday () and poll on that instead...
 *********************************************************************************
 */

void delayMicroseconds (unsigned int howLong)
{
    struct timespec sleeper ;
    unsigned int uSecs = howLong % 1000000 ;
    unsigned int wSecs = howLong / 1000000 ;

/**/ if (howLong ==   0)
        return ;
#if 0
        else if (howLong  < 100)
delayMicrosecondsHard (howLong) ;
#endif
    else
    {
        sleeper.tv_sec  = wSecs ;
        sleeper.tv_nsec = (long)(uSecs * 1000L) ;
        nanosleep (&sleeper, NULL) ;
    }
}

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Either put them in a separate file, lcdBinary.c, and use   */
/* inline Assembler there, or use a standalone Assembler file */
/* You can also directly implement them here (inline Asm).    */
/* ********************************************************** */
//void digitalWrite (uint32_t *gpio, int pin, int value){
//    if(value == 1){
//        gpio[GPIO_GPSET0] = 1 << pin;
//    }
//    else if(value == 0){
//        gpio[GPIO_GPCLR0] = 1 << pin;
//    }
//}
void digitalWrite (uint32_t *gpio, int pin, int value){

/* control pin */
    asm volatile(
    "\tMOV R1, %[gpio]\n"		/* retrieve GPIO base */
    "\tMOV R0, %[value]\n"
    "\tCMP R0, #0\n"
    "\tBEQ 1f\n"
    "\tMOV R3, %[gpio_GPSET0]\n"
    "\tMOV R2, #4\n"			/* (32 bits = 4 bytes, need to move whole registers) */
    "\tMUL R3, R3, R2\n"			/* set offset for GPSET register */
    "\tMOV R2, #0b1\n"			/* construct bitmask */
    "\tLSL R2, %[pin]\n"			/* bit at pin */
    "\tSTR R2, [R1, R3]\n"		/* write bitmask to GPSET0 (7-th reg) */
    "\tB 2f\n"

    "1:	\tMOV R3, %[gpio_GPCLR0]\n"
    "\tMOV R2, #4\n"			/* (32 bits = 4 bytes, need to move whole registers) */
    "\tMUL R3, R3, R2\n"			/* set offset for GPCLR register */
    "\tMOV R2, #0b1\n"		/* construct bitmask */
    "\tLSL R2, %[pin]\n"			/* bit at pin */
    "\tSTR R2, [R1, R3]\n"		/* write bitmask to GPCLR0 (10-th reg) */
    "2:"

    :
    : [gpio] "r" (gpio),
    [gpio_GPSET0] "i" (GPIO_GPSET0),
    [gpio_GPCLR0] "i" (GPIO_GPCLR0),
    [pin] "r" (pin),
    [value] "r" (value)
    : "r0", "r1", "r2", "r3", "cc" );

}

///* set the @mode@ of a GPIO @pin@ to INPUT or OUTPUT; @gpio@ is the mmaped GPIO base address */
///* Mode 1 defines a pin as an output (LED)
// * Mode 0 defines a pin as an input (Buttton )*/
//void pinMode(uint32_t *gpio, int pin, int mode){
//    int fSel = pin / 10;
//    int shift = (pin % 10) * 3;
//    if(mode == 1 || mode == 0){
//        *(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (mode << shift);
//    }
//
//
//
//}
void pinMode(uint32_t *gpio, int pin, int mode){

    asm volatile (

    "\tMOV R0, %[pin]\n"
    "\tMOV R1, #10\n"
    //"\tUDIV R1, R0, R1\n"
    "\tMOV R1, #1\n"
    "\tMOV R2, #4\n"
    "\tMUL R1, R1, R2\n"		/* R1 is fSel (in bytes) */
    "\tMOV R2, %[gpio]\n"

    "\tADD R1, R1, %[gpio]\n" 	/* R1 is *(gpio + fSel) */
    "\tLDR R1, [R1]\n"

    "\tSUB R0, R0, #10\n"
    "\tMOV R2, #3\n"
    "\tMUL R0, R0, R2\n"		/* R0 is shift */

    "\tMOV R2, %[mode]\n"	/* R2 is mode <<shift */
    "\tLSL R2, R2, R0\n"

    "\tMOV R3, #7\n"
    "\tLSL R3, R3, R0\n"
    "\tMVN R0, R3\n"	/* R0 is ~(7 << shift) */

    "\tAND R0, R1, R0\n"		/* R0 is (*(gpio + fSel) & ~(7 << shift)) */



    "\tORR R2, R2, R0\n"		/* R0 is *(gpio+fSel) & ~(7 << shift) | mode */
    "\tADD R0, %[gpio], #4\n"

    "\tSTR R2, [R0]\n"	/* write to *(gpio+fSel)? */

    :
    :[pin] "r" (pin),
    [gpio] "r" (gpio),
    [mode] "r" (mode)
    :"r0", "r1", "r2", "r3" );

}


/* read a @value@ (LOW or HIGH) from pin number @pin@ (a button device); @gpio@ is the mmaped GPIO base address */
//int readButton(uint32_t *gpio, int button){
//    if ((*(gpio + 13 /* GPLEV0 */) & (1 << (button & 31))) != 0){
//        if(!buttonDown){
//            buttonDown = 1;
//            colourVal++;
//            return 1;
//        }
//
//    }
//    else {
//        buttonDown = 0;
//    }
//    return 0;
//}

int readButton(uint32_t *gpio, int button){
    int res;
    asm volatile(
    "\tMOV R0, %[BUTT]\n"		/* get button int, then AND with 31 */
    //	"\tLDRB R0, [%[BUTT], #0]\n"
    "\tMOV R1, #31\n"
    "\tAND R0, R0, R1\n"
    "\tMOV R1, #0b1\n"
    "\tLSL R1, R1, R0\n"		/* left shift 1 by (button&31) */
    "\tMOV R0, %[gpio]\n"
    "\tMOV R2, %[GP_lev0]\n"
    "\tADD R0, R0, R2, LSL #2\n"
    "\nLDR R0, [R0]\n"
    "\tAND R0, R0, R1\n"		/* r0 is (gpio+GPLEV) & (1 << (button&31)) */
    "\tCMP R0, #0\n"
    "\tBEQ one\n"			/* if 0 (button not pressed), skip to "one" */

    "\tMOV R0, %[buttonDOWN]\n"	/* Check buttonDown */
    "\tCMP R0, #0\n"
    "\tBNE two\n"
    "\tMOV %[buttonDOWN], #1\n"	/* if buttonDown == 0, buttonDown=1, countVal++, return 1 */
    "\tMOV %[RES], #1\n"		/* if buttonDown != 0, return 1 */
    "\tADD %[colourVal], %[colourVal], #1\n"

    "\tB three\n"	/* skip next couple lines */


    "one:	\tMOV %[buttonDOWN], #0\n"	/* else buttonDown = 0 */

    "two:	\tMOV %[RES], #0\n"	/* return = 0 */

    "three:\t\n"

    : [buttonDOWN] "+r" (buttonDown),
    [RES] "=r" (res),
    [colourVal] "+r" (colourVal)

    : [BUTT] "r" (button),
    [GP_lev0] "i" (GP_LEV0),
    [gpio] "r" (gpio)
    : "r0", "r1", "r2", "cc" );
    //~ printf("\nColourVal: %d \n", colourVal);
    //~ printf("\nRes: %d \n", res);
    //delay(500);
    return res;
}



/* wait for a button input on pin number @button@; @gpio@ is the mmaped GPIO base address */
/* can use readButton(), depending on your implementation */
void waitForButton (uint32_t *gpio, int button){

    if(colourVal < 3 ){
        readButton(gpio, button);
        delay(100);
    }




}

/* ======================================================= */
/* SECTION: game logic                                     */
/* ------------------------------------------------------- */
/* AUX fcts of the game logic */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* initialise the secret sequence; by default it should be a random sequence */
void initSeq() {
/* ***  COMPLETE the code here  ***  */
    theSeq = (int*)malloc(seqlen*sizeof(int));
    for (int i = 0; i < seqlen; ++i) {
        theSeq[i] = (rand() % seqlen) + 1;
    }
    

}

/* display the sequence on the terminal window, using the format from the sample run in the spec */

/* ***  COMPLETE the code here  ***  */
void showSeq(int *seq) {
    printf("Sequence is : %s %s %s \n", color_names[seq[0]-1], color_names[seq[1]-1], color_names[seq[2]-1]);
}


/* Displays a given sequence through the output LED's
 * Used for debugging
 * */
void displaySeq(uint32_t *gpio, int *seq){
    for(int i =0; i< seqlen; i++){
        int val = seq[i];
        for(int j = 0; j < val; j++){
            digitalWrite(gpio, GREEN_LED, 1);
            delay(600);
            digitalWrite(gpio, GREEN_LED, 0);
            delay(600);
        }

        digitalWrite(gpio, RED_LED, 1);
        delay(1000);
        digitalWrite(gpio, RED_LED, 0);
    }
    clearPins(gpio);

}

int *dCopy(int *ori){
    int *newSeq = malloc(seqlen*sizeof(int));
    for (int i = 0; i < seqlen; i++){
        newSeq[i]= ori[i];
    }
    return newSeq;
}

/* counts how many entries in seq2 match entries in seq1 */
/* returns exact and approximate matches, either both encoded in one value, */
/* or as a pointer to a pair of values */
int countMatches(int *seq1, int *seq2) {
    int exactCount = 0;
    int approxCount = 0;
    for (int i = 0; i < seqlen; i++) {
        // int s1= seq1[i];
        // int s2 = seq2[i];
        if (seq1[i] == seq2[i]) {
            exactCount++;
            seq1[i] = 4;
            seq2[i] = 4;
        }

    }
    for (int j = 0; j < seqlen; ++j) {
        //int s1 = seq1[j];
        //int s2;
        if(seq1[j] == 4){
            continue;
        }
        else{
            for (int y = 0; y < seqlen; y++){
                //s2 = seq2[y];
                if (seq1[j] == seq2[y] && seq2[y] != 4 ){
                    seq2[y] = 4;
                    approxCount++;
                    break;
                }
            }
        }
    }


    return exactCount * 10 + approxCount;
}

/* show the results from calling countMatches on seq1 and seq1 */
void showMatches(int /* or int* */ code, /* only for debugging */ int *seq1, int *seq2, /* optional, to control layout */ int lcd_format) {
    int approx = code % 10;
    int exact = code / 10;
    printf("%d exact\n", exact);
    printf("%d approximate\n", approx);
}

/* parse an integer value as a list of digits, and put them into @seq@ */
/* needed for processing command-line with options -s or -u            */
void readSeq(int *seq, int val) {
/* ***  COMPLETE the code here  ***  */
    for(int i = seqlen-1; i>=0; i--){
        seq[i] = val % 10;
        val = val / 10;
    }


}

/* read a guess sequence fron stdin and store the values in arr */
/* only needed for testing the game logic, without button input */
int readNum(int max) {
/* ***  COMPLETE the code here  ***  */
    int num = -1;

    do{
        printf("Input number between 0-2 :");
        scanf("%d", &num);

    } while (num > max || num < 0);

    return num;
}

/* ======================================================= */
/* SECTION: TIMER code                                     */
/* ------------------------------------------------------- */
/* TIMER code */

/* timestamps needed to implement a time-out mechanism */
static uint64_t startT, stopT;

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* you may need this function in timer_handler() below  */
/* use the libc fct gettimeofday() to implement it      */
uint64_t timeInMicroseconds(){
    struct timeval tv, tNow, tLong, tEnd ;
    uint64_t now ;
// gettimeofday (&tNow, NULL) ;
    gettimeofday (&tv, NULL) ;
    now  = (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec ; // in us
// now  = (uint64_t)tv.tv_sec * (uint64_t)1000 + (uint64_t)(tv.tv_usec / 1000) ; // in ms

    return (uint64_t)now; // (now - epochMilli) ;
}




/* this should be the callback, triggered via an interval timer, */
/* that is set-up through a call to sigaction() in the main fct. */

void timer_handler (int signum)
{
    /** Commented code below is useful for debugging purposes */
    //~ static int count = 0;
    //~ stopT = timeInMicroseconds();
    //~ count++;
    //~ fprintf(stderr, "timer expired %d times; (measured interval %f sec)\n", count, (stopT-startT)/1000000.0);
    //~ startT = timeInMicroseconds();
    waitingFlag = 0;
}



/* initialise time-stamps, setup an interval timer, and install the timer_handler callback */
void initITimer(uint64_t timeout){
/* ***  COMPLETE the code here  ***  */
    struct sigaction sa;
    struct itimerval timer;

/* Install timer_handler as the signal handler for SIGVTALRM. */
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;
    sigaction (SIGALRM, &sa, NULL);

/* Configure the timer to expire after 250 msec... */
    timer.it_value.tv_sec = timeout;
    timer.it_value.tv_usec = 0;
/* ... and every 250 msec after that. */
    timer.it_interval.tv_sec = timeout;
    timer.it_interval.tv_usec = 0;
/* Start a virtual timer. It counts down whenever this process is
  executing. */
    setitimer (ITIMER_REAL, &timer, NULL);

/* Do busy work. */


}

/* ======================================================= */
/* SECTION: Aux function                                   */
/* ------------------------------------------------------- */
/* misc aux functions */

int failure (int fatal, const char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  if (!fatal) //  && wiringPiReturnCodes)
    return -1 ;

  va_start (argp, message) ;
  vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  fprintf (stderr, "%s", buffer) ;
  exit (EXIT_FAILURE) ;

  return 0 ;
}

/*
 * waitForEnter:
 *********************************************************************************
 */

void waitForEnter (void)
{
  printf ("Press ENTER to continue: ") ;
  (void)fgetc (stdin) ;
}







/* ======================================================= */
/* SECTION: aux functions for game logic                   */
/* ------------------------------------------------------- */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* --------------------------------------------------------------------------- */
/* interface on top of the low-level pin I/O code */

/* blink the led on pin @led@, @c@ times */
void blinkN(uint32_t *gpio, int led, int c) {
/* ***  COMPLETE the code here  ***  */
    int i;

    for(i = 0; i < c; i++){
        digitalWrite(gpio, led, 0);
        delay(400);
        digitalWrite(gpio, led, 1);
        delay(400);
    }
    digitalWrite(gpio, led, 0);
}

void clearPins(uint32_t *gpio){
    digitalWrite(gpio, GREEN_LED, 0);
    digitalWrite(gpio, RED_LED, 0);


}

/* Takes an integer input which represents how much exact/approximates matches there are
 * e.g.
 * 20 = 2 exact 0 approximate
 *  2 = 0 exact 2 approximate
 * 11 = 1 exact 1 approximate
 * */

void echoMatches(uint32_t *gpio, int exact, int approx){


    //Blinking for exact matches
    blinkN(gpio, GREEN_LED, exact);
    delay(DELAY);

    //Seperator Red blink
    blinkN(gpio, RED_LED, 1);
    delay(DELAY);


    //Blinking for approximate matches
    blinkN(gpio, GREEN_LED, approx);
    delay(DELAY);



}

/* Displays a successful guess*/
void displaySuccess(uint32_t *gpio){
    digitalWrite(gpio, RED_LED, 1);
    delay(50);
    blinkN(gpio, GREEN_LED, 3);
    digitalWrite(gpio, RED_LED, 0);
}

/* ======================================================= */
/* SECTION: main fct                                       */
/* ------------------------------------------------------- */

int main (int argc, char *argv[])
{ // this is just a suggestion of some variable that you may want to use

    srand(time(NULL));
    int found = 0, attempts = 0, i, j, code;
    int c, d, buttonPressed, rel, foo;
    int *attSeq;
    int timeout = TIMEOUT;
    int greenLED = GREEN_LED, redLED = RED_LED, pinButton = BUTTON;
    int fSel, shift, pin,  clrOff, setOff, off, res;
    int fd;

  char str1[32];
  char str2[32];
  
  struct timeval t1, t2 ;
  int t ;

  char buf [32] ;

  // variables for command-line processing
  char str_in[20], str[20] = "some text";
  int verbose = 0, debug = 0, help = 0, opt_m = 0, opt_n = 0, opt_s = 0, unit_test = 0, res_matches = 0;
  
  // -------------------------------------------------------
  // process command-line arguments

  // see: man 3 getopt for docu and an example of command line parsing
  { // see the CW spec for the intended meaning of these options
    int opt;
    while ((opt = getopt(argc, argv, "hvdus:")) != -1) {
      switch (opt) {
      case 'v':
	verbose = 1;
	break;
      case 'h':
	help = 1;
	break;
      case 'd':
	debug = 1;
	break;
      case 'u':
	unit_test = 1;
	break;
      case 's':
	opt_s = atoi(optarg); 
	break;
      default: /* '?' */
	fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
	exit(EXIT_FAILURE);
      }
    }
  }

  if (help) {
    fprintf(stderr, "MasterMind program, running on a Raspberry Pi, with connected LED, button and LCD display\n"); 
    fprintf(stderr, "Use the button for input of numbers. The LCD display will show the matches with the secret sequence.\n"); 
    fprintf(stderr, "For full specification of the program see: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf\n"); 
    fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
    exit(EXIT_SUCCESS);
  }
  
  if (unit_test && optind >= argc-1) {
    fprintf(stderr, "Expected 2 arguments after option -u\n");
    exit(EXIT_FAILURE);
  }

  if (verbose && unit_test) {
    printf("1st argument = %s\n", argv[optind]);
    printf("2nd argument = %s\n", argv[optind+1]);
  }

  if (verbose) {
    fprintf(stdout, "Settings for running the program\n");
    fprintf(stdout, "Verbose is %s\n", (verbose ? "ON" : "OFF"));
    fprintf(stdout, "Debug is %s\n", (debug ? "ON" : "OFF"));
    fprintf(stdout, "Unittest is %s\n", (unit_test ? "ON" : "OFF"));
    if (opt_s)  fprintf(stdout, "Secret sequence set to %d\n", opt_s);
  }

  seq1 = (int*)malloc(seqlen*sizeof(int));
  seq2 = (int*)malloc(seqlen*sizeof(int));
  cpy1 = (int*)malloc(seqlen*sizeof(int));
  cpy2 = (int*)malloc(seqlen*sizeof(int));

  // check for -u option, and if so run a unit test on the matching function
  if (unit_test && argc > optind+1) { // more arguments to process; only needed with -u 
    
    strcpy(str_in, argv[optind]);
    opt_m = atoi(str_in);
    strcpy(str_in, argv[optind+1]);
    opt_n = atoi(str_in);
    // CALL a test-matches function; see testm.c for an example implementation
    readSeq(seq1, opt_m); // turn the integer number into a sequence of numbers
    readSeq(seq2, opt_n); // turn the integer number into a sequence of numbers
    if (verbose)
      fprintf(stdout, "Testing matches function with sequences %d and %d\n", opt_m, opt_n);
    res_matches = countMatches(seq1, seq2);
    showMatches(res_matches, seq1, seq2, 1);
    exit(EXIT_SUCCESS);
  } else {
    /* nothing to do here; just continue with the rest of the main fct */
  }

  if (opt_s) { // if -s option is given, use the sequence as secret sequence
    if (theSeq==NULL)
      theSeq = (int*)malloc(seqlen*sizeof(int));
    readSeq(theSeq, opt_s);
    if (verbose) {
      fprintf(stderr, "Running program with secret sequence:\n");
      showSeq(theSeq);
    }
  }
  




  if (geteuid () != 0)
    fprintf (stderr, "setup: Must be root. (Did you forget sudo?)\n") ;

  // init of guess sequence, and copies (for use in countMatches)
  attSeq = (int*) malloc(seqlen*sizeof(int));
  cpy1 = (int*)malloc(seqlen*sizeof(int));
  cpy2 = (int*)malloc(seqlen*sizeof(int));

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000 ;

  // -----------------------------------------------------------------------------
  // memory mapping 
  // Open the master /dev/memory device

  if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    return failure (FALSE, "setup: Unable to open /dev/mem: %s\n", strerror (errno)) ;

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
  if ((int32_t)gpio == -1)
    return failure (FALSE, "setup: mmap (GPIO) failed: %s\n", strerror (errno)) ;

  // -------------------------------------------------------
  // Configuration of LED and BUTTON

  /* ***  COMPLETE the code here  ***  */
 


  /* initialise the secret sequence */
  if (!opt_s){
      initSeq();
  }
  if (debug){
      showSeq(theSeq);
  }
    
    
 pinMode(gpio, greenLED, 1);
 pinMode(gpio, redLED, 1);
 pinMode(gpio, pinButton, 0);



//~ printf("Showing the secret code in just a second, (Debugging purposes only) \n");
//~ delay(500);
//~ displaySeq(gpio, theSeq);
//~ printf("Seq[1] = %d\n", theSeq[1]);
//~ printf("Seq[2] = %d\n", theSeq[2]);
//~ clearPins(gpio);
printf("======================================= \n");


//Green led and RED on indicates waiting for input (debugging)
    //~ digitalWrite(gpio, greenLED, 1);
    //~ digitalWrite(gpio, redLED, 1);
    //~ delay(1000);
    //~ clearPins(gpio);

// 2. Waits until button is pressed, before game is started
    printf("Press the button to begin game  \n");

  /* Game will keep continue indefinitely until the correct guess has been inputted */
    while(1){
        
        /* 'found' variable keeps track of how many colours have been inputted and is used to 
         * index the inputted colours in attSeq. i.e. attSeq[found] = 3 (blue)  */ 
        while(found < seqlen){
            waitingFlag = 1;


            clearPins(gpio);


            /* This is waiting for the first press of the button to initialise the sequence. Flow of excecution will
             * not continue until the first button press is registered for that particular number in that sequence */
            while(!readButton(gpio, pinButton));


            /*Waiting flag will only change when the timer has elapsed and the
             * callback is fired  i.e. timehandler sets waitingFlag to 1 when 3 seconds have elapsed*/
            initITimer(timeout);
            while(waitingFlag){
                waitForButton(gpio, pinButton);
            }

            //red LED indicating input accepted
            delay(DELAY);
            digitalWrite(gpio, redLED, 1);
            delay(DELAY);
            digitalWrite(gpio, redLED, 0);


            //resetting timer
            initITimer(0);


            //echoing the input value
            blinkN(gpio, greenLED, colourVal);
            delay(DELAY);
            attSeq[found] = colourVal;
            colourVal = 0;
            found++;

            //fixed time interval between each sequence number


        }

    //input has been accepted, blinking twice
        blinkN(gpio, redLED, 2);
        delay(DELAY);





        showSeq(theSeq);
        showSeq(attSeq);
        cpy1 = dCopy(theSeq);
        cpy2 = dCopy(attSeq);



//echoing matches
        int matches = countMatches(cpy1, cpy2);
        int approx = matches % 10;
        int exact = matches / 10;
        echoMatches(gpio, exact, approx);



//Condition where the user's guess is correct
        if(exact == seqlen){
            displaySuccess(gpio);
            break;
        }
        found = 0;
        attempts++;
    }
  return 0;
}
