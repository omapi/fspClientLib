#
# Scons test for building shared library
#
# Version 1.0
# 9-Sep-2014

from SCons.Script import ARGUMENTS

def checkForBuildingSharedLibrary(conf):
    """check if building shared library was requested"""
    conf.Message("checking whether to build shared library... ")
    makeshared=ARGUMENTS.get('enable-shared', 0)
    if makeshared == 0 or makeshared == 'no':
			  conf.Result(0)
			  return False
    else:
			  conf.Result(1)
			  return True
