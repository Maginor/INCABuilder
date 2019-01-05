import ctypes
import numpy as np

incadll = ctypes.CDLL('persist.dll')             #NOTE: Change this to whatever specific model dll you need

incadll.DllSetupModel.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
incadll.DllSetupModel.restype  = ctypes.c_void_p

incadll.DllRunModel.argtypes = [ctypes.c_void_p]

incadll.DllCopyDataSet.argtypes = [ctypes.c_void_p]
incadll.DllCopyDataSet.restype  = ctypes.c_void_p

incadll.DllDeleteDataSet.argtypes = [ctypes.c_void_p]

incadll.DllGetTimesteps.argtypes = [ctypes.c_void_p]
incadll.DllGetTimesteps.restype = ctypes.c_ulonglong

incadll.DllGetInputTimesteps.argtypes = [ctypes.c_void_p]
incadll.DllGetInputTimesteps.restype = ctypes.c_ulonglong

incadll.DllGetInputStartDate.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

incadll.DllGetResultSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.POINTER(ctypes.c_double)]

incadll.DllGetInputSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.POINTER(ctypes.c_double), ctypes.c_bool]

incadll.DllSetParameterDouble.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_double]

incadll.DllSetParameterUInt.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_ulonglong]

incadll.DllSetParameterBool.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_bool]

incadll.DllSetParameterTime.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_char_p]

incadll.DllWriteParametersToFile.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

incadll.DllGetParameterDouble.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong]
incadll.DllGetParameterDouble.restype = ctypes.c_double

incadll.DllGetParameterUInt.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong]
incadll.DllGetParameterUInt.restype  = ctypes.c_ulonglong

incadll.DllGetParameterBool.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong]
incadll.DllGetParameterBool.restype  = ctypes.c_bool

incadll.DllGetParameterTime.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.c_char_p]

incadll.DllSetInputSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulonglong, ctypes.POINTER(ctypes.c_double), ctypes.c_ulonglong]


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
		datasetptr = incadll.DllSetupModel(_CStr(parameterfilename), _CStr(inputfilename))
		return cls(datasetptr)
		
	def run_model(self) :
		incadll.DllRunModel(self.datasetptr)
	
	def copy(self) :
		return DataSet(incadll.DllCopyDataSet(self.datasetptr))
		
	def delete(self) :
		incadll.DllDeleteDataSet(self.datasetptr)
		
	def write_parameters_to_file(self, filename) :
		incadll.DllWriteParametersToFile(self.datasetptr, _CStr(filename))
		
	def get_result_series(self, name, indexes) :
		timesteps = incadll.DllGetTimesteps(self.datasetptr)
	
		resultseries = (ctypes.c_double * timesteps)()
	
		incadll.DllGetResultSeries(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), resultseries)
	
		return np.array(resultseries, copy=False)
		
	def get_input_series(self, name, indexes, alignwithresults=False) :
		'''
		alignwithresults=False : Extract the entire input series that was provided in the input file
		alignwithresults=True  : Extract the series from the parameter 'Start date', with 'Timesteps' number of values (i.e. aligned with any result series).
		'''
		if alignwithresults :
			timesteps = incadll.DllGetTimesteps(self.datasetptr)
		else :
			timesteps = incadll.DllGetInputTimesteps(self.datasetptr)
		
		inputseries = (ctypes.c_double * timesteps)()
		
		incadll.DllGetInputSeries(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), inputseries, alignwithresults)
		
		return np.array(inputseries, copy=False)
		
	def set_input_series(self, name, indexes, inputseries) :
		array = (ctypes.c_double * len(inputseries))(*inputseries)
		
		incadll.DllSetInputSeries(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), array, len(inputseries))
	
	def set_parameter_double(self, name, indexes, value):
		incadll.DllSetParameterDouble(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), ctypes.c_double(value))
	
	def set_parameter_uint(self, name, indexes, value):
		incadll.DllSetParameterUInt(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), ctypes.c_ulonglong(value))
		
	def set_parameter_bool(self, name, indexes, value):
		incadll.DllSetParameterBool(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), ctypes.c_bool(value))
		
	def set_parameter_time(self, name, indexes, value):
		incadll.DllSetParameterTime(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), value.encode('ascii'))
		
	def get_parameter_double(self, name, indexes) :
		return incadll.DllGetParameterDouble(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		
	def get_parameter_uint(self, name, indexes) :
		return incadll.DllGetParameterUInt(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		
	def get_parameter_bool(self, name, indexes) :
		return incadll.DllGetParameterBool(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		
	def get_parameter_time(self, name, indexes) :
		# NOTE: We allocate the string here instead of in the C++ code so that the python garbage collector can delete it if it goes out of use.
		# ALTERNATIVELY, we could just return a datetime value from this function so that the user does not have to work with the string format.
		string = ctypes.create_string_buffer(32)
		incadll.DllGetParameterTime(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), string)
		return string.value.decode('ascii')

	def get_input_timesteps(self):
		incadll.DllGetInputTimesteps(self.datasetptr)
		
	def get_input_start_date(self):
		string = ctypes.create_string_buffer(32)
		incadll.DllGetInputStartDate(self.datasetptr, string)
		return string.value.decode('ascii')