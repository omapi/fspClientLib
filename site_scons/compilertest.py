#
# Scons compiler tester
#
# Version 1.2
# 08-Sep-2014
#

def checkForCCOption(conf,option):
   """Checks if CC compiler supports given command line option.

   Adds option to CCFLAGS option is supported by compiler.

   """
   conf.Message("Checking whether %s supports %s... " % (conf.env['CC'],option))
   lastCFLAGS=conf.env['CCFLAGS']
   conf.env.Append(CCFLAGS = option)
   rc = conf.TryCompile("""
void dummy(void);
void dummy(void) {}
""",'.c')
   if not rc:
       try:
          lastCFLAGS.remove(option)
       except ValueError:
          pass
       conf.env.Replace(CCFLAGS = lastCFLAGS)
   conf.Result(rc)
   return rc
