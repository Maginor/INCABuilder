import ctypes
import numpy as np

incadll = None

def initialize(dllname) :
	global incadll
	incadll = ctypes.CDLL(dllname)             #NOTE: Change this to whatever specific model dll you need

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
	
	incadll.DllGetIndexSetsCount.argtypes = [ctypes.c_void_p]
	incadll.DllGetIndexSetsCount.restype = ctypes.c_ulonglong
	
	incadll.DllGetIndexSets.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char_p)]

	incadll.DllGetIndexCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetIndexCount.restype = ctypes.c_ulonglong
	
	incadll.DllGetIndexes.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllGetParameterIndexSetsCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetParameterIndexSetsCount.restype = ctypes.c_ulonglong
	
	incadll.DllGetParameterIndexSets.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllGetResultIndexSetsCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetResultIndexSetsCount.restype  = ctypes.c_ulonglong
	
	incadll.DllGetResultIndexSets.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllGetInputIndexSetsCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetInputIndexSetsCount.restype  = ctypes.c_ulonglong
	
	incadll.DllGetInputIndexSets.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllGetParameterDoubleMinMax.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double)]
	
	incadll.DllGetParameterUIntMinMax.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_ulonglong), ctypes.POINTER(ctypes.c_ulonglong)]
	
	incadll.DllGetParameterDescription.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetParameterDescription.restype = ctypes.c_char_p
	
	incadll.DllGetParameterUnit.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetParameterUnit.restype = ctypes.c_char_p
	
	incadll.DllGetResultUnit.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetResultUnit.restype = ctypes.c_char_p
	
	incadll.DllGetAllParametersCount.argtypes = [ctypes.c_void_p]
	incadll.DllGetAllParametersCount.restype = ctypes.c_ulonglong
	
	incadll.DllGetAllParameters.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_char_p)]

def _CStr(string):
	return string.encode('utf-8')   #TODO: We should figure out what encoding is best to use here.

def _PackIndexes(indexes):
	cindexes = [index.encode('utf-8') for index in indexes]
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
		incadll.DllSetParameterTime(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), value.encode('utf-8'))
		
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
		return string.value.decode('utf-8')
		
	def get_parameter_double_min_max(self, name) :
		min = ctypes.c_double(0)
		max = ctypes.c_double(0)
		incadll.DllGetParameterDoubleMinMax(self.datasetptr, _CStr(name), ctypes.POINTER(ctypes.c_double)(min), ctypes.POINTER(ctypes.c_double)(max))
		return (min.value, max.value)

	def get_parameter_uint_min_max(self, name):
		min = ctypes.c_ulonglong(0)
		max = ctypes.c_ulonglong(0)
		incadll.DllGetParameterUIntMinMax(self.datasetptr, _CStr(name), ctypes.POINTER(ctypes.c_ulonglong)(min), ctypes.POINTER(ctypes.c_ulonglong)(max))
		return (min.value, max.value)
		
	def get_parameter_description(self, name):
		desc = incadll.DllGetParameterDescription(self.datasetptr, _CStr(name))
		return desc.decode('utf-8')
		
	def get_parameter_unit(self, name):
		unit = incadll.DllGetParameterUnit(self.datasetptr, _CStr(name))
		return unit.decode('utf-8')
		
	def get_result_unit(self, name) :
		unit = incadll.DllGetResultUnit(self.datasetptr, _CStr(name))
		return unit.decode('utf-8')
		
	def get_input_timesteps(self):
		incadll.DllGetInputTimesteps(self.datasetptr)
		
	def get_input_start_date(self):
		string = ctypes.create_string_buffer(32)
		incadll.DllGetInputStartDate(self.datasetptr, string)
		return string.value.decode('utf-8')
		
	def get_index_sets(self):
		num = incadll.DllGetIndexSetsCount(self.datasetptr)
		array = (ctypes.c_char_p * num)()
		incadll.DllGetIndexSets(self.datasetptr, array)
		return [string.decode('utf-8') for string in array]
		
	def get_indexes(self, index_set):
		num = incadll.DllGetIndexCount(self.datasetptr, _CStr(index_set))
		array = (ctypes.c_char_p * num)()
		incadll.DllGetIndexes(self.datasetptr, _CStr(index_set), array)
		return [string.decode('utf-8') for string in array]
		
	def get_parameter_index_sets(self, name):
		num = incadll.DllGetParameterIndexSetsCount(self.datasetptr, _CStr(name))
		array = (ctypes.c_char_p * num)()
		incadll.DllGetParameterIndexSets(self.datasetptr, _CStr(name), array)
		return [string.decode('utf-8') for string in array]
		
	def get_result_index_sets(self, name):
		num = incadll.DllGetResultIndexSetsCount(self.datasetptr, _CStr(name))
		array = (ctypes.c_char_p * num)()
		incadll.DllGetResultIndexSets(self.datasetptr, _CStr(name), array)
		return [string.decode('utf-8') for string in array]
		
	def get_input_index_sets(self, name):
		num = incadll.DllGetInputIndexSetsCount(self.datasetptr, _CStr(name))
		array = (ctypes.c_char_p * num)()
		incadll.DllGetResultInputSets(self.datasetptr, _CStr(name), array)
		return [string.decode('utf-8') for string in array]
		
	def get_parameter_list(self) :
		num = incadll.DllGetAllParametersCount(self.datasetptr)
		namearray = (ctypes.c_char_p * num)()
		typearray = (ctypes.c_char_p * num)()
		incadll.DllGetAllParameters(self.datasetptr, namearray, typearray)
		return [(name.decode('utf-8'), type.decode('utf-8')) for name, type in zip(namearray, typearray)]
		
		
		