import ctypes
import numpy as np

incadll = None

def initialize(dllname) :
	global incadll
	incadll = ctypes.CDLL(dllname)             #NOTE: Change this to whatever specific model dll you need

	
	incadll.DllEncounteredError.argtypes = [ctypes.c_char_p]
	incadll.DllEncounteredError.restype = ctypes.c_int
	
	incadll.DllSetupModel.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
	incadll.DllSetupModel.restype  = ctypes.c_void_p

	incadll.DllRunModel.argtypes = [ctypes.c_void_p]

	incadll.DllCopyDataSet.argtypes = [ctypes.c_void_p]
	incadll.DllCopyDataSet.restype  = ctypes.c_void_p

	incadll.DllDeleteDataSet.argtypes = [ctypes.c_void_p]

	incadll.DllGetTimesteps.argtypes = [ctypes.c_void_p]
	incadll.DllGetTimesteps.restype = ctypes.c_uint64

	incadll.DllGetInputTimesteps.argtypes = [ctypes.c_void_p]
	incadll.DllGetInputTimesteps.restype = ctypes.c_uint64

	incadll.DllGetInputStartDate.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

	incadll.DllGetResultSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64, ctypes.POINTER(ctypes.c_double)]

	incadll.DllGetInputSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64, ctypes.POINTER(ctypes.c_double), ctypes.c_bool]

	incadll.DllSetParameterDouble.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64, ctypes.c_double]

	incadll.DllSetParameterUInt.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64, ctypes.c_uint64]

	incadll.DllSetParameterBool.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64, ctypes.c_bool]

	incadll.DllSetParameterTime.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64, ctypes.c_char_p]

	incadll.DllWriteParametersToFile.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

	incadll.DllGetParameterDouble.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64]
	incadll.DllGetParameterDouble.restype = ctypes.c_double

	incadll.DllGetParameterUInt.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64]
	incadll.DllGetParameterUInt.restype  = ctypes.c_uint64

	incadll.DllGetParameterBool.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64]
	incadll.DllGetParameterBool.restype  = ctypes.c_bool

	incadll.DllGetParameterTime.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64, ctypes.c_char_p]

	incadll.DllSetInputSeries.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64, ctypes.POINTER(ctypes.c_double), ctypes.c_uint64, ctypes.c_bool]
	
	incadll.DllGetIndexSetsCount.argtypes = [ctypes.c_void_p]
	incadll.DllGetIndexSetsCount.restype = ctypes.c_uint64
	
	incadll.DllGetIndexSets.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char_p)]

	incadll.DllGetIndexCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetIndexCount.restype = ctypes.c_uint64
	
	incadll.DllGetIndexes.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllGetParameterIndexSetsCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetParameterIndexSetsCount.restype = ctypes.c_uint64
	
	incadll.DllGetParameterIndexSets.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllGetResultIndexSetsCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetResultIndexSetsCount.restype  = ctypes.c_uint64
	
	incadll.DllGetResultIndexSets.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllGetInputIndexSetsCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetInputIndexSetsCount.restype  = ctypes.c_uint64
	
	incadll.DllGetInputIndexSets.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllGetParameterDoubleMinMax.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double)]
	
	incadll.DllGetParameterUIntMinMax.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(ctypes.c_uint64)]
	
	incadll.DllGetParameterDescription.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetParameterDescription.restype = ctypes.c_char_p
	
	incadll.DllGetParameterUnit.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetParameterUnit.restype = ctypes.c_char_p
	
	incadll.DllGetResultUnit.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetResultUnit.restype = ctypes.c_char_p
	
	incadll.DllGetAllParametersCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
	incadll.DllGetAllParametersCount.restype = ctypes.c_uint64
	
	incadll.DllGetAllParameters.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_char_p), ctypes.c_char_p]
	
	incadll.DllGetAllResultsCount.argtypes = [ctypes.c_void_p]
	incadll.DllGetAllResultsCount.restype = ctypes.c_uint64
	
	incadll.DllGetAllResults.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllGetAllInputsCount.argtypes = [ctypes.c_void_p]
	incadll.DllGetAllInputsCount.restype = ctypes.c_uint64
	
	incadll.DllGetAllInputs.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_char_p)]
	
	incadll.DllInputWasProvided.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_uint64]
	incadll.DllInputWasProvided.restype = ctypes.c_bool
	
	incadll.DllGetBranchInputsCount.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]
	incadll.DllGetBranchInputsCount.restype = ctypes.c_uint64
	
	incadll.DllGetBranchInputs.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)]

def _CStr(string):
	return string.encode('utf-8')   #TODO: We should figure out what encoding is best to use here.

def _PackIndexes(indexes):
	cindexes = [index.encode('utf-8') for index in indexes]
	return (ctypes.c_char_p * len(cindexes))(*cindexes)
	
def check_dll_error() :
	errmsgbuf = ctypes.create_string_buffer(1024)
	errcode = incadll.DllEncounteredError(errmsgbuf)
	if errcode == 1 :
		errmsg = errmsgbuf.value.decode('utf-8')
		raise RuntimeError(errmsg)
	
class DataSet :
	def __init__(self, datasetptr) :
		self.datasetptr = datasetptr
	
	@classmethod
	def setup_from_parameter_and_input_files(cls, parameterfilename, inputfilename) :
		datasetptr = incadll.DllSetupModel(_CStr(parameterfilename), _CStr(inputfilename))
		check_dll_error()
		return cls(datasetptr)
		
	def run_model(self) :
		'''
		Runs the model with the parameters and input series that are currently stored in the dataset. All result series will also be stored in the dataset.
		'''
		incadll.DllRunModel(self.datasetptr)
		check_dll_error()
	
	def copy(self) :
		'''
		Create a copy of the dataset that contains all the same parameter values and input series. Result series will not be copied.
		'''
		cp = DataSet(incadll.DllCopyDataSet(self.datasetptr))
		check_dll_error()
		return cp
		
	def delete(self) :
		'''
		Delete all data that was allocated by the C++ code for this dataset. Interaction with the dataset after it was deleted is not recommended. Note that this will not delete the model itself, only the parameter, input and result data. This is because typically you can have multiple datasets sharing the same model (such as if you created dataset copies using dataste.copy()). There is currently no way to delete the model.
		'''
		incadll.DllDeleteDataSet(self.datasetptr)
		check_dll_error()
		
	def write_parameters_to_file(self, filename) :
		'''
		Write the parameters in the dataset to a standard INCABuilder parameter file.
		'''
		incadll.DllWriteParametersToFile(self.datasetptr, _CStr(filename))
		check_dll_error()
		
	def get_result_series(self, name, indexes) :
		'''
		Extract one of the result series that was produced by the model. Can only be called after dataset.run_model() has been called at least once.
		
		Keyword arguments:
			name             -- string. The name of the result series. Example : "Soil moisture"
			indexes          -- list of strings. A list of index names to identify the particular input series. Example : ["Langtjern"] or ["Langtjern", "Forest"]
		
		Returns:
			A numpy.array containing the specified timeseries.
		'''
		timesteps = incadll.DllGetTimesteps(self.datasetptr)
		check_dll_error()
	
		resultseries = (ctypes.c_double * timesteps)()
	
		incadll.DllGetResultSeries(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), resultseries)
		check_dll_error()
	
		return np.array(resultseries, copy=False)
		
	def get_input_series(self, name, indexes, alignwithresults=False) :
		'''
		Extract one of the input series that were provided with the dataset.
		
		Keyword arguments:
			name             -- string. The name of the input series. Example : "Air temperature"
			indexes          -- list of strings. A list of index names to identify the particular input series. Example : ["Langtjern"] or ["Langtjern", "Forest"]
			alignwithresults -- boolean. If False: Extract the entire input series that was provided in the input file. If True: Extract the series from the parameter 'Start date', with 'Timesteps' number of values (i.e. aligned with any result series of the dataset).
		
		Returns:
			A numpy.array containing the specified timeseries.
		'''
		if alignwithresults :
			timesteps = incadll.DllGetTimesteps(self.datasetptr)
		else :
			timesteps = incadll.DllGetInputTimesteps(self.datasetptr)
		check_dll_error()
		
		inputseries = (ctypes.c_double * timesteps)()
		
		incadll.DllGetInputSeries(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), inputseries, alignwithresults)
		check_dll_error()
		
		return np.array(inputseries, copy=False)
		
	def set_input_series(self, name, indexes, inputseries, alignwithresults=False) :
		'''
		Overwrite one of the input series in the dataset.
		
		Keyword arguments:
			name             -- string. The name of the input series. Example : "Air temperature"
			indexes          -- list of strings. A list of index names to identify the particular input series. Example : ["Langtjern"] or ["Langtjern", "Forest"]
			inputseries      -- list of double. The values to set.
			alignwithresults -- boolean. If False: Start writing to the first timestep of the input series. If True: Start writing at the timestep corresponding to the parameter 'Start date'.
		'''
		array = (ctypes.c_double * len(inputseries))(*inputseries)
		
		incadll.DllSetInputSeries(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), array, len(inputseries), alignwithresults)
		check_dll_error()
	
	def set_parameter_double(self, name, indexes, value):
		'''
		Overwrite the value of one parameter. Can only be called on parameters that were registered with the type double.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Maximum capacity"
			indexes          -- list of strings. A list of index names to identify the particular parameter instance. Example : [], ["Langtjern"] or ["Langtjern", "Forest"]
			value            -- float. The value to write to the parameter.
		'''
		incadll.DllSetParameterDouble(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), ctypes.c_double(value))
		check_dll_error()
	
	def set_parameter_uint(self, name, indexes, value):
		'''
		Overwrite the value of one parameter. Can only be called on parameters that were registered with the type uint.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Fertilizer addition start day"
			indexes          -- list of strings. A list of index names to identify the particular parameter instance. Example : [], ["Langtjern"] or ["Langtjern", "Forest"]
			value            -- unsigned integer. The value to write to the parameter.
		'''
		incadll.DllSetParameterUInt(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), ctypes.c_uint64(value))
		check_dll_error()
		
	def set_parameter_bool(self, name, indexes, value):
		'''
		Overwrite the value of one parameter. Can only be called on parameters that were registered with the type bool.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Reach has effluent inputs"
			indexes          -- list of strings. A list of index names to identify the particular parameter instance. Example : [], ["Langtjern"] or ["Langtjern", "Forest"]
			value            -- bool. The value to write to the parameter.
		'''
		incadll.DllSetParameterBool(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), ctypes.c_bool(value))
		check_dll_error()
		
	def set_parameter_time(self, name, indexes, value):
		'''
		Overwrite the value of one parameter. Can only be called on parameters that were registered with the type time.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Start date"
			indexes          -- list of strings. A list of index names to identify the particular parameter instance. Example : [], ["Langtjern"] or ["Langtjern", "Forest"]
			value            -- string. The value to write to the parameter. Must be on the format "YYYY-MM-dd", e.g. "1999-05-15"
		'''
		
		#TODO: Maybe we just want to take a datetime value as value instead since that is what most people are going to use?
		
		incadll.DllSetParameterTime(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), value.encode('utf-8'))
		check_dll_error()
		
	def get_parameter_double(self, name, indexes) :
		'''
		Read the value of one parameter. Can only be called on parameters that were registered with the type double.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Maximum capacity"
			indexes          -- list of strings. A list of index names to identify the particular parameter instance. Example : [], ["Langtjern"] or ["Langtjern", "Forest"]
		
		Returns:
			The value of the parameter (a floating point number).
		'''
		val = incadll.DllGetParameterDouble(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		check_dll_error()
		return val
		
	def get_parameter_uint(self, name, indexes) :
		'''
		Read the value of one parameter. Can only be called on parameters that were registered with the type uint.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Fertilizer addition start day"
			indexes          -- list of strings. A list of index names to identify the particular parameter instance. Example : [], ["Langtjern"] or ["Langtjern", "Forest"]
		
		Returns:
			The value of the parameter (a nonzero integer).
		'''
		val = incadll.DllGetParameterUInt(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		check_dll_error()
		return val
		
	def get_parameter_bool(self, name, indexes) :
		'''
		Read the value of one parameter. Can only be called on parameters that were registered with the type bool.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Reach has effluent inputs"
			indexes          -- list of strings. A list of index names to identify the particular parameter instance. Example : [], ["Langtjern"] or ["Langtjern", "Forest"]
		
		Returns:
			The value of the parameter (a boolean).
		'''
		return incadll.DllGetParameterBool(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		
	def get_parameter_time(self, name, indexes) :
		'''
		Read the value of one parameter. Can only be called on parameters that were registered with the type time.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Start date"
			indexes          -- list of strings. A list of index names to identify the particular parameter instance. Example : [], ["Langtjern"] or ["Langtjern", "Forest"]
		
		Returns:
			The value of the parameter (a string with the format "YYYY-MM-dd").
		'''
		
		#TODO: Maybe we just want to return a datetime value from this instead since that is what most people are going to use?
		
		string = ctypes.create_string_buffer(32)
		incadll.DllGetParameterTime(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes), string)
		check_dll_error()
		return string.value.decode('utf-8')
		
	def get_parameter_double_min_max(self, name) :
		'''
		Retrieve the recommended min and max values for a parameter. Can only be called on a parameter that was registered with type double.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Baseflow index"
			
		Returns:
			a tuple (min, max) where min and max are of type float
		'''
		min = ctypes.c_double(0)
		max = ctypes.c_double(0)
		incadll.DllGetParameterDoubleMinMax(self.datasetptr, _CStr(name), ctypes.POINTER(ctypes.c_double)(min), ctypes.POINTER(ctypes.c_double)(max))
		check_dll_error()
		return (min.value, max.value)

	def get_parameter_uint_min_max(self, name):
		'''
		Retrieve the recommended min and max values for a parameter. Can only be called on a parameter that was registered with type uint.
		
		Keyword arguments:
			name             -- string. The name of the parameter. Example : "Fertilizer addition period"
			
		Returns:
			a tuple (min, max) where min and max are of type nonzero integer
		'''
		min = ctypes.c_uint64(0)
		max = ctypes.c_uint64(0)
		incadll.DllGetParameterUIntMinMax(self.datasetptr, _CStr(name), ctypes.POINTER(ctypes.c_uint64)(min), ctypes.POINTER(ctypes.c_uint64)(max))
		check_dll_error()
		return (min.value, max.value)
		
	def get_parameter_description(self, name):
		'''
		Retrieve (as a string) a longer form description of the parameter with the given name.
		'''
		desc = incadll.DllGetParameterDescription(self.datasetptr, _CStr(name))
		check_dll_error()
		return desc.decode('utf-8')
		
	def get_parameter_unit(self, name):
		'''
		Retrieve (as a string) the unit of the parameter with the given name.
		'''
		unit = incadll.DllGetParameterUnit(self.datasetptr, _CStr(name))
		check_dll_error()
		return unit.decode('utf-8')
		
	def get_result_unit(self, name) :
		'''
		Retrieve (as a string) the unit of the result series with the given name.
		'''
		unit = incadll.DllGetResultUnit(self.datasetptr, _CStr(name))
		check_dll_error()
		return unit.decode('utf-8')
		
	def get_input_timesteps(self):
		'''
		Get the number of timesteps that was allocated for the input data.
		'''
		incadll.DllGetInputTimesteps(self.datasetptr)
		check_dll_error()
		
	def get_input_start_date(self):
		'''
		Get the start date that was set for the input data. Returns a string of the form "YYYY-MM-dd".
		'''
		string = ctypes.create_string_buffer(32)
		incadll.DllGetInputStartDate(self.datasetptr, string)
		check_dll_error()
		return string.value.decode('utf-8')
		
	def get_index_sets(self):
		'''
		Get the name of each index set that is present in the model. Returns a list of strings.
		'''
		num = incadll.DllGetIndexSetsCount(self.datasetptr)
		check_dll_error()
		array = (ctypes.c_char_p * num)()
		incadll.DllGetIndexSets(self.datasetptr, array)
		check_dll_error()
		return [string.decode('utf-8') for string in array]
		
	def get_indexes(self, index_set):
		'''
		Get the name of each index in an index set.
		
		Keyword arguments:
			index_set          -- string. The name of the index set
		
		Returns:
			A list of strings where each string is an index in the index set.
		'''
		num = incadll.DllGetIndexCount(self.datasetptr, _CStr(index_set))
		check_dll_error()
		array = (ctypes.c_char_p * num)()
		incadll.DllGetIndexes(self.datasetptr, _CStr(index_set), array)
		check_dll_error()
		return [string.decode('utf-8') for string in array]
		
	def get_parameter_index_sets(self, name):
		'''
		Get the name of each index set a parameter indexes over.
		
		Keyword arguments:
			name            -- string. The name of the parameter
		
		Returns:
			A list of strings where each string is the name of an index set.
		'''
		num = incadll.DllGetParameterIndexSetsCount(self.datasetptr, _CStr(name))
		check_dll_error()
		array = (ctypes.c_char_p * num)()
		incadll.DllGetParameterIndexSets(self.datasetptr, _CStr(name), array)
		check_dll_error()
		return [string.decode('utf-8') for string in array]
		
	def get_result_index_sets(self, name):
		'''
		Get the name of each index set a result indexes over.
		
		Keyword arguments:
			name            -- string. The name of the result series
		
		Returns:
			A list of strings where each string is the name of an index set.
		'''
		num = incadll.DllGetResultIndexSetsCount(self.datasetptr, _CStr(name))
		check_dll_error()
		array = (ctypes.c_char_p * num)()
		incadll.DllGetResultIndexSets(self.datasetptr, _CStr(name), array)
		check_dll_error()
		return [string.decode('utf-8') for string in array]
		
	def get_input_index_sets(self, name):
		'''
		Get the name of each index set an input series indexes over.
		
		Keyword arguments:
			name            -- string. The name of the input series
		
		Returns:
			A list of strings where each string is the name of an index set.
		'''
		num = incadll.DllGetInputIndexSetsCount(self.datasetptr, _CStr(name))
		check_dll_error()
		array = (ctypes.c_char_p * num)()
		incadll.DllGetResultInputSets(self.datasetptr, _CStr(name), array)
		check_dll_error()
		return [string.decode('utf-8') for string in array]
		
	def get_parameter_list(self, groupname = '') :
		'''
		Get the name and type of all the parameters in the model as a list of pairs of strings.
		
		Keyword arguments:
			groupname       -- string. (optional) Only list the parameters belonging to this parameter group.
		'''
		num = incadll.DllGetAllParametersCount(self.datasetptr, _CStr(groupname))
		check_dll_error()
		namearray = (ctypes.c_char_p * num)()
		typearray = (ctypes.c_char_p * num)()
		incadll.DllGetAllParameters(self.datasetptr, namearray, typearray, _CStr(groupname))
		check_dll_error()
		return [(name.decode('utf-8'), type.decode('utf-8')) for name, type in zip(namearray, typearray)]
		
	def get_equation_list(self) :
		'''
		Get the name and type of all the equations in the model as a list of pairs of strings.
		'''
		num = incadll.DllGetAllResultsCount(self.datasetptr)
		check_dll_error()
		namearray = (ctypes.c_char_p * num)()
		typearray = (ctypes.c_char_p * num)()
		incadll.DllGetAllResults(self.datasetptr, namearray, typearray)
		check_dll_error()
		return [(name.decode('utf-8'), type.decode('utf-8')) for name, type in zip(namearray, typearray)]
		
	def get_input_list(self) :
		'''
		Get the name and type of all the equations in the model as a list of pairs of strings.
		'''
		num = incadll.DllGetAllInputsCount(self.datasetptr)
		check_dll_error()
		namearray = (ctypes.c_char_p * num)()
		typearray = (ctypes.c_char_p * num)()
		incadll.DllGetAllInputs(self.datasetptr, namearray, typearray)
		check_dll_error()
		return [(name.decode('utf-8'), type.decode('utf-8')) for name, type in zip(namearray, typearray)]
		
	def input_was_provided(self, name, indexes):
		'''
		Find out if a particular input timeseries was provided in the input file (or set maually using set_input_series), or if it is missing.
		
		Keyword arguments:
			name             -- string. The name of the input series. Example : "Air temperature"
			indexes          -- list of strings. A list of index names to identify the particular input series. Example : ["Langtjern"] or ["Langtjern", "Forest"]
			
		Returns:
			a boolean
		'''
		value = incadll.DllInputWasProvided(self.datasetptr, _CStr(name), _PackIndexes(indexes), len(indexes))
		check_dll_error()
		return value
		
	def get_branch_inputs(self, indexsetname, indexname):
		'''
		Get the branch inputs of an index in a branched index set.
		
		Keyword arguments:
			indexsetname     -- string. The name of an index set. This index set has to be branched.
			indexname        -- string. The name of an index in this index set.
			
		Returns:
			a list of strings containing the names of the indexes that are branch inputs to the given index.
		'''
		num = incadll.DllGetBranchInputsCount(self.datasetptr, _CStr(indexsetname), _CStr(indexname))
		check_dll_error()
		namearray = (ctypes.c_char_p * num)()
		incadll.DllGetBranchInputs(self.datasetptr, _CStr(indexsetname), _CStr(indexname), namearray)
		check_dll_error()
		return [name.decode('utf-8') for name in namearray]
		