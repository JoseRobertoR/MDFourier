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
 */

#include "plot.h"
#include "log.h"
#include "freq.h"
#include "cline.h"
#include "windows.h"

#define PLOT_RES_X 1600
#define PLOT_RES_Y 800

int FillPlot(PlotFile *plot, char *name, int sizex, int sizey, double x0, double y0, double x1, double y1, double penWidth, parameters *config)
{
	plot->plotter = NULL;
	plot->plotter_params = NULL;
	plot->file = NULL;

	ComposeFileName(plot->FileName, name, ".png", config);

	plot->sizex = sizex;
	plot->sizey = sizey;
	plot->x0 = x0;
	plot->y0 = y0;
	plot->x1 = x1;
	plot->y1 = y1;
	plot->penWidth = penWidth;
	return 1;
}

int CreatePlotFile(PlotFile *plot)
{
	char		size[20];

	plot->file = fopen(plot->FileName, "wb");
	if(!plot->file)
	{
		logmsg("Couldn't create Plot file %s\n", plot->FileName);
		return 0;
	}

	sprintf(size, "%dx%d", plot->sizex, plot->sizey);
	plot->plotter_params = pl_newplparams ();
	pl_setplparam (plot->plotter_params, "BITMAPSIZE", size);
	if((plot->plotter = pl_newpl_r("png", stdin, plot->file, stderr, plot->plotter_params)) == NULL)
	{
		logmsg("Couldn't create Plotter\n");
		return 0;
	}

	if(pl_openpl_r(plot->plotter) < 0)
	{
		logmsg("Couldn't open Plotter\n");
		return 0;
	}
	pl_fspace_r(plot->plotter, plot->x0, plot->y0, plot->x1, plot->y1);
	pl_flinewidth_r(plot->plotter, plot->penWidth);
	pl_bgcolor_r(plot->plotter, 0, 0, 0);
	pl_erase_r(plot->plotter);

	return 1;
}

int ClosePlot(PlotFile *plot)
{
	if(pl_closepl_r(plot->plotter) < 0)
	{
		logmsg("Couldn't close Plotter\n");
		return 0;
	}
	
	if(pl_deletepl_r(plot->plotter) < 0)
	{
		logmsg("Couldn't delete Plotter\n");
		return 0;
	}

	if(pl_deleteplparams(plot->plotter_params) < 0)
	{
		logmsg("Couldn't delete Plotter Params\n");
		return 0;
	}

	plot->plotter = NULL;
	plot->file = NULL;
	plot->plotter_params = NULL;

	fclose(plot->file);
	return 1;
}

void PlotResults(AudioSignal *Signal, parameters *config)
{
	FlatAmplDifference *amplDiff;
	FlatFreqDifference *freqDiff;

	logmsg("* Plotting results to PNGs\n");

	amplDiff = CreateFlatDifferences(config);
	if(!amplDiff)
	{
		logmsg("Not enough memory for plotting\n");
		return;
	}

	if(PlotEachTypeDifferentAmplitudes(amplDiff, config->compareName, config) > 1)
		PlotAllDifferentAmplitudes(amplDiff, config->compareName, config);

	free(amplDiff);
	amplDiff = NULL;

	PlotAllSpectrogram(basename(Signal->SourceFile), Signal, config);

	freqDiff = CreateFlatMissing(config);
	if(PlotEachTypeMissingFrequencies(freqDiff, config->compareName, config) > 1)
		PlotAllMissingFrequencies(freqDiff, config->compareName, config);

	free(amplDiff);
	amplDiff = NULL;
}

void PlotAllDifferentAmplitudes(FlatAmplDifference *amplDiff, char *filename, parameters *config)
{
	PlotFile	plot;
	char		name[2048];
	double		dbs = 15;

	if(!config)
		return;

	if(!amplDiff)
		return;

	if(!config->Differences.BlockDiffArray)
		return;

	sprintf(name, "DifferentAmplitudes_%s", filename);
	FillPlot(&plot, name, PLOT_RES_X, PLOT_RES_Y, 0, -1*dbs, 20000, dbs, 1, config);

	if(!CreatePlotFile(&plot))
		return;

	pl_pencolor_r (plot.plotter, 0, 0xcccc, 0);
	pl_fline_r(plot.plotter, 0, 0, 20000, 0);

	pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
	for(int i = 3; i < dbs; i += 3)
	{
		pl_fline_r(plot.plotter, 0, i, 20000, i);
		pl_fline_r(plot.plotter, 0, -1*i, 20000, -1*i);
	}

	pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
	for(int i = 0; i < 20000; i += 1000)
		pl_fline_r(plot.plotter, i, -1*dbs, i, dbs);

	pl_pencolor_r (plot.plotter, 0, 0xFFFF, 0);

	for(int a = 0; a < config->Differences.cntAmplAudioDiff; a++)
	{
		if(amplDiff[a].type > TYPE_CONTROL)
		{ 
			double range_0_1;
			long int color;
			double intensity;	

			range_0_1 = (fabs(config->significantVolume) - fabs(amplDiff[a].refAmplitude))/fabs(config->significantVolume);
			intensity = CalculateWeightedError(range_0_1, config);
			color = intensity*0xffff;

			SetPenColor(amplDiff[a].color, color, &plot);
			pl_fpoint_r(plot.plotter, amplDiff[a].hertz, amplDiff[a].diffAmplitude);
		}
	}

	pl_fspace_r (plot.plotter, 0.0, -1*PLOT_RES_Y, PLOT_RES_X, PLOT_RES_Y);
	pl_pencolor_r (plot.plotter, 0, 0xcccc, 0);
	pl_ffontsize_r (plot.plotter, PLOT_RES_Y/20);

	pl_fmove_r (plot.plotter, PLOT_RES_X-(PLOT_RES_X/20), 10);
	pl_alabel_r (plot.plotter, 'c', 'c', "0 db");

	ClosePlot(&plot);
}

int PlotEachTypeDifferentAmplitudes(FlatAmplDifference *amplDiff, char *filename, parameters *config)
{
	int 		i = 0, type = 0, types = 0;
	char		name[2048];

	for(i = 0; i < config->types.typeCount; i++)
	{
		type = config->types.typeArray[i].type;
		if(type > TYPE_CONTROL)
		{
			sprintf(name, "DifferentAmplitudes_%s_%s", filename, config->types.typeArray[i].typeName);
			PlotSingleTypeDifferentAmplitudes(amplDiff, type, name, config);
			types ++;
		}
	}
	return types;
}

void PlotSingleTypeDifferentAmplitudes(FlatAmplDifference *amplDiff, int type, char *filename, parameters *config)
{
	PlotFile	plot;
	double		dbs = 15;

	if(!config)
		return;

	if(!amplDiff)
		return;

	if(!config->Differences.BlockDiffArray)
		return;

	FillPlot(&plot, filename, PLOT_RES_X, PLOT_RES_Y, 0, -1*dbs, 20000, dbs, 1, config);

	if(!CreatePlotFile(&plot))
		return;

	pl_pencolor_r (plot.plotter, 0, 0xcccc, 0);
	pl_fline_r(plot.plotter, 0, 0, 20000, 0);

	pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
	for(int i = 3; i < dbs; i += 3)
	{
		pl_fline_r(plot.plotter, 0, i, 20000, i);
		pl_fline_r(plot.plotter, 0, -1*i, 20000, -1*i);
	}

	pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
	for(int i = 0; i < 20000; i += 1000)
		pl_fline_r(plot.plotter, i, -1*dbs, i, dbs);

	pl_pencolor_r (plot.plotter, 0, 0xFFFF, 0);

	for(int a = 0; a < config->Differences.cntAmplAudioDiff; a++)
	{
		if(amplDiff[a].type == type)
		{ 
			double range_0_1;
			long int color;
			double intensity;	

			range_0_1 = (fabs(config->significantVolume) - fabs(amplDiff[a].refAmplitude))/fabs(config->significantVolume);
			intensity = CalculateWeightedError(range_0_1, config);
			color = intensity*0xffff;

			SetPenColor(amplDiff[a].color, color, &plot);
			pl_fpoint_r(plot.plotter, amplDiff[a].hertz, amplDiff[a].diffAmplitude);
		}
	}

	pl_fspace_r (plot.plotter, 0.0, -1*PLOT_RES_Y, PLOT_RES_X, PLOT_RES_Y);
	pl_pencolor_r (plot.plotter, 0, 0xcccc, 0);
	pl_ffontsize_r (plot.plotter, PLOT_RES_Y/20);

	pl_fmove_r (plot.plotter, PLOT_RES_X-(PLOT_RES_X/20), 10);
	pl_alabel_r (plot.plotter, 'c', 'c', "0 db");

	ClosePlot(&plot);
}

void PlotAllMissingFrequencies(FlatFreqDifference *freqDiff, char *filename, parameters *config)
{
	PlotFile plot;
	char	 name[2048];

	if(!config)
		return;

	if(!config->Differences.BlockDiffArray)
		return;

	sprintf(name, "MissingFrequencies_%s", filename);
	FillPlot(&plot, name, PLOT_RES_X, PLOT_RES_Y, 0, config->significantVolume, 20000, 0.0, 1, config);

	if(!CreatePlotFile(&plot))
		return;

	pl_pencolor_r (plot.plotter, 0, 0xaaaa, 0);
	pl_fline_r(plot.plotter, 0, 0, 20000, 0);

	pl_pencolor_r (plot.plotter, 0, 0x7777, 0);
	for(int i = 3; i < fabs(config->significantVolume); i += 3)
		pl_fline_r(plot.plotter, 0, -1*i, 20000, -1*i);

	pl_pencolor_r (plot.plotter, 0, 0x7777, 0);
	for(int i = 0; i < 20000; i += 1000)
		pl_fline_r(plot.plotter, i, config->significantVolume, i, 0);

	pl_pencolor_r (plot.plotter, 0, 0xFFFF, 0);
	for(int f = 0; f < config->Differences.cntFreqAudioDiff; f++)
	{
		if(freqDiff[f].type > TYPE_CONTROL)
		{ 
			double range_0_1;
			long int color;
			double intensity;

			range_0_1 = (fabs(config->significantVolume) - fabs(freqDiff[f].amplitude))/fabs(config->significantVolume);
			intensity = CalculateWeightedError(range_0_1, config);
			color = intensity*0xffff;

			SetPenColor(freqDiff[f].color, color, &plot);
			pl_fpoint_r(plot.plotter, freqDiff[f].hertz, 
					freqDiff[f].amplitude);
		}
	}
	ClosePlot(&plot);
}

int PlotEachTypeMissingFrequencies(FlatFreqDifference *freqDiff, char *filename, parameters *config)
{
	int 		i = 0, type = 0, types = 0;
	char		name[2048];

	for(i = 0; i < config->types.typeCount; i++)
	{
		type = config->types.typeArray[i].type;
		if(type > TYPE_CONTROL)
		{
			sprintf(name, "MissingFrequencies_%s_%s", filename, config->types.typeArray[i].typeName);
			PlotSingleTypeMissingFrequencies(freqDiff, type, name, config);
			types ++;
		}
	}
	return types;
}

void PlotSingleTypeMissingFrequencies(FlatFreqDifference *freqDiff, int type, char *filename, parameters *config)
{
	PlotFile plot;

	if(!config)
		return;

	if(!config->Differences.BlockDiffArray)
		return;

	FillPlot(&plot, filename, PLOT_RES_X, PLOT_RES_Y, 0, config->significantVolume, 20000, 0.0, 1, config);

	if(!CreatePlotFile(&plot))
		return;

	pl_pencolor_r (plot.plotter, 0, 0xaaaa, 0);
	pl_fline_r(plot.plotter, 0, 0, 20000, 0);

	pl_pencolor_r (plot.plotter, 0, 0x7777, 0);
	for(int i = 3; i < fabs(config->significantVolume); i += 3)
		pl_fline_r(plot.plotter, 0, -1*i, 20000, -1*i);

	pl_pencolor_r (plot.plotter, 0, 0x7777, 0);
	for(int i = 0; i < 20000; i += 1000)
		pl_fline_r(plot.plotter, i, config->significantVolume, i, 0);

	pl_pencolor_r (plot.plotter, 0, 0xFFFF, 0);
	for(int f = 0; f < config->Differences.cntFreqAudioDiff; f++)
	{
		if(freqDiff[f].type == type)
		{
			double range_0_1;
			long int color;
			double intensity;

			range_0_1 = (fabs(config->significantVolume) - fabs(freqDiff[f].amplitude))/fabs(config->significantVolume);
			intensity = CalculateWeightedError(range_0_1, config);
			color = intensity*0xffff;

			SetPenColor(freqDiff[f].color, color, &plot);
			pl_fpoint_r(plot.plotter, freqDiff[f].hertz, 
					freqDiff[f].amplitude);
		}
	}
	ClosePlot(&plot);
}

void PlotAllSpectrogram(char *filename, AudioSignal *Signal, parameters *config)
{
	PlotFile plot;
	char	 name[2048];

	if(!config)
		return;

	sprintf(name, "Spectrogram_%s", filename);
	FillPlot(&plot, name, PLOT_RES_X, PLOT_RES_Y, 0, config->significantVolume, 20000, 0.0, 1, config);

	if(!CreatePlotFile(&plot))
		return;

	pl_pencolor_r (plot.plotter, 0, 0xbbbb, 0);
	pl_fline_r(plot.plotter, 0, 0, 20000, 0);

	pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
	for(int i = 3; i < fabs(config->significantVolume); i += 3)
		pl_fline_r(plot.plotter, 0, -1*i, 20000, -1*i);

	pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
	for(int i = 0; i < 20000; i += 1000)
		pl_fline_r(plot.plotter, i, config->significantVolume, i, 0);

	pl_pencolor_r (plot.plotter, 0, 0xFFFF, 0);
	
	for(int b = 0; b < config->types.totalChunks; b++)
	{
		int type;

		type = GetBlockType(config, b);
		if(type > TYPE_CONTROL)
		{ 
			for(int i = 0; i < config->MaxFreq; i++)
			{
				double range_0_1;
				long int color;
				double intensity;
		
				range_0_1 = (fabs(config->significantVolume) - fabs(Signal->Blocks[b].freq[i].amplitude))/fabs(config->significantVolume);
				intensity = CalculateWeightedError(range_0_1, config);
				color = intensity*0xffff;
		
				SetPenColorStr(GetBlockColor(config, b), color, &plot);
				pl_fpoint_r(plot.plotter, Signal->Blocks[b].freq[i].hertz, 
						Signal->Blocks[b].freq[i].amplitude);
			}
		}
	}
	ClosePlot(&plot);
}

/*
void PlotAllSpectrogramLineBased(char *filename, AudioSignal *Signal, parameters *config)
{
	PlotFile plot;
	char	 name[2048];

	if(!config)
		return;

	sprintf(name, "Spectrogram_Line_%s", filename);
	FillPlot(&plot, name, PLOT_RES_X, PLOT_RES_Y, 0, config->significantVolume, 20000, 0.0, 1, config);

	if(!CreatePlotFile(&plot))
		return;

	pl_pencolor_r (plot.plotter, 0, 0xbbbb, 0);
	pl_fline_r(plot.plotter, 0, 0, 20000, 0);

	pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
	for(int i = 3; i < fabs(config->significantVolume); i += 3)
		pl_fline_r(plot.plotter, 0, -1*i, 20000, -1*i);

	pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
	for(int i = 0; i < 20000; i += 1000)
		pl_fline_r(plot.plotter, i, config->significantVolume, i, 0);

	pl_pencolor_r (plot.plotter, 0, 0xFFFF, 0);
	
	SortFrequencies(Signal, config);

	for(int b = 0; b < config->types.totalChunks; b++)
	{
		int type;

		pl_fline_r(plot.plotter, 0, -1*config->significantVolume, 1, -1*config->significantVolume);
		type = GetBlockType(config, b);
		if(type > TYPE_CONTROL)
		{ 
			pl_pencolorname_r(plot.plotter, GetBlockColor(config, b));
			for(int i = 0; i < config->MaxFreq; i++)
			{
				double range_0_1;
				long int color;
				double intensity;
		
				range_0_1 = (fabs(config->significantVolume) - fabs(Signal->Blocks[b].freq[i].amplitude))/fabs(config->significantVolume);
				intensity = CalculateWeightedError(range_0_1, config);
				color = intensity*0xffff;
		
				SetPenColorStr(GetBlockColor(config, b), color, &plot);
				pl_fcont_r(plot.plotter, Signal->Blocks[b].freq[i].hertz, 
						Signal->Blocks[b].freq[i].amplitude);
			}
			pl_endpath_r(plot.plotter);
		}
	}
	ClosePlot(&plot);
}
*/

void PlotWindow(windowManager *wm, parameters *config)
{
	PlotFile plot;
	char	 name[2048];
	float 	 *window = NULL;
	long int size;

	if(!config || !wm || !wm->windowArray)
		return;

	window = getWindowByLength(wm, 20);
	if(!window)
		return;

	size = getWindowSizeByLength(wm, 20);

	sprintf(name, "WindowPlot_%s", GetWindow(config->window));
	FillPlot(&plot, name, 512, 544, 0, -0.1, 1, 1.1, 0.001, config);

	if(!CreatePlotFile(&plot))
		return;

	pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
	pl_fline_r(plot.plotter, 0, 1, 1, 1);
	pl_fline_r(plot.plotter, 0, 0, 1, 0);

	pl_pencolor_r (plot.plotter, 0, 0xFFFF, 0);	
	for(int i = 0; i < size; i++)
		pl_fpoint_r(plot.plotter, (double)i/(double)size, window[i]);
	
	ClosePlot(&plot);
}

void PlotBetaFunctions(parameters *config)
{
	char	 name[2048];
	int		 type = 0;

	if(!config)
		return;

	for(type = 0; type <= 5; type ++)
	{
		PlotFile plot;

		config->outputFilterFunction = type;
		sprintf(name, "BetaFunctionPlot_%d", type);
		FillPlot(&plot, name, 512, 544, 0, -0.1, 1, 1.1, 0.01, config);
	
		if(!CreatePlotFile(&plot))
			return;
	
		pl_pencolor_r (plot.plotter, 0, 0x5555, 0);
		pl_fline_r(plot.plotter, 0, 1, 1, 1);
		pl_fline_r(plot.plotter, 0, 0, 1, 0);

		pl_flinewidth_r(plot.plotter, 0.005);
		pl_fline_r(plot.plotter, .5, -0.1, .5, 1.1);
		pl_fline_r(plot.plotter, .25, -0.1, .25, 1.1);
		pl_fline_r(plot.plotter, .75, -0.1, .75, 1.1);

		pl_fline_r(plot.plotter, 0, .5, 1, .5);
		pl_fline_r(plot.plotter, 0, .25, 1, .25);
		pl_fline_r(plot.plotter, 0, .75, 1, .75);
	
		pl_pencolor_r (plot.plotter, 0, 0xFFFF, 0);	
		for(int i = 0; i < 512; i++)
		{
			double x, y;
			long int color;

			x = (double)i/(double)512;
			y = CalculateWeightedError((double)i/(double)512, config);
			
			color = y*0xffff;
	
			SetPenColorStr("aqua", color, &plot);
			//logmsg("x: %g (%g) y: %g (%g) c:%ld\n", x, x*60, y, y*60, color);
			pl_fpoint_r(plot.plotter, x, y);
		}
		
		ClosePlot(&plot);
	}
}

int MatchColor(char *color)
{
	/*
	int i = 0;

	for(i = 0; i < strlen(color); i++)
		color[i] = tolower(color[i]);
	*/

	if(strcmp(color, "red") == 0)
		return(COLOR_RED);
	if(strcmp(color, "green") == 0)
		return(COLOR_GREEN);
	if(strcmp(color, "blue") == 0)
		return(COLOR_BLUE);
	if(strcmp(color, "yellow") == 0)
		return(COLOR_YELLOW);
	if(strcmp(color, "magenta") == 0)
		return(COLOR_MAGENTA);
	if(strcmp(color, "aqua") == 0 || strcmp(color, "aquamarine") == 0)
		return(COLOR_AQUA);
	if(strcmp(color, "orange") == 0)
		return(COLOR_ORANGE);
	if(strcmp(color, "purple") == 0)
		return(COLOR_PURPLE);
	if(strcmp(color, "gray") == 0 || strcmp(color, "white") == 0)
		return(COLOR_GRAY);

	logmsg("Unmatched color %s, using green\n", color);
	return COLOR_GREEN;
}

void SetPenColorStr(char *colorName, long int color, PlotFile *plot)
{
	SetPenColor(MatchColor(colorName), color, plot);
}

void SetPenColor(int colorIndex, long int color, PlotFile *plot)
{
	switch(colorIndex)
	{
		case COLOR_RED:
			pl_pencolor_r(plot->plotter, color, 0, 0);
			break;
		case COLOR_GREEN:
			pl_pencolor_r(plot->plotter, 0, color, 0);
			break;
		case COLOR_BLUE:
			pl_pencolor_r(plot->plotter, 0, 0, color);
			break;
		case COLOR_YELLOW:
			pl_pencolor_r(plot->plotter, color, color, 0);
			break;
		case COLOR_AQUA:
			pl_pencolor_r(plot->plotter, 0, color, color);
			break;
		case COLOR_MAGENTA:
			pl_pencolor_r(plot->plotter, color, 0, color);
			break;
		case COLOR_PURPLE:
			pl_pencolor_r(plot->plotter, color/2, 0, color);
			break;
		case COLOR_ORANGE:
			pl_pencolor_r(plot->plotter, color, color/2, 0);
			break;
		case COLOR_GRAY:
			pl_pencolor_r(plot->plotter, color, color, color);
			break;
		default:
			pl_pencolor_r(plot->plotter, 0, color, 0);
			break;
	}
}

void SortFlatAmplitudeDifferencesByRefAmplitude(FlatAmplDifference *ADiff, long int size)
{
	long int 			i = 0, j = 0;
	FlatAmplDifference 	tmp;
	
	for (i = 1; i < size; i++)
	{
		tmp = ADiff[i];
		j = i - 1;
		while(j >= 0 && tmp.refAmplitude < ADiff[j].refAmplitude)
		{
			ADiff[j+1] = ADiff[j];
			j--;
		}
		ADiff[j+1] = tmp;
	}
}

FlatAmplDifference *CreateFlatDifferences(parameters *config)
{
	long int	count = 0;
	FlatAmplDifference *ADiff = NULL;

	ADiff = (FlatAmplDifference*)malloc(sizeof(FlatAmplDifference)*config->Differences.cntAmplAudioDiff);
	if(!ADiff)
		return NULL;

	for(int b = 0; b < config->types.totalChunks; b++)
	{
		int type = 0, color = 0;
		
		type = GetBlockType(config, b);
		color = MatchColor(GetBlockColor(config, b));
		for(int a = 0; a < config->Differences.BlockDiffArray[b].cntAmplBlkDiff; a++)
		{
			ADiff[count].hertz = config->Differences.BlockDiffArray[b].amplDiffArray[a].hertz;
			ADiff[count].refAmplitude = config->Differences.BlockDiffArray[b].amplDiffArray[a].refAmplitude;
			ADiff[count].diffAmplitude = config->Differences.BlockDiffArray[b].amplDiffArray[a].diffAmplitude;
			ADiff[count].type = type;
			ADiff[count].color = color;
			count ++;
		}
	}
	SortFlatAmplitudeDifferencesByRefAmplitude(ADiff, config->Differences.cntAmplAudioDiff);
	/*
	for(int a = 0; a < config->Differences.cntAmplAudioDiff; a++)
	{
		logmsg("Frequency: %7g Hz\tAmplitude: %4.2f dbs\tVolume Difference: %4.2f dbs (%d)\n",
			ADiff[a].hertz,
			ADiff[a].refAmplitude,
			ADiff[a].diffAmplitude,
			ADiff[a].type);
	}
	*/
	return(ADiff);
}

void SortFlatMissingDifferencesByAmplitude(FlatFreqDifference *FDiff, long int size)
{
	long int 			i = 0, j = 0;
	FlatFreqDifference 	tmp;
	
	for (i = 1; i < size; i++)
	{
		tmp = FDiff[i];
		j = i - 1;
		while(j >= 0 && tmp.amplitude < FDiff[j].amplitude)
		{
			FDiff[j+1] = FDiff[j];
			j--;
		}
		FDiff[j+1] = tmp;
	}
}

FlatFreqDifference *CreateFlatMissing(parameters *config)
{
	long int	count = 0;
	FlatFreqDifference *FDiff = NULL;

	FDiff = (FlatFreqDifference*)malloc(sizeof(FlatFreqDifference)*config->Differences.cntFreqAudioDiff);
	if(!FDiff)
		return NULL;

	for(int b = 0; b < config->types.totalChunks; b++)
	{
		int type = 0, color = 0;
		
		type = GetBlockType(config, b);
		color = MatchColor(GetBlockColor(config, b));
		for(int f = 0; f < config->Differences.BlockDiffArray[b].cntFreqBlkDiff; f++)
		{
			FDiff[count].hertz = config->Differences.BlockDiffArray[b].freqMissArray[f].hertz;
			FDiff[count].amplitude = config->Differences.BlockDiffArray[b].freqMissArray[f].amplitude;
			FDiff[count].type = type;
			FDiff[count].color = color;
			count ++;
		}
	}
	SortFlatMissingDifferencesByAmplitude(FDiff, config->Differences.cntFreqAudioDiff);
	return(FDiff);
}