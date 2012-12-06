//MODULES

#include <math.h>
#include "modular.h"



//diskin
//sound file reader using portsf
//type 0 - one shot
//type 1 - loop
//type 2 - manual control
//
//This module loads in a sound file and either loops through it one sample at a time
//or allows the programmer to read through the table in any way with manual control
void diskin(int patchcord, char* filename, int type, moduleData *data)
{
	//init
	if(!data->initflag[patchcord])
	{
		data->initflag[patchcord] = 1;
		data->tableIndex[patchcord] = 0;
		
		int i;
		PSF_PROPS props;							//file properties

		long framesread;							//read a sample

		long totalread;								//how many samples have been read

		int ifd = -1;								//for opening a file

		float* frame = NULL;						//a sample

	
		//init portsf
		psf_init();
		
		//read the sound file
		ifd = psf_sndOpen(filename, &props, 0);
		
		//number of audio channels
		printf("chans: %ld\n", props.chans);
		
		//malloc one frame
		frame = (float*) malloc(props.chans * sizeof(float));
		
		//read in the first frame
		data->file[patchcord][0] = psf_sndReadFloatFrames(ifd,frame,1);

		totalread = 0;
		
		//read in the rest of the frames (psf_sndReadFloatFrames returns 1 if a frame was read)
		while ((psf_sndReadFloatFrames(ifd,frame,1)) == 1)		

		{
			data->file[patchcord][totalread] = frame[0];
			totalread++;
		}
		
		//store the max for this file
		data->fileMax[patchcord] = totalread;
		printf("%s: total frames read: %ld\n", filename, totalread);
	}
	
	//if not on manual control
	if(type != 2 && data->tableIndex[patchcord] < data->fileMax[patchcord] + 1)
	{
		//set output and incriment
		data->sigout[patchcord] = data->file[patchcord][(int)data->tableIndex[patchcord]];
		data->tableIndex[patchcord]++;
		
		//if loop is on, wrap back around
		if(type == 1)
			if(data->tableIndex[patchcord] > data->fileMax[patchcord]) data->tableIndex[patchcord] = 0;
	}
}



//vDelay
//tempvar = table increment
//
//Still doesn't work quite right... The interpolation hasn't been implemented
//
//This function is used to generate a variable time delay with feedback. It starts by writing
//the current input sample to a buffer, incrementing the buffer index, then outputting
//a sample from the new buffer index. Once the end of the buffer has been reached,
//the buffer index loops back around to the beginning. This is based on the table lookup
//method which is described in much greater detail in the osc function.
//Samples need to be interpolated between increment points - this has not been implemented
//correctly yet.
void vDelay(int patchcord, float input, float length, float feedback, moduleData *data)
{
	int i;
	
	//initialize some stuff
	if(!data->initflag[patchcord])
	{
		data->initflag[patchcord] = 1;
		data->table[patchcord][0] = 0;
		data->tempvar[patchcord] = 0;
		data->tableIndex[patchcord] = 0;
	}
	
	//write sample to buffer adding feedback from anything that is already there.
	data->table[patchcord][(int)data->tableIndex[patchcord]] = input + (feedback*data->table[patchcord][(int)data->tableIndex[patchcord]]);
	
	//determine increment based on the the length of the delay
	data->tempvar[patchcord] = ((1 / length) * MAX_DELAY) / SAMPLE_RATE;
	
	//increment buffer pointer
	data->tableIndex[patchcord] += data->tempvar[patchcord];
	
	//INTERPOLATE - this hasn't been done yet.
	
	//if the buffer length has been reached, reset the index
	if(data->tableIndex[patchcord] > MAX_DELAY)
		data->tableIndex[patchcord] -= MAX_DELAY;
	
	//output delayed buffer
	data->sigout[patchcord] = data->table[patchcord][(int)data->tableIndex[patchcord]];
}



//delay
//tempvar = length - this is so it can only be set at init
void delay(int patchcord, float input, float length, float feedback, moduleData *data)
{
	int i;
	
	//initialize some stuff
	if(!data->initflag[patchcord])
	{
		if(length > 3) exit(0); 					//quit if the delay length is greater than 3 seconds
		
		data->tempvar[patchcord] = length;				//so delay time can only be set at init
		
		data->initflag[patchcord] = 1;
		for(i = 0; i < length * SAMPLE_RATE; i++)	//zero out the buffer
			data->table[patchcord][i] = 0;
		
		data->tableIndex[patchcord] = 0;
	}
	
	//write sample to buffer adding feedback from anything that is already there.
	data->table[patchcord][(int)data->tableIndex[patchcord]] = input + (feedback * data->table[patchcord][(int)data->tableIndex[patchcord]]);		
	
	//increment buffer pointer
	data->tableIndex[patchcord]++;
	
	//if the buffer length has been reached, reset the index
	if(data->tableIndex[patchcord] > data->tempvar[patchcord] * SAMPLE_RATE)
		data->tableIndex[patchcord] = 0;
	
	//output delayed buffer
	data->sigout[patchcord] = data->table[patchcord][(int)data->tableIndex[patchcord]];
}





//envelope - linear envelope generator
//tempvar = table increment
//tempvar2 = envelope finished flag
//
//This function creates a table based on the attack, decay, and amplitude of the function call
//A table increment value is calculated based on the total envelope time (attack + decay)
//every audio rate tick will output (sigout) the value stored in table[index]
//and increment the index by the table increment value.
//This is very similar to how the oscillator works in this program, and is explained
//in more detail there.
void envelope(int patchcord, float attack, float decay, float amp, moduleData *data)
{
	//initialize
	if(data->initflag[patchcord] == 0)
	{
		int i;
		float total, Nattack, Ndecay;
		
		data->initflag[patchcord] = 1;
		data->tempvar2[patchcord] = 0;
		
		//total time in seconds		
		total = attack+decay;
		
		//determine how many points in our table the attack and decay times should take up
		Nattack = (attack / total) * ENV_SIZE;
		Ndecay = ENV_SIZE - Nattack;
		
		//generate a linear AD envelope
		for(i = 0; i < ENV_SIZE; i++)
		{
			//ramp up
			if(i < Nattack)
				data->table[patchcord][i] = i / Nattack;
				
			//ramp down
			else
				data->table[patchcord][i] = (Ndecay - (i - Nattack)) / Ndecay;
		}
		
		//set the first and last point of our envelope to 0
		data->table[patchcord][0] = 0;
		data->table[patchcord][ENV_SIZE] = 0;
		
		//determine table increment to control how fast the envelope table is stepped through
		data->tempvar[patchcord] = ((1/total) * ENV_SIZE/SAMPLE_RATE);	//tempvar = table increment
		
		//make sure we're at the beginning of the table
		data->tableIndex[patchcord] = 0;
	}

	//table look up method for moving through a table at different rates
	//This idea is explained more in the osc function.
	data->sigout[patchcord] = data->table[patchcord][(int)data->tableIndex[patchcord]];		//set the signal output to the current table value
	data->sigout[patchcord] *= amp;													//scale the signal by the amplitude
	data->tableIndex[patchcord] += data->tempvar[patchcord];							//increment the table index (tempvar is table increment)
	
	//one we've hit the end of the envelope...
	if(data->tableIndex[patchcord] > ENV_SIZE)
	{
		//set the envelope finished flag (tempvar2)
		data->tempvar2[patchcord] = 1;
		
		//and set the output signal to 0
		data->sigout[patchcord] = 0;
	}
}



//expenv - an exponential envelope generator
//tempvar - table increment
//tempvar2 - envelope finished flag
//
//expenv generates an exponential function instead of a linear one.
//It uses the same method as envelope() but instead draws an exponential function.
void expenv(int patchcord, float attack, float decay, float amp, moduleData *data)
{
	//init
	if(!data->initflag[patchcord])
	{
		data->initflag[patchcord] = 1;
		data->tempvar2[patchcord] = 0;
		
		float a, b;								//coefficients
		float total = attack + decay;			//total length in seconds
		int i;
		attack = (attack/total) * ENV_SIZE;		//how many points in the function table will be attack
		decay = ENV_SIZE - attack;				//how many points in the function table will be decay
		
		//determine coeficients for the equation f(x) = Ax^2 + Bx + C
		a = 1/(attack * attack);
		b = 1/(decay * decay);
		
		//fill function table with attack points
		for(i = 0; i < attack; i++)
			data->table[patchcord][i] = a*i*i;

		//fill function table with decay points
		for(i = 0; i < decay; i++)
			data->table[patchcord][i+(int)attack] = (b*i*i) - decay*2*b*i + decay*b + 1;
		
		//determine increment	
		data->tempvar[patchcord] = ((1/total) * ENV_SIZE/SAMPLE_RATE);	//tempvar = table increment
		data->tableIndex[patchcord] = 0;
	}
	//set sigout
	data->sigout[patchcord] = data->table[patchcord][(int)data->tableIndex[patchcord]];
	
	//amplify
	data->sigout[patchcord] *= amp;
	
	//increment
	data->tableIndex[patchcord] += data->tempvar[patchcord];
	
	//if envelope finished
	if(data->tableIndex[patchcord] > ENV_SIZE)
	{
		data->tempvar2[patchcord] = 1;
		data->sigout[patchcord] = 0;
	}
	
}



//osc - table look up oscillator
//tempvar = table increment
//
//This function is a great representation of how to achieve different oscillator rates
//with only one function table. The method is as follows:
//
// 1) generate a table (an array) of points for one full cycle of a waveform
//		This needs to be fairly large table, generally base 2, and at least 8192 points for good resolution
//		This table generation happens in the gen10 function of this program. The tables are referred to as ftables (function tables)
//
// 2) calculate a table increment value. This value will be how many points in the table will be skipped.
//		The higher the frequency, the higher the table increment. To make sense of this, the more points we skip,
//		the faster we'll draw a full waveform (which will produce a higher frequency).
//		The forumla for this is: increment = (frequency * table_size) / sample_rate
//
// 3) set the output to the current point in the table (table[index])
//
// 4) increase the table index by the table increment
//
// 5) repeat steps 2-4 at the sample rate (recalculating the table increment every time allows for variable frequency)

void osc(int patchcord, float freq, float amp, int ftable, moduleData *data)
{	
	//set table increment based on frequency (step 2)
	data->tempvar[patchcord] = (fabs(freq) * TABLE_LENGTH/SAMPLE_RATE);

	//set output (step 3)
	data->sigout[patchcord] = data->table[ftable][(int)data->tableIndex[patchcord]] * amp;
	
	//increment (step 4)
	data->tableIndex[patchcord] += data->tempvar[patchcord];
	
	//if index is greater than the table size, then loop back around to the beginning
	if(data->tableIndex[patchcord] > TABLE_LENGTH) data->tableIndex[patchcord] -= TABLE_LENGTH;
}


//white noise generator
void noise(int patchcord, float amp, moduleData *data)
{
	data->sigout[patchcord] = rand() % (int)(SAMPLE_RATE / 2);
	data->sigout[patchcord] /= (float)(SAMPLE_RATE / 2);
	data->sigout[patchcord] *= amp;
}






//onepole lowpass filter
//formula: output_sample = previous_sample + ((1 / sample_rate) * 2 * PI * frequency) * (input_sample - previous_sample)
void onePoleLow(int patchcord, float input, int freq, moduleData *data)
{
	data->sigout[patchcord]=data->sigout[patchcord] + (ISRX2P*freq) * (input - data->sigout[patchcord]);
}


//Max Mathews' two-pole filter modified to work as a highpass filter
void twoPoleHigh(int patchcord, float input, int freq, moduleData *data)
{
	float rr,phi; 
	float sigma = 1000;

	//initialize
	//This filter cannot take a varible cutoff frequency.
	//Once the frequency has been set, it can't be moved without reinitializing the entire filter.
	if(data->initflag[patchcord] == 0)
	{
		data->initflag[patchcord] = 1;
		
		//lots of math
		rr = exp(-sigma * ISRX2P);
		phi = ISRX2P * freq;    			//ISRX2P=(1/samplingrate)*2*pi 
		data->RCOS[patchcord] = rr * cos(phi);                    
		data->RSIN[patchcord] = rr * sin(phi);
		
		//set some starting values
		data->zz1[patchcord] = .01;
		data->yy1[patchcord] = .01;
	}

	//more math
	data->yy2[patchcord] = (data->RCOS[patchcord] * data->yy1[patchcord]) - (data->RSIN[patchcord] * (input + data->zz1[patchcord]));   
	data->zz2[patchcord] = (data->RSIN[patchcord] * data->yy1[patchcord]) + (data->RCOS[patchcord] * data->zz1[patchcord]);  
	data->zz1[patchcord] = data->zz2[patchcord];
	data->yy1[patchcord] = data->yy2[patchcord];
	
	//set output
	data->sigout[patchcord] = data->zz1[patchcord];
}

//averaging low pass filter
//tempvar is the previous sample
//forumla: output = (input + previous_sample) / 2
//This is a very terrible filter for audio
//The cutoff frequency is always SR/4
//This type of averaging low pass filter is often used to smooth out controller data
void averagingLowPass(int patchcord, float input, moduleData *data)
{
	if(data->initflag[patchcord] == 0)
	{
		data->tempvar[patchcord] = data->sigout[patchcord];
		data->initflag[patchcord] = 1;
	}
	
	data->sigout[patchcord] = (input + data->tempvar[patchcord]) / 2;
	data->tempvar[patchcord] = data->sigout[patchcord];
}


//averaging high pass filter
//tempvar is the previous sample
//formula: output = (input - previous_sample) / 2
//Same issues as the averaging high pass filter
//The cutoff frequency is always SR/4
//This is much more noticable when applied to audio than the low pass
void averagingHighPass(int patchcord, float input, moduleData *data)
{
	if(data->initflag[patchcord] == 0)
	{
		data->tempvar[patchcord] = data->sigout[patchcord];
		data->initflag[patchcord] = 1;
	}
	
	data->sigout[patchcord] = (input - data->tempvar[patchcord]) / 2;
	data->tempvar[patchcord] = data->sigout[patchcord];
}


	



//twopole filter  
// phi=(2pi/sampling_rate)*(freq) where freq is the "real frequency" of pole in hz
// rr=exp(- (2pi/sampling_rate)*(sigma) ) where sigma is "decay freq" of pole in hz 
//                         ALTERNATIVELY
// rr=exp((- 1/(decay*sampling_rate)) where decay is the time in seconds for impulse response to decay to 1/efloat twopole(float input)
//
//Filter programmed by Max Mathews. I have adapted it to work in this program.
void twopole(int patchcord, float input, float freq, moduleData *data)
{
	float rr,phi; 
	float sigma = 1000;

	//initialize
	//This filter cannot take a varible cutoff frequency.
	//Once the frequency has been set, it can't be moved without reinitializing the entire filter.
	if(data->initflag[patchcord] == 0)
	{
		data->initflag[patchcord] = 1;
		
		//lots of math
		rr = exp(-sigma * ISRX2P);
		phi = ISRX2P * freq;    			//ISRX2P=(1/samplingrate)*2*pi 
		data->RCOS[patchcord] = rr * cos(phi);                    
		data->RSIN[patchcord] = rr * sin(phi);
		
		//set some starting values
		data->zz1[patchcord] = .01;
		data->yy1[patchcord] = .01;
	}

	//more math
	data->yy2[patchcord] = (data->RCOS[patchcord] * data->yy1[patchcord]) - (data->RSIN[patchcord] * (data->zz1[patchcord] - input));   
	data->zz2[patchcord] = (data->RSIN[patchcord] * data->yy1[patchcord]) + (data->RCOS[patchcord] * data->zz1[patchcord]);  
	data->zz1[patchcord] = data->zz2[patchcord];
	data->yy1[patchcord] = data->yy2[patchcord];
	
	//set output
	data->sigout[patchcord] = data->zz1[patchcord];
}