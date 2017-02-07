echo "You must run this from a Visual Studio developer commmand line"

set TYPE=Debug
set ARCH=x64

set BUILDDIR=build_%ARCH%_%TYPE%
call "%VCINSTALLDIR%\vcvarsall" %ARCH%

mkdir %BUILDDIR%
cd %BUILDDIR%
cmake .. -DCMAKE_BUILD_TYPE=%TYPE% -G "NMake Makefiles"
nmake
cd ..

