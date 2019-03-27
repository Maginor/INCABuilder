# Python wrapper

The python wrapper allows you to easily create optimization and calibration routines for a model and do post-processing on the results.

This has currently only been tested with the 64bit version of python 3.

To learn the basics of how to build and compile models, see the quick start guide at the front page, and the tutorials.

The C++-end code of the C++-python interface is in python_wrapper.h, while the python-end of it is in inca.py.

To use a model with the python wrapper, create your own .cpp file along the lines of persistwrapper.cpp, and replace the model building part of it with the modules you want. Then make a .bat file along the lines of compilepersist.bat that compiles your .cpp file. The C++ code compiles to a .dll (dynamically linked library) that can be loaded using inca.initialize('name_of_dll.dll')

We have already built some examples atop of inca.py to show you how you can interact with the models through the interface. See e.g. optimization_example.py or some of the SimplyP examples. There is also docstring documentation in inca.py . We have also made some calibration utilities available through inca_calibration.py .


![Alt text](../Documentation/img/triangle_plot.png?raw=true "Triangle plot from running emcee on reach flow in SimplyP")
