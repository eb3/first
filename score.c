//
//SCORE
//

#include "modular.h"

float score(float input, moduleData *data)
{
	//initialize some stuff - using varibles from signal number PATCHES (in this case, signal #200)
	//This is just so that we can use an initflag for the score.
	if(data->initflag[PATCHES] == 0)
	{
		//So we don't run this chunk of code again
		data->initflag[PATCHES] = 1;
		
		//make arrays of amplitude values to send to gen10
		float amps1[] = {1};														//just a fundamental - produces a sine wave
		float amps2[] = {1, 0, .3, 0, .2, 0, .14, 0, .111};							//fundamental and the first 8 harmonics of a square wave
		float amps3[] = {1, .5, .33, .25, .2, .167, .14, .125, .111};				//fundamental and the first 8 harmonics of a sawtooth wave
		float amps4[] = {1, 0, .1111, 0, .04, 0, .00204, 0, .00123, 0, .000826};	//fundamental and the first 10 harmonics of a triangle wave
		
		//generate waveform tables
		gen10(1, amps1, 1, data);		//sine wave
		gen10(2, amps2, 9, data);		//square wave
		gen10(3, amps3, 9, data);		//sawtooth wave
		gen10(4, amps4, 11, data);		//triangle wave
	}
	
	float sendOutput;	//local variable that will eventually be returned to the callback function, which will then put the calculated output sample to the DAC
	float mix;			//local variable for temporarily mixing signals. This isn't necessary, but makes the code easier to read.
	
	//increment time counter (this will hold the number of samples our program has output)
	data->counter++;
	
	
	//this next bit of code is an example of how to do some basic synthesis with our modules (functions)
	//this is explained in more detail in README.txt
	//more score examples can also be found in README.txt


	switch(data->currentScore)
	{
		//delayed osc*env
		case 1:
		{
			//starting on patchcord 5 since 1-4 are used by the ftables
			osc(5, data->note, .8, 2, data);			//oscillator on patchcord 5, frequency given from command line, amplitude of .8, ftable 2
			expenv(6, .1, .1, 1, data);					//expenv on patchcord 6, attack time of .1, decay time of .1, amplitude of 1
			mix = data->sigout[5] * data->sigout[6];	//mulitply the oscillator (signal output of patchcord 5) by the expenv (signal output of patchcord 6)
			
			delay(7, mix, .15, .9, data);
			
			sendOutput = data->sigout[7] + mix;			//set sendOutput - wet and dry signal
			break;
		}
		//delayed input
		case 2:
		{
			delay(11, input, .5, .7, data);
			sendOutput = input + data->sigout[11];		//wet and dry delayed input
			break;
		}
		//diskin oneshot
		case 3:
		{
			diskin(8, "yes.aiff", 0, data);				//diskin - file must be mono - set type to zero (oneshot)
			sendOutput = data->sigout[8];
			break;
		}
		//diskin controlled by osc
		case 4:
		{
			diskin(9, "yes.aiff", 2, data);					//diskin with type 2 (manual control)
			osc(10, .8, 25090, 3, data);					//osc with an amplitude of the number of samples in the file (25090)
			
			//set output to the file on cord 9 with the index of osc 10's amplitude (absolute value to avoid negative numbers)
			sendOutput = data->file[9][(int)fabs(data->sigout[10])];
			break;
		}
		//highpass filter
		case 5:
		{
			diskin(12, "loop.aiff", 1, data);
			twoPoleHigh(13, data->sigout[12], 16000, data);
			mix = data->sigout[12] * data->sigout[13];
			
			//delay(15, mix, .2, .3, data); commented out because delaying this loop doesn't sound good
			sendOutput = mix;
			break;
		}
	}
	
	
	//the % operand is called modulus. It takes two values, divides them, and results in the remainder.
	//example: 5 % 2 would result in 1 since the remaineder of 5 / 2 is 1 (remember long hand divison?)
	//
	//samples / sample_rate = time in seconds.
	//so in this line of code, if counter % sample_rate is equal to 0, then we know our time is exactly on a second and not in between - print the time to the screen.
	if((int)data->counter % (int)SAMPLE_RATE == 0) printf("Elapsed time: %d\n", (int)data->counter / (int)SAMPLE_RATE);
	
	//if elapsed time in second is greater than the length in seconds given from the command line, set our endFlag to 1
	//endFlag = 1 will cause our while loop in the mainloop function to end, portaudio will clean up and shut down, the program will end.
	if((data->counter / (float)SAMPLE_RATE) > data->length) data->endFlag = 1;
	
	//return a sample to be put to the DAC
	return sendOutput;
}
