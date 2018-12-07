import ctypes
import numpy as np

incadll = ctypes.CDLL('persist.dll')             #NOTE: Change this to whatever specific model dll you need

incadll.SetupModel.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
incadll.SetupModel.restype  = ctypes.c_void_p

incadll.RunModel.argtypes = [ctypes.c_void_p]

incadll.GetTimesteps.argtypes = [ctypes.c_void_p]
incadll.GetTimesteps.restype = ctypes.c_ulonglong

incadll.GetResultSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.POINTER(ctypes.c_double)]

incadll.GetInputSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.POINTER(ctypes.c_double), ctypes.c_bool]

incadll.SetParameterDouble.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_double]

def SetupModel(parameterfilename, inputfilename):
	return incadll.SetupModel(parameterfilename.encode('ascii'), inputfilename.encode('ascii'))           #TODO: We should figure out what encoding is best to use here.

def RunModel(dataset):
	incadll.RunModel(dataset)
	
def GetTimesteps(dataset):
	return incadll.GetTimesteps(dataset)
	
def _PackIndexes(indexes):
	cindexes = [index.encode('ascii') for index in indexes]
	return (ctypes.c_char_p * len(cindexes))(*cindexes)
	
def GetResultSeries(dataset, name, indexes):	
	timesteps = incadll.GetTimesteps(dataset)
	
	resultseries = (ctypes.c_double * timesteps)()
	
	incadll.GetResultSeries(dataset, name.encode('ascii'), _PackIndexes(indexes), len(indexes), resultseries)
	
	return np.array(resultseries, copy=False)
	
def GetInputSeries(dataset, name, indexes, alignwithresults=False):
	'''
	alignwithresults=False : Extract the entire input series that was provided in the input file
	alignwithresults=True  : Extract the series from the parameter 'Start date', with 'Timesteps' number of values (i.e. aligned with any result series).
	'''
	timesteps = incadll.GetTimesteps(dataset)
	
	inputseries = (ctypes.c_double * timesteps)()
	
	incadll.GetInputSeries(dataset, name.encode('ascii'), _PackIndexes(indexes), len(indexes), inputseries, alignwithresults)
	
	return np.array(inputseries, copy=False)
	
def SetParameterDouble(dataset, name, indexes, value):
	incadll.SetParameterDouble(dataset, name.encode('ascii'), _PackIndexes(indexes), len(indexes), ctypes.c_double(value))
	
	