/* 
 * MDFourier
 * A Fourier Transform analysis tool to compare different 
 * Sega Genesis/Mega Drive audio hardware revisions, and
 * other hardware in the future
 *
 * Copyright (C)2019 Artemio Urbina
 *
 * This file is part of the 240p Test Suite
 *
 * You can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
 *
 * Requires the FFTW library: 
 *	  http://www.fftw.org/
 * 
 * Compile with: 
 *	  gcc -Wall -std=gnu99 -o mdfourier mdfourier.c -lfftw3 -lm
 */

#include "freq.h"
#include "log.h"
#include "cline.h"

int IsCRTNoise(double freq)
{
	/* Peak around 15697-15698 usualy */
	if(freq >= 15620 && freq <= 15710)
		return 1;
	return 0;
}

AudioSignal *CreateAudioSignal(parameters *config)
{
	AudioSignal *Signal = NULL;

	if(!config)
		return NULL;

	if(!config->types.totalChunks)
		return NULL;

	Signal = (AudioSignal*)malloc(sizeof(AudioSignal));
	if(!Signal)
	{
		logmsg("Not enough memory for Data Structures\n");
		return NULL;
	}

	Signal->Blocks = (AudioBlocks*)malloc(sizeof(AudioBlocks)*config->types.totalChunks);
	if(!Signal->Blocks)
	{
		logmsg("Not enough memory for Data Structures\n");	
		return NULL;
	}

	for(int n = 0; n < config->types.totalChunks; n++)
	{
		Signal->Blocks[n].freq = (Frequency*)malloc(sizeof(Frequency)*config->MaxFreq);
		if(!Signal->Blocks[n].freq)
		{
			logmsg("Not enough memory for Data Structures\n");
			return NULL;
		}
	}

	CleanAudio(Signal, config);
	return Signal;
}

void CleanAudio(AudioSignal *Signal, parameters *config)
{
	if(!Signal)
		return;

	if(!Signal->Blocks)
		return;

	for(int n = 0; n < config->types.totalChunks; n++)
	{
		if(Signal->Blocks[n].freq)
		{
			for(int i = 0; i < config->MaxFreq; i++)
			{
				Signal->Blocks[n].freq[i].hertz = 0;
				Signal->Blocks[n].freq[i].magnitude = 0;
				Signal->Blocks[n].freq[i].amplitude = 0;
				Signal->Blocks[n].freq[i].phase = 0;
				Signal->Blocks[n].freq[i].matched = 0;
			}
		}
		Signal->Blocks[n].fftwValues.spectrum = NULL;
		Signal->Blocks[n].fftwValues.size = 0;
		Signal->Blocks[n].fftwValues.seconds = 0;
	}
	Signal->SourceFile[0] = '\0';
	Signal->hasFloor = 0;
	Signal->floorFreq = 0.0;
	Signal->floorAmplitude = 0.0;	
}

void ReleaseAudio(AudioSignal *Signal, parameters *config)
{
	if(!Signal)
		return;

	if(!Signal->Blocks)
		return;

	for(int n = 0; n < config->types.totalChunks; n++)
	{
		if(Signal->Blocks[n].freq)
		{
			free(Signal->Blocks[n].freq);
			Signal->Blocks[n].freq = NULL;
		}

		if(Signal->Blocks[n].fftwValues.spectrum)
		{
			fftw_free(Signal->Blocks[n].fftwValues.spectrum);
			Signal->Blocks[n].fftwValues.spectrum = NULL;
		}
		Signal->Blocks[n].fftwValues.spectrum = NULL;
		Signal->Blocks[n].fftwValues.size = 0;
		Signal->Blocks[n].fftwValues.seconds = 0;
	}

	free(Signal->Blocks);
	Signal->Blocks = NULL;

	Signal->SourceFile[0] = '\0';
	Signal->hasFloor = 0;
	Signal->floorFreq = 0.0;
	Signal->floorAmplitude = 0.0;	
}

void ReleaseAudioBlockStructure(parameters *config)
{
	if(config->types.typeCount && config->types.typeArray)
	{
		free(config->types.typeArray);
		config->types.typeArray = NULL;
		config->types.typeCount = 0;
	}
}

int LoadAudioBlockStructure(parameters *config)
{
	FILE 	*file;
	char	buffer[512];

	if(!config)
		return 0;

	file = fopen("mdfblocks.mfn", "r");
	if(!file)
	{
		printf("Could not load audio configuiration file mdblocks.mfn\n");
		return 0;
	}
	
	fscanf(file, "%s ", buffer);
	if(strcmp(buffer, "MDFourierAudioBlockFile") != 0)
	{
		printf("Not an MD Fourier Audio Block File\n");
		fclose(file);
		return 0;
	}
	fscanf(file, "%s\n", buffer);
	if(atof(buffer) > 1.0)
	{
		printf("This executable can parse 1.0 files only\n");
		fclose(file);
		return 0;
	}
	if(fscanf(file, "%s\n", config->types.Name) != 1)
	{
		printf("Invalid Name '%s'\n", buffer);
		fclose(file);
		return 0;
	}

	if(fscanf(file, "%s\n", buffer) != 1)
	{
		printf("Invalid Frame Rate Adjustment '%s'\n", buffer);
		fclose(file);
		return 0;
	}
	config->types.framerateAdjust = strtod(buffer, NULL);
	if(!config->types.framerateAdjust)
	{
		printf("Invalid Frame Rate Adjustment '%s'\n", buffer);
		fclose(file);
		return 0;
	}

	fscanf(file, "%s\n", buffer);
	config->types.typeCount = atoi(buffer);
	if(!config->types.typeCount)
	{
		printf("Invalid type count '%s'\n", buffer);
		fclose(file);
		return 0;
	}
	config->types.typeArray = (AudioBlockType*)malloc(sizeof(AudioBlockType)*config->types.typeCount);
	if(!config->types.typeArray)
	{
		printf("Not enough memory\n");
		fclose(file);
		return 0;
	}

	for(int i = 0; i < config->types.typeCount; i++)
	{
		if(fscanf(file, "%s %d %d %f\n", 
			config->types.typeArray[i].typeName,
			&config->types.typeArray[i].type,
			&config->types.typeArray[i].elementCount,
			&config->types.typeArray[i].seconds) != 4)
		{
			printf("Invalid MD Fourier Audio Blocks File\n");
			fclose(file);
			return 0;
		}
	}

	config->types.regularChunks = GetTotalAudioBlocks(config);
	config->types.totalChunks = GetTotalAudioBlocksWithSilence(config);
	if(!config->types.totalChunks)
	{
		printf("Total Audio Blocks should be at least 1\n");
		return 0;
	}

	fclose(file);
	
	return 1;
}

double GetFramerateAdjust(parameters *config)
{
	return(config->types.framerateAdjust);
}

int GetSilenceIndex(parameters *config)
{
	int index = 0;

	if(!config)
		return -1;

	for(int i = 0; i < config->types.typeCount; i++)
	{
		if(config->types.typeArray[i].type == TYPE_SILENCE)
			return index;
		else
			index += config->types.typeArray[i].elementCount;
	}
	return -1;
}

int GetTotalAudioBlocks(parameters *config)
{
	int count = 0;

	if(!config)
		return -1;

	for(int i = 0; i < config->types.typeCount; i++)
	{
		if(config->types.typeArray[i].type != TYPE_SILENCE)
			count += config->types.typeArray[i].elementCount;
	}
	return count;
}

double GetLongestBlock(parameters *config)
{
	double longest = 0;

	if(!config)
		return 0;

	for(int i = 0; i < config->types.typeCount; i++)
	{
		if(config->types.typeArray[i].seconds > longest)
			longest = config->types.typeArray[i].seconds;
	}
	return longest;
}

double GetTotalBlockDuration(parameters *config)
{
	double total = 0;

	if(!config)
		return 0;

	for(int i = 0; i < config->types.typeCount; i++)
		total += config->types.typeArray[i].seconds *
			config->types.typeArray[i].elementCount;

	return total;
}

double GetBlockDuration(parameters *config, int pos)
{
	int elementsCounted = 0;

	if(!config)
		return 0;

	for(int i = 0; i < config->types.typeCount; i++)
	{
		elementsCounted += config->types.typeArray[i].elementCount;
		if(elementsCounted > pos)
			return(config->types.typeArray[i].seconds);
	}
	
	return 0;
}

char *GetBlockName(parameters *config, int pos)
{
	int elementsCounted = 0;

	if(!config)
		return 0;

	for(int i = 0; i < config->types.typeCount; i++)
	{
		elementsCounted += config->types.typeArray[i].elementCount;
		if(elementsCounted > pos)
			return(config->types.typeArray[i].typeName);
	}
	
	return 0;
}

int GetBlockSubIndex(parameters *config, int pos)
{
	int elementsCounted = 0, last = 0;

	if(!config)
		return 0;

	for(int i = 0; i < config->types.typeCount; i++)
	{
		elementsCounted += config->types.typeArray[i].elementCount;
		if(elementsCounted > pos)
			return(pos - last);
		last = elementsCounted;
	}
	
	return 0;
}

int GetBlockType(parameters *config, int pos)
{
	int elementsCounted = 0;

	if(!config)
		return TYPE_NOTYPE;

	for(int i = 0; i < config->types.typeCount; i++)
	{
		elementsCounted += config->types.typeArray[i].elementCount;
		if(elementsCounted > pos)
			return(config->types.typeArray[i].type);
	}
	
	return TYPE_NOTYPE;
}

int GetTotalAudioBlocksWithSilence(parameters *config)
{
	int count = 0;

	if(!config)
		return -1;

	for(int i = 0; i < config->types.typeCount; i++)
		count += config->types.typeArray[i].elementCount;

	return count;
}

void FindFloor(AudioSignal *Signal, parameters *config)
{
	int index;

	if(!Signal)
		return;
	
	if(!Signal->hasFloor)
		return;

	index = GetSilenceIndex(config);
	if(index == -1)
	{
		logmsg("There is no Silence block defined in the current format\n");
		return;
	}

	for(int i = 0; i < config->MaxFreq; i++)
	{
		if(!IsCRTNoise(Signal->Blocks[index].freq[i].hertz))
		{
			Signal->floorAmplitude = Signal->Blocks[index].freq[i].amplitude;
			Signal->floorFreq = Signal->Blocks[index].freq[i].hertz;
			logmsg("Found 'Silence' block: %g Hz at %0.4f.db\n",
				Signal->floorFreq, Signal->floorAmplitude);
			return;
		}
	}
	Signal->hasFloor = 0;  /* revoke it if not found */
}

void GlobalNormalize(AudioSignal *Signal, parameters *config)
{
	double MaxMagnitude = 0;

	if(!Signal)
		return;

	/* Find global peak */
	if(config->normalize == 'g' ||
	  (config->normalize == 'r' && config->relativeMaxMagnitude == 0.0))
	{
		for(int block = 0; block < config->types.totalChunks; block++)
		{
			for(int i = 0; i < config->MaxFreq; i++)
			{
				if(!Signal->Blocks[block].freq[i].hertz)
					break;
				if(Signal->Blocks[block].freq[i].magnitude > MaxMagnitude)
					MaxMagnitude = Signal->Blocks[block].freq[i].magnitude;
			}
		}

		if(config->normalize == 'r')
			config->relativeMaxMagnitude = MaxMagnitude;
	}

	if(config->normalize == 'r' && config->relativeMaxMagnitude != 0.0)
		MaxMagnitude = config->relativeMaxMagnitude;

	/* Normalize and calculate Amplitude in dbs */
	for(int block = 0; block < config->types.totalChunks; block++)
	{
		for(int i = 0; i < config->MaxFreq; i++)
		{
			if(!Signal->Blocks[block].freq[i].hertz)
				break;
			Signal->Blocks[block].freq[i].amplitude = 
				20*log10(Signal->Blocks[block].freq[i].magnitude / MaxMagnitude);
			Signal->Blocks[block].freq[i].magnitude = 
				Signal->Blocks[block].freq[i].magnitude * 100.0 / MaxMagnitude;
		}
	}
}

/* Do per block normalization if requested */
/* This makes it so relative channel/block */
/* volume is ignored at a global level */
void LocalNormalize(AudioBlocks *AudioArray, parameters *config)
{
	double MaxMagnitude = 0;

	if(!AudioArray)
		return;

	/* Find MaxMagnitude for Normalization */
	for(long int i = 0; i < config->MaxFreq; i++)
	{
		if(AudioArray->freq[i].hertz)
		{
			if(AudioArray->freq[i].magnitude > MaxMagnitude)
				MaxMagnitude = AudioArray->freq[i].magnitude;
		}
	}
 
	/* Normalize to 100 */
	/* Calculate per Block dbs */
	for(long int i = 0; i < config->MaxFreq; i++)
	{
		AudioArray->freq[i].amplitude = 
			20*log10(AudioArray->freq[i].magnitude/MaxMagnitude);
		AudioArray->freq[i].magnitude = 
			AudioArray->freq[i].magnitude*100.0/MaxMagnitude;
	}
}

void CleanMatched(AudioSignal *ReferenceSignal, AudioSignal *TestSignal, parameters *config)
{
	for(int block = 0; block < config->types.totalChunks; block++)
	{
		for(int i = 0; i < config->MaxFreq; i++)
		{
			if(!ReferenceSignal->Blocks[block].freq[i].hertz)
				break;
			ReferenceSignal->Blocks[block].freq[i].matched = 0;	
		}
	}

	for(int block = 0; block < config->types.totalChunks; block++)
	{
		for(int i = 0; i < config->MaxFreq; i++)
		{
			if(!TestSignal->Blocks[block].freq[i].hertz)
				break;
			TestSignal->Blocks[block].freq[i].matched = 0;
		}
	}
}

void PrintFrequencies(AudioSignal *Signal, parameters *config)
{
	if(IsLogEnabled())
			DisableConsole();

	for(int block = 0; block < config->types.totalChunks; block++)
	{
		logmsg("==================== %s# %d (%d) ===================\n", GetBlockName(config, block), GetBlockSubIndex(config, block), block);
			if(config->spreadsheet)
				logmsg("Spreadsheet-%s#%d\n", GetBlockName(config, block), GetBlockSubIndex(config, block));

		for(int j = 0; j < config->MaxFreq; j++)
		{
			if(Signal->Blocks[block].freq[j].hertz)
			{
				logmsg("Frequency [%2d] %7g Hz [Amplitude: %g]",
					j, 
					Signal->Blocks[block].freq[j].hertz,
					Signal->Blocks[block].freq[j].amplitude);
				/* detect CRT frequency */
				if(IsCRTNoise(Signal->Blocks[block].freq[j].hertz))
					logmsg(" *** CRT Noise ***");
				logmsg("\n");
			}
	
			if(config->spreadsheet)
			{
				logmsg("Spreadsheet-index-Hz-amplitude, %d, %g, %g\n",
					j, Signal->Blocks[block].freq[j].hertz, Signal->Blocks[block].freq[j].amplitude);
			}

			if(config->debugVerbose && j == 20)  /* this is just for internal quick debugging */
				exit(1);
		}
	}

	if(IsLogEnabled())
		EnableConsole();
}

void FillFrequencyStructures(AudioBlocks *AudioArray, parameters *config)
{
	long int i = 0;
	double boxsize = 0, size = 0;

	size = AudioArray->fftwValues.size;
	boxsize = AudioArray->fftwValues.seconds;
	/* for(i = 1; i < monoSignalSize/2+1; i++) */
	for(i = config->startHz*boxsize; i < config->endHz*boxsize; i++)	/* Nyquist at 44.1khz */
	{
		double r1 = creal(AudioArray->fftwValues.spectrum[i]);
		double i1 = cimag(AudioArray->fftwValues.spectrum[i]);
		double magnitude, previous;
		int    j = 0;
		double Hertz;

		/* magnitude = 2*(r1*r1 + i1*i1)/(monoSignalSize*monoSignalSize); */
 		/* amplitude = 10 * log10(magnitude+DBL_EPSILON); */
		magnitude = sqrt(r1*r1 + i1*i1)/size;
		Hertz = ((double)i/boxsize);

		previous = 1.e30;
		/* printf("%g Hz %g m (old %g) %g dbs\n", Hertz, m, magnitude, a); */
		if(!IsCRTNoise(Hertz))
		{
			for(j = 0; j < config->MaxFreq; j++)
			{
				if(magnitude > AudioArray->freq[j].magnitude && magnitude < previous)
				{
					/* Move the previous values down the array */
					for(int k = config->MaxFreq-1; k > j; k--)
						AudioArray->freq[k] = AudioArray->freq[k - 1];
	
					AudioArray->freq[j].hertz = Hertz;
					AudioArray->freq[j].magnitude = magnitude;
					AudioArray->freq[j].amplitude = 0;
					AudioArray->freq[j].phase = atan2(i1, r1);
					break;
				}
				previous = AudioArray->freq[j].magnitude;
			}
		}
	}
}

void CompressFrequencies(AudioBlocks *AudioArray, parameters *config)
{
	long int i = 0;

	/* The following mess compressed adjacent frequencies */
	/* Disabled by default and it is not as useful as expected in the current form */
	
	for(i = 0; i < config->MaxFreq; i++)
	{
		for(int j = 0; j < config->MaxFreq; j++)
		{
			double hertzDiff;

			hertzDiff = fabs(AudioArray->freq[j].hertz - AudioArray->freq[i].hertz);
			
			if(AudioArray->freq[i].hertz && AudioArray->freq[j].hertz
				&& i != j && hertzDiff <= config->HzWidth)
			{
				if(AudioArray->freq[i].magnitude > AudioArray->freq[j].magnitude)
				{
					AudioArray->freq[i].magnitude += AudioArray->freq[j].magnitude;
					/* AudioArray->freq[i].magnitude/= 2; */

					AudioArray->freq[j].hertz = 0;
					AudioArray->freq[j].magnitude = 0;
					AudioArray->freq[j].phase = 0;
					AudioArray->freq[i].amplitude = 0;
				}
				else
				{
					AudioArray->freq[j].magnitude += AudioArray->freq[i].magnitude;
					/* AudioArray->freq[j].magnitude/= 2; */

					AudioArray->freq[i].hertz = 0;
					AudioArray->freq[i].magnitude = 0;
					AudioArray->freq[i].phase = 0;
					AudioArray->freq[i].amplitude = 0;
				}
			}
		}
	}

	/* sort array by magnitude */
	for(i = 0; i < config->MaxFreq; i++)
	{
		for(int j = i + 1; j < config->MaxFreq; j++)
		{
			if (AudioArray->freq[i].magnitude < AudioArray->freq[j].magnitude) 
			{
					Frequency	t;
				
					t = AudioArray->freq[i];
					AudioArray->freq[i] = AudioArray->freq[j];
					AudioArray->freq[j] = t;
				}
		}
	}
}

void InsertMessageInBuffer(msgbuff *message, parameters *config)
{
	if(config->justResults)
		return;

	strcat(message->message, message->buffer);
	message->msgPos = strlen(message->message);
	if(message->msgPos > message->msgSize - 512)
	{
		message->message = (char*)realloc(message->message, (message->msgSize+4096)*(sizeof(char)));
		message->msgSize += 4096;
	}
}

void PrintComparedBlocks(AudioBlocks *ReferenceArray, AudioBlocks *ComparedArray, parameters *config, AudioSignal *Signal)
{
	if(IsLogEnabled())
		DisableConsole();

	/* changed Magnitude->amplitude */
	for(int j = 0; j < config->MaxFreq; j++)
	{
		if(ReferenceArray->freq[j].hertz)
		{
			int match = 0;

			logmsg("[%5d] Ref: %7g Hz %6.2fdb [>%3d]", 
						j,
						ReferenceArray->freq[j].hertz,
						ReferenceArray->freq[j].amplitude,
						ReferenceArray->freq[j].matched - 1);

			if(ComparedArray->freq[j].hertz)
				logmsg("\tComp: %7g Hz %6.2fdb [<%3d]", 
						ComparedArray->freq[j].hertz,
						ComparedArray->freq[j].amplitude,
						ComparedArray->freq[j].matched - 1);
			else
				logmsg("\tCompared:\tNULL");
			match = ReferenceArray->freq[j].matched - 1;
			if(match != -1 &&
				ReferenceArray->freq[j].hertz != 
				ComparedArray->freq[match].hertz)
					logmsg("H");
			else
					logmsg(" ");
			if(match != -1 &&
				ReferenceArray->freq[j].amplitude != 
				ComparedArray->freq[match].amplitude)
					logmsg("W");
			logmsg("\n");
		}
	}
	logmsg("\n\n");

	if(IsLogEnabled())
		EnableConsole();
}
