
    #include <sys/types.h>
    #include <dirent.h>

    void dummy(void);
    void dummy()
    {
	struct dirent d;
	d.d_fileno=0;
    }
    