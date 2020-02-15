        setlocal
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"
        path "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin";%path%
        mkdir build
        cd build
        cmake ../
        echo "Preparing to build."
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe ./sweet_osal_platform.sln /property:Configuration=Debug -maxcpucount:4"
        cd ../

        endlocal