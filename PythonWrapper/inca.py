import ctypes
import numpy as np

incadll = ctypes.CDLL('persist.dll')             #NOTE: Change this to whatever specific model dll you need

incadll.SetupModel.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
incadll.SetupModel.restype  = ctypes.c_void_p

incadll.RunModel.argtypes = [ctypes.c_void_p]

incadll.CopyDataSet.argtypes = [ctypes.c_void_p]
incadll.CopyDataSet.restype  = ctypes.c_void_p

incadll.DeleteDataSet.argtypes = [ctypes.c_void_p]

incadll.GetTimesteps.argtypes = [ctypes.c_void_p]
incadll.GetTimesteps.restype = ctypes.c_ulonglong

incadll.GetInputTimesteps.argtypes = [ctypes.c_void_p]
incadll.GetInputTimesteps.restype = ctypes.c_ulonglong

incadll.GetResultSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.POINTER(ctypes.c_double)]

incadll.GetInputSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.POINTER(ctypes.c_double), ctypes.c_bool]

incadll.SetParameterDouble.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_double]

incadll.SetParameterUInt.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_ulonglong]

incadll.SetParameterBool.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_bool]

incadll.SetParameterTime.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_char_p]

incadll.WriteParametersToFile.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

incadll.GetParameterDouble.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong]
incadll.GetParameterDouble.restype = ctypes.c_double

incadll.GetParameterUInt.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong]
incadll.GetParameterUInt.restype  = ctypes.c_ulonglong

incadll.GetParameterBool.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong]
incadll.GetParameterBool.restype  = ctypes.c_bool

incadll.GetParameterTime.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_char_p]



def _CStr(string):
	return string.encode('ascii')   #TODO: We should figure out what encoding is best to use here.

def _PackIndexes(indexes):
	cindexes = [index.encode('ascii') for index in indexes]
	return (ctypes.c_char_p * len(cindexes))(*cindexes)
	
class DataSet :
	def __init__(self, datasetptr) :
		self.datasetptr = datasetptr
	
	@classmethod
	def setup_from_parameter_and_input_files(cls, parameterfilename, inputfilename) :
		datasetptr = incadll.SetupModel(_CStr(parameterfilename), _CStr(inputfilename))
		return cls(datasetptr)
		
	def run_model(self) :
		incadll.RunModel(self.datasetptr)
	
	def copy(self) :
		return DataSet(incadll.CopyDataSet(self.datasetptr))
		
	def delete(self) :
		incadll.DeleteDataSet(self.datasetptr)
		
	def write_parameters_to_file(self, filename) :
		incadll.WriteParametersToFile(self.datasetptr, _CStr(filename))
		
	def get_result_series(self, name, indexes) :
		timesteps = incadll.GetTimesteps(self.datasetptr)
	
		resultseries = (ctypes.c_double * timesteps)()
	
		incadll.GetResultSeries(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), resultseries)
	
		return np.array(resultseries, copy=False)
		
	def get_input_series(self, name, indexes, alignwithresults=False) :
		'''
		alignwithresults=False : Extract the entire input series that was provided in the input file
		alignwithresults=True  : Extract the series from the parameter 'Start date', with 'Timesteps' number of values (i.e. aligned with any result series).
		'''
		if alignwithresults :
			timesteps = incadll.GetTimesteps(self.datasetptr)
		else :
			timesteps = incadll.GetInputTimesteps(self.datasetptr)
		
		inputseries = (ctypes.c_double * timesteps)()
		
		incadll.GetInputSeries(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), inputseries, alignwithresults)
		
		return np.array(inputseries, copy=False)
		
	def set_parameter_double(self, name, indexes, value):
		incadll.SetParameterDouble(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), ctypes.c_double(value))
	
	def set_parameter_uint(self, name, indexes, value):
		incadll.SetParameterUInt(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), ctypes.c_ulonglong(value))
		
	def set_parameter_bool(self, name, indexes, value):
		incadll.SetParameterBool(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), ctypes.c_bool(value))
		
	def set_parameter_time(self, name, indexes, value):
		incadll.SetParameterTime(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), value.encode('ascii'))
		
	def get_parameter_double(self, name, indexes) :
		return incadll.GetParameterDouble(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		
	def get_parameter_uint(self, name, indexes) :
		return incadll.GetParameterUInt(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		
	def get_parameter_bool(self, name, indexes) :
		return incadll.GetParameterBool(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		
	def get_parameter_time(self, name, indexes) :
		# NOTE: We allocate the string here instead of in the C++ code so that the python garbage collector can delete it if it goes out of use.
		# ALTERNATIVELY, we could just return a datetime value from this function so that the user does not have to work with the string format.
		string = ctypes.create_string_buffer(32)
		incadll.GetParameterTime(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), string)
		return string.value.decode('ascii')

