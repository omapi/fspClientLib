#
# SCons user-supplied FSP lock prefix tester
#
# Version 1.1
# 24-Aug-2009
#

from SCons.Script import ARGUMENTS

def checkForLockPrefix(conf):
    """Check for user-supplied lock prefix."""
    conf.Message("Checking for user supplied lockprefix... ")
    lp = ARGUMENTS.get('lockprefix', 0) or ARGUMENTS.get("with-lockprefix",0)

    if lp:
	  conf.Result(1)
	  conf.env.Append(CPPFLAGS = '-DFSP_KEY_PREFIX=\\"'+lp+'\\"')
    else:
	  conf.Result(0)
