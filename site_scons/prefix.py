#
# SCons user-supplied prefix tester
#
# Version 1.1
# 24-Aug-2009
#

from SCons.Script import ARGUMENTS

def checkForUserPrefix(conf,oldprefix=None):
    """Returns prefix specified on command line or oldprefix if none is found.""" 
    conf.Message("Checking for user supplied prefix... ")
    lp = ARGUMENTS.get('prefix', 0)
    if lp:
       conf.Result(1)
       return lp
    else:
       conf.Result(0)
       return oldprefix
