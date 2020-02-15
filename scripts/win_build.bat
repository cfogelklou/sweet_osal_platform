        setlocal
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
        path "C:\Program Files (x86)\MSBuild\16.0\Bin;C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin";%path%
        mkdir build
        cd build
        cmake -G "Visual Studio 16 2019" ..
        echo "Stuff that's here."
        dir
        echo "Preparing to build."
        msbuild sweet_osal_platform.sln /property:Configuration=Debug -maxcpucount:4
        cd ../

        endlocal