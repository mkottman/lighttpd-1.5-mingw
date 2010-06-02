lighttpd-1.5-mingw
==================

A set of patches from myself and various hints and other patches found on internet,
which enables you to compile lighttpd-1.5 on Windows using MinGW and CMake, with FastCGI support.

I have been able to successfully compile and run lighttpd 1.5 on Windows, running the
[Lua Kepler framework](http://www.keplerproject.org/) (WSAPI, Orbit) through FastCGI - other
languages and frameworks should work too.

Patches are for SVN revision 2724, however I think that it will work on future versions,
if not, drop me a message, I will update it.

Requirements
------------

* SVN - get the lighttpd source: `svn checkout svn://svn.lighttpd.net/lighttpd/trunk/ lighttpd`
* MinGW - for an unknown reason, the provided GCC 3.5 compiler fails to compile one module, you
          have to [update the compiler to version 4.5](http://www.mingw.org/wiki/HOWTO_Install_the_MinGW_GCC_Compiler_Suite)
          (run these commands in MSYS)
* CMake - http://www.cmake.org/cmake/resources/software.html
* FastCGI - see below
* some other libraries like PCRE and Zlib/Bzlib - info on how to obtain these in `lighttpd/doc/build-win32.txt`, or
    try to find them in [MinGW repository](http://sourceforge.net/projects/mingw/files/MinGW) and
    [here](http://sourceforge.net/projects/kde-windows/files/pcre/) or compile yourself, and copy the libs into `MinGW/lib` and
    headers into `MinGW/include`

Optional:

* OpenSSL - either download [Win32OpenSSL](http://www.slproweb.com/products/Win32OpenSSL.html) or build yourself, however make sure CMake will find it (it will surely find Win32OpenSSL)

Compilation
------------

Copy mingw.patch to lighttpd source directory, and issue the following commands in the cmd.exe (not in MSYS)
in the lighttpd directory:

    patch -p0 < mingw.patch
    mkdir build
    cd build
    cmake-gui ..                                    # choose "MinGW Makefiles" and check everything you want included ... 
    cmake -G "MinGW Makefiles" -DWITH_OPENSSL=ON .. # ... or enter everything from command line
    make

Then compile spawn-fcgi (source included). It was originally posted by keathmilligan at 
[lighttpd forums](http://redmine.lighttpd.net/boards/2/topics/686) and I modified it to wait for the spawned processes:

    gcc -o spawn-fcgi spawn-fcgi-win32.c -lws2_32

FastCGI
-------

The easy way to get the library for MinGW is to download the [FastCGI DevPack](http://osdir.com/ml/web.fastcgi.devel/2005-04/msg00012.html)

The following is an extract from the announcement:

> The package is avaible here:
> http://prdownloads.sourceforge.net/devpaks/libfcgi-2.4.0-1cm.DevPak?download
> 
> Those who want to have these files and are not using Dev-Cpp, but want to use Mingw: 
> Please rename libfcgi-2.4.0-1cm.DevPak to libfcgi-2.4.0-1cm.tar.bz2 and extract the files in this archive. 
> Now you should be able to use them and copy them to your mingw installation.

So, rename, extract, copy to MinGW folder, CMake should find it.

