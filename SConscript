Import(Split("env PREFIX VERSION PACKAGE SHARED"))

#Build static library
libfsp=env.StaticLibrary(target = 'fsplib', source = Split('fsplib.c lock.c'))
env.Alias("build", libfsp)

#Build shared library
if SHARED:
    env.Append(SHLIBSUFFIX = '.0.0.0')
    env.Append(SHLINKFLAGS = '-Wl,-soname -Wl,libfsplib.so.0')
    libfspshared=env.SharedLibrary(target = 'fsplib', source = Split('fsplib.c lock.c'))
    env.Alias("build", libfspshared)

#Install library and header
env.Install(dir = PREFIX+'/lib', source = libfsp)
env.Install(dir = PREFIX+'/include', source='fsplib.h')
if SHARED:
    env.Install(dir = PREFIX+'/lib', source = libfspshared)

#Build test program
test=env.Program(target = 'test', source = ['test.c', 'comm.c', libfsp])
env.Alias("build",test)
if SHARED:
    testshared=env.Program(target = 'test-shared', source = ['test.c', libfspshared])
    env.Alias("build",testshared)

# *************** Targets ****************

#Add install target
env.Alias("install",[ PREFIX+'/lib', PREFIX+'/include'])

#Change default target to build
env.Default(None)
env.Default("build")

#Add dist target
TARBALL=PACKAGE+'-'+VERSION+'.tar.gz'
env.Replace(TARFLAGS = '-c -z')
env.Tar(TARBALL,Split("test.c fsplib.h lock.h lock.c fsplib.c"))
env.Tar(TARBALL,Split("TODO NEWS README AUTHORS ChangeLog COPYING"))
env.Tar(TARBALL,Split("SConstruct SConscript"))
env.Alias("dist",TARBALL)
#Clean tarball when doing build clean
env.Clean("build",TARBALL)
