### example CMakeLists.txt to develop programs using OpenMS
project("Fragment_Isotope_Distribution_Paper")
cmake_minimum_required(VERSION 2.8.3)

## list all your executables here (a corresponding .cpp file should exist, e.g. Main.cpp)
set(my_executables
        GenerateTrainingData
        CompareToTheoretical
        CompareToShotgun
        CompareToTargeted
        CreateAveragineModel
        GetSulfurDistribution
        SpeedTest
        ProcessCalibration
        )

## list all classes here, which are required by your executables
## (all these classes will be linked into a library)
set(my_sources
        GenerateTrainingData.cpp
        CompareToTheoretical.cpp
        CompareToShotgun.cpp
        CompareToTargeted.cpp
        CreateAveragineModel.cpp
        GetSulfurDistribution.cpp
        SpeedTest.cpp
        Stats.h
        Ion.h
        Ion.cpp
        SpectrumUtilities.h
        IsotopeDistributions.cpp
        IsotopeDistributions.h
        FASTAParser.cpp
        FASTAParser.h
        ProcessCalibration.cpp
        )

## find OpenMS configuration and register target "OpenMS" (our library)
find_package(OpenMS PATHS "/Users/dennisg/OpenMS/dev-OpenMS/")
## if the above fails you can try calling cmake with -D OpenMS_DIR=/path/to/OpenMS/
## or modify the find_package() call accordingly
#find_package(OpenMS PATHS "/Users/dennisg/OpenMS/OpenMS/")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-c++11-extensions")

# check whether the OpenMS package was found
if (OpenMS_FOUND)

    # check if the variable containing the include directories is defined
    if(NOT OpenMS_INCLUDE_DIRECTORIES)
        set(_message "The variable \${OpenMS_INCLUDE_DIRECTORIES} is not defined.")
        set(_message "${_message}This CMakeLists.txt and the build of OpenMS seem to incompatible.")
        set(_message "${_message}Please use the latest version from the OpenMS release!")
        message(FATAL_ERROR ${_message})
    endif()

    ## include directories for OpenMS headers (and contrib)
    # Note: If you want to link against a specific libirary contained
    #       in the OpenMS package you should also list the include
    #       corresponding directory here, e.g.,
    #       OpenMS_GUI -> ${OpenMS_GUI_INCLUDE_DIRECTORIES}
    include_directories(${OpenMS_INCLUDE_DIRECTORIES})

    ## append precompiler macros and compiler flags specific to OpenMS
    ## Warning: this could be harmful to your project. Check this if problems occur.
    ## Also, use this to add your own compiler flags, e.g. for OpenMP support.
    ## e.g. for Visual Studio use /openmp
    ## set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPENMS_ADDCXX_FLAGS} /openmp")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPENMS_ADDCXX_FLAGS}")
    add_definitions(${OPENMS_DEFINITIONS})

    ## library with additional classes from above
    add_library(my_custom_lib STATIC ${my_sources})

    ## add targets for the executables
    foreach(i ${my_executables})
        add_executable(${i} ${i}.cpp)
        ## link executables against OpenMS
        target_link_libraries(${i} OpenMS my_custom_lib)
    endforeach(i)


else(OpenMS_FOUND)
    message(FATAL_ERROR "OpenMSConfig.cmake file not found!")
endif(OpenMS_FOUND)
