#
# SCons check for maintainer mode
#
# Version 1.3
# 13-Sep-2009
#

from SCons.Script import ARGUMENTS

def checkForMaintainerMode(conf,force=False):
    """Check if user wants to enable maintainer compilation mode.
       
       If force is true than maintainer mode is forced"""
    conf.Message("Checking whether to enable maintainer mode... ")
    maint=ARGUMENTS.get('maintainer-mode', 0) or \
          ARGUMENTS.get('enable-maintainer-mode', 0)
    try:
         maint2=int(maint)
    except ValueError:
         maint2=None
    if force: maint2 = 1
    if maint2 > 0 or str(maint).lower() == 'yes':
			  conf.Result(1)
			  conf.env.Append(CCFLAGS = '-O0')
			  conf.env.Append(CPPFLAGS = '-DMAINTAINER_MODE')
                          return True
    else:
			  conf.Result(0)
                          return False
