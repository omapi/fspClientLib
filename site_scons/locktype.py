#
# SCons FSP locking type tester
#
# Version 1.2
# 10-Sep-2009
#

from SCons.Script import ARGUMENTS

def checkForLockingType(check,conf):
    """check for available locking methods"""
    fun_lockf=conf.CheckFunc("lockf")
    fun_semop=conf.CheckFunc("semop")
    fun_shmget=conf.CheckFunc("shmget")
    fun_flock=conf.CheckFunc("flock")
    # select locking type
    check.Message("Checking for FSP locking type... ")
    lt=ARGUMENTS.get('locking', 0) or ARGUMENTS.get("with-locking",0) or ARGUMENTS.get("lock",0) or ARGUMENTS.get("with-lock",0)
    if lt == "none":
        conf.env.Append(CPPFLAGS = '-DFSP_NOLOCKING')
        check.Result("none")
    elif lt == "lockf" and fun_lockf:
        conf.env.Append(CPPFLAGS = '-DFSP_USE_LOCKF')
        check.Result("lockf")
    elif lt == "semop" and fun_semop and fun_shmget:
        conf.env.Append(CPPFLAGS = '-DFSP_USE_SHAREMEM_AND_SEMOP')
        check.Result("semop and shmget")
    elif lt == "shmget" and fun_shmget and fun_lockf:
        conf.env.Append(CPPFLAGS = '-DFSP_USE_SHAREMEM_AND_LOCKF')
        check.Result("lockf and shmget")
    elif lt == "flock" and fun_flock:
        conf.env.Append(CPPFLAGS = '-DFSP_USE_FLOCK')
        check.Result("flock")
    # AUTODETECT best available locking type    
    elif fun_semop and fun_shmget:
        conf.env.Append(CPPFLAGS = '-DFSP_USE_SHAREMEM_AND_SEMOP')
        check.Result("semop and shmget")
    elif fun_shmget and fun_lockf:
        conf.env.Append(CPPFLAGS = '-DFSP_USE_SHAREMEM_AND_LOCKF')
        check.Result("lockf and shmget")
    elif fun_lockf:
        conf.env.Append(CPPFLAGS = '-DFSP_USE_LOCKF')
        check.Result("lockf")
    elif fun_flock:
        conf.env.Append(CPPFLAGS = '-DFSP_USE_FLOCK')
        check.Result("flock")
    else:
        conf.env.Append(CPPFLAGS = '-DFSP_NOLOCKING')
        check.Result("none")
