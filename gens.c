//GEN ROUTINES

#include "modular.h"

//Gen 10 - fill a function table based on a sum of sines.
//
//Idea taken from Csound. This gen routine
//generates a periodic waveform by adding harmonics
//together at user defined amplitudes.
void gen10(int ftable, float* amps, int nSines, moduleData *data)
{
	int i, j;
	float max = 0;			//variable used to keep the maximum amplitude of the table 1
	
	//loop once through the entire ftable to be filled up
	for(i = 0; i < TABLE_LENGTH; i++)
	{
		
		//loop through the total number of harmonics and the fundamental (nSines).
		for(j = 0; j < nSines; j++)
		{
			//calculate the next value of our table with the following formula:
			//f(x) = (sin(2*PI*x / table_size) * fundamental) + (sin((2*PI*x / table_size) * 2) * harmonic[1]) ... + (sin((2*PI*x / table_size) * N) * harmonic[N - 1])
			data->table[ftable][i] += (sin((2.0*PI*i/(float)TABLE_LENGTH)*(j+1))) * amps[j];
		}
		if(fabs(data->table[ftable][i]) > max) max = fabs(data->table[ftable][i]);		//Store the maximum amplitude value to be divided later
	}
	
	//divide everything by the maximum value to scale everything from -1 to 1
	for(i = 0; i < TABLE_LENGTH; i++)
		data->table[ftable][i] /= max;
}