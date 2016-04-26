#
# SCons check for dirent member
#
# Version 1.0
# 9-Sep-2014
#

def checkForDirentMember(conf,member,symbol):
    """Checks if dirent structure has specified field member.
 
    Keyword arguments:
    member -- what member we are checking
    symbol -- symbol to be defined by preprocessor if member exists

    """
    conf.Message("checking whether dirent structure has member "+member+"... ")
    rc=conf.TryCompile("""
    #include <sys/types.h>
    #include <dirent.h>

    void dummy(void);
    void dummy()
    {
	struct dirent d;
	d."""+member+"""=0;
    }
    """,".c")
    if rc:
        conf.env.Append(CPPFLAGS = '-D'+symbol)
    conf.Result(rc)
    return rc
