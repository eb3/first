#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "portaudio.h"
#include "portsf.h"


//constants
#define SAMPLE_RATE         44100.0			//Audio sample rate
#define ISRX2P 				.000142475857	//(1/samplingrate) * 2 * pi
#define FRAMES_PER_BUFFER	256				//Buffer size
#define PATCHES				200				//Maximum number of signals
#define PI 					3.141592654		//PI
#define ENV_SIZE			8192			//Make size for envelope array
#define TABLE_LENGTH		8192			//Make size for all other tables
#define MAX_DELAY			3*SAMPLE_RATE	//Size of delay buffer table
#define MAX_FILE_SIZE		60*SAMPLE_RATE	//Max size of input file


//define a data structure that will be used to pass user data to portaudio
typedef struct
{
	//flags
	int* initflag;			//whether or not a signal has been initialized
	int endFlag;
	
	//temp variables
	float* tempvar;			//Each module uses tempvariables in different ways
	float* tempvar2;		//Comments above the module explain what these variables represent
	float* sigout;			//Output value of a signal
	
	//filter variables
	float* yy1;
	float* yy2;
	float* zz1;
	float* zz2;
	float* RCOS;
	float* RSIN;
	float cutofffreq;
	
	//table look up variables
	float** table;			//multidimensional table to store tables for signals
	float* tableIndex;		//where we are in a table
	float** file;			//file tables to be read
	float* fileMax;			//size of the file loaded in
		
	//variable identifying a stream for each instance of a module (virtual patch chord)
	int patchcord;
	
	//note to play from commandline
	int note;
	
	//total number of samples calculated (counter / sample_rate = elapsed time in seconds)
	double counter;
	
	//length to run for in seconds
	int length;
	
	//current score
	int currentScore;
	
} moduleData;



// function prototypes
void initialize(moduleData *data);
int mainloop(moduleData *data);

//modules
void onePoleLow(int patchcord, float input, int freq, moduleData *data);
void twoPoleHigh(int patchcord, float input, int freq, moduleData *data);
void twopole(int patchcord, float input, float freq, moduleData *data);
void osc(int patchcord, float freq, float amp, int ftable, moduleData *data);
void envelope(int patchcord, float attack, float decay, float amp, moduleData *data);
void expenv(int patchcord, float attack, float decay, float amp, moduleData *data);
void averagingLowPass(int patchcord, float input, moduleData *data);
void averagingHighPass(int patchcord, float input, moduleData *data);
void vDelay(int patchcord, float input, float length, float feedback, moduleData *data);
void delay(int patchcord, float input, float length, float feedback, moduleData *data);
void noise(int patchcord, float amp, moduleData *data);
void diskin(int patchcord, char* filename, int type, moduleData *data);


//gen routines
void gen10(int ftable, float* amps, int nSines, moduleData *data);

//score
float score(float input, moduleData *data);