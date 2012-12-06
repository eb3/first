//Modular
//
//Written by Tim Lukens modified by Beau Wright
//tim.lukens@gmail.com
//
//Adapted from the USB Phaser by Max Mathews
//
//Notes about how to use and understand this modular synthesizer
//in README.txt
//
//This is the final version
//
//main() is at the bottom
//
//compile with:
//gcc -o modular main.c modules.c gens.c score.c libportaudio.dylib libportsf.a


#include "modular.h"



//// portaudio callback function ////
//this function is defined in the portaudio.h header file and implemented here
//once started, this function gets called automatically at the rate of sample_rate / frames_per_buffer
static int paModuleCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
	//cast our userData to a pointer named data
	moduleData *data = (moduleData*)userData;
	int i;   

	//local pointer to input and output buffers
	float *out = outputBuffer;
	float *in = (float*)inputBuffer;

	// loop through the buffersize and call the score
 	for(i=0; i<framesPerBuffer; i++) {
		out[i] = score(in[i], data); //write one sample to the dac
 	}
  
	return 0;
}






int mainloop(moduleData *data)
{	
	PaStream *stream;		//define a pointer to a PaStream type. We'll use this pointer to open a Portaudio stream
	PaError err;			//err is for catching and handling errors.
    
	printf("\n\n\nModule: starting audio.\n\n\n");

	//Initialize Portaudio
	//Pa_Initialize() returns paNoError if everything went fine
	err = Pa_Initialize();
	if( err != paNoError ) goto error;

	//create a stream - this function returns paNoError if everything went fine
	err = Pa_OpenDefaultStream( &stream,	//name of our stream
				1,         					//mono input
				1,          				//mono output
				paFloat32,  				//32 bit floating point output
				SAMPLE_RATE,				//sample rate
				FRAMES_PER_BUFFER,			//Audio buffer size
				paModuleCallback,			//name of the callback function
				data);						//userData struct
	if( err != paNoError ) goto error;

	//start the stream we just created - this function returns paNoError if everything went fine
	err = Pa_StartStream( stream );
	if( err != paNoError ) goto error;
	
	//right at this point in the program, the function paModuleCallback will start getting called automatically at the rate of (sample_rate / frames_per_buffer).
	//so all this function needs to do now is sit until we're done synthesizing.

	//This while loop will sit and do nothing. This is to prevent our program from moving forward in this function (paModuleCallback is still getting called, though).
	//Once endFlag gets set to 1, the function will continue.
	while(!data->endFlag);


	//Stop the stream
	err = Pa_StopStream( stream );
	if( err != paNoError ) goto error;
	
	//delete the stream
	err = Pa_CloseStream( stream );
	if( err != paNoError ) goto error;
	
	//clean up and terminate Portaudio
	Pa_Terminate();
	printf("\n\n\nModule: all done.\n\n\n");
	return err;


	//goto's are considered a bad programming practice, but this code is simple enough to not be an issue
error:
	Pa_Terminate();
	fprintf( stderr, "An error occured while using the portaudio stream\n" );
	fprintf( stderr, "Error number: %d\n", err );
	fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
	return err;
}



//initialization function
//this function memory allocates all of the arrays in the userData structure and sets some initial values
void initialize(moduleData *data)
{
	int i;
	
	data->counter = 0;
	data->endFlag = 0;
	
	data->sigout = (float*)malloc(sizeof(float) * PATCHES);
	data->initflag = (int*)malloc(sizeof(int) * PATCHES);
	data->RCOS = (float*)malloc(sizeof(float) * PATCHES);
	data->RSIN = (float*)malloc(sizeof(float) * PATCHES);
	data->yy1 = (float*)malloc(sizeof(float) * PATCHES);
	data->yy2 = (float*)malloc(sizeof(float) * PATCHES);
	data->zz1 = (float*)malloc(sizeof(float) * PATCHES);
	data->zz2 = (float*)malloc(sizeof(float) * PATCHES);
	data->tempvar = (float*)malloc(sizeof(float) * PATCHES);
	data->tempvar2 = (float*)malloc(sizeof(float) * PATCHES);
	data->table = (float**)malloc(sizeof(float) * PATCHES);
	data->tableIndex = (float*)malloc(sizeof(float) * PATCHES);
	data->file = (float**)malloc(sizeof(float) * PATCHES);
	data->fileMax = (float*)malloc(sizeof(float) * PATCHES);

		
	for (i=0;i<PATCHES;i++)
	{
		data->sigout[i] = 0;
		data->initflag[i] = 0;
		data->RCOS[i] = 0;
		data->RSIN[i] = 0;
		data->yy1[i] = 0;
		data->yy2[i] = 0;
		data->zz1[i] = 0;
		data->zz2[i] = 0;
		data->tempvar[i] = 0;
		data->tempvar2[i] = 0;
		data->table[i] = (float*)malloc(sizeof(float) * ENV_SIZE);
		data->file[i] = (float*)malloc(sizeof(float) * MAX_FILE_SIZE);
	}
}


//main function
//as is standard in C programs, argc is the number of arguments typed in by the user, and argv[] holds the arguments typed by the user.
int main(int argc, char* argv[])
{
	moduleData data; //userData struct full of all the variables used by Portaudio
	
	//If there aren't the right number of command line arguments, give a simple usage statement and exit.
	//In C programs, argument 0 is the name of the program, so if our program only takes three arguments (like in this case)
	//argc would then equal 4.
  	if(argc < 4 || argc > 4)
  	{
  		printf("usage: ./modular frequency (in hertz) time (in seconds) score\n");
  		printf("example: ./modular 440 5 1\n\n");
  		printf("score 1: delayed square wave\n");
  		printf("score 2: delayed input\n");
  		printf("score 3: diskin oneshot\n");
  		printf("score 4: diskin controlled by osc\n");
  		printf("score 5; diskin high pass filtered\n");
  		return 1;
  	}
  	
  	//set note to argument one and length to argument 2
  	//since command line arguments are chars, we must use atoi() which converts ascii characters into an integer
  	data.note = atoi(argv[1]);
  	data.length = atoi(argv[2]);
  	data.currentScore = atoi(argv[3]);
  	
  	//initialize
	initialize(&data);
	
	//start the mainloop
	mainloop(&data);

	return 0;
}

