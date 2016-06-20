# Process this file with http://www.scons.org to build FSPlib.

import os
# init Scons
EnsureSConsVersion(0,96)
PACKAGE='fsplib'
VERSION='0.12_v3'

#Defaults
PREFIX='/usr/local'
SHARED=0
P2PLAB_PATH='p2pnat/lib/x64'

env = Environment(CPPPATH=['#/include','#/p2pnat/include'],LIBPATH=os.path.join('#/',P2PLAB_PATH),LIBS='p2pnat')
# Turn CPPFLAGS to list
env.Append( CPPFLAGS = [])
# Add by xxfan
env.Append( LINKFLAGS = Split('-z origin') )
env.Append( RPATH = env.Literal(os.path.join('\\$$ORIGIN',P2PLAB_PATH)))
############## Imports #################
from maintainer import checkForMaintainerMode
from compilertest import checkForCCOption
from buildshared import checkForBuildingSharedLibrary
from dirent import checkForDirentMember
from lockprefix import checkForLockPrefix
from locktype import checkForLockingType
from prefix import checkForUserPrefix

################### Functions ######################
def importEnv(list=None, prefix=None):
   if list:
       for i in list:
	   if os.environ.get(i):
	       kw={}
	       kw[i]=os.environ.get(i)
	       kw={ 'ENV': kw }
	       env.Append(**kw)
   if prefix:
       for i in os.environ.keys():
	   if i.startswith(prefix):
	       kw={}
	       kw[i]=os.environ.get(i)
	       kw={ 'ENV': kw }
	       env.Append(**kw)

#import environment
importEnv(['HOME','CC'])
importEnv(prefix='DISTCC_')
importEnv(prefix='CCACHE_')
if env['ENV'].get('CC'):
    env.Replace( CC = env['ENV'].get('CC'))
# Get CC from commandline
if ARGUMENTS.get('CC', 0):
    env.Replace(CC =  ARGUMENTS.get('CC'))

############# Custom configure tests ###################

#Start configuration
conf = Configure(env,{
                       'checkForGCCOption':checkForCCOption,
                       'MAINTAINER_MODE':checkForMaintainerMode,
		       'checkForLockPrefix':checkForLockPrefix,
		       'checkPrefix':checkForUserPrefix,
		       'checkDirentFor':checkForDirentMember,
		       'ENABLE_SHARED':checkForBuildingSharedLibrary,
		       'checkForLockingType':checkForLockingType
		      }
		      )
for option in Split("""
      -Wall -W -Wstrict-prototypes -Wmissing-prototypes -Wshadow
      -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings
      -Waggregate-return -Wmissing-declarations
      -Wmissing-format-attribute -Wnested-externs
      -ggdb -fno-common -Wchar-subscripts -Wcomment
      -Wimplicit -Wsequence-point -Wreturn-type
      -Wfloat-equal -Wno-system-headers -Wredundant-decls
      -Wmissing-noreturn -pedantic
      -Wlong-long -Wundef -Winline
      -Wpointer-arith -Wno-unused-parameter
      -Wno-cast-align
"""):
       conf.checkForGCCOption(option)
#portability checks
if not conf.CheckType("union semun", "#include <sys/types.h>\n#include <sys/ipc.h>\n#include <sys/sem.h>",'c'):
       conf.env.Append(CPPFLAGS = "-D_SEM_SEMUN_UNDEFINED=1")
conf.checkDirentFor('d_namlen','HAVE_DIRENT_NAMLEN')
conf.checkDirentFor('d_fileno','HAVE_DIRENT_FILENO')
conf.checkDirentFor('d_type','HAVE_DIRENT_TYPE')
if conf.CheckCHeader('stdint.h'):
       conf.env.Append(CPPFLAGS = "-DHAVE_STDINT_H")
conf.checkForLockingType(conf)
conf.checkForLockPrefix()
conf.checkPrefix()
SHARED = conf.ENABLE_SHARED()
conf.MAINTAINER_MODE()
conf.Finish()

# process build rules
Export( Split("env PREFIX PACKAGE VERSION SHARED"))
env.SConscript(dirs=['.'])
