if (WITH_CUDA)
    add_definitions(/DXMRIG_FEATURE_CUDA)

    set(HEADERS_BACKEND_CUDA
        src/backend/cuda/CudaBackend.h
        src/backend/cuda/CudaConfig_gen.h
        src/backend/cuda/CudaConfig.h
        src/backend/cuda/CudaLaunchData.h
        src/backend/cuda/CudaThread.h
        src/backend/cuda/CudaThreads.h
        src/backend/cuda/CudaWorker.h
        src/backend/cuda/interfaces/ICudaRunner.h
        src/backend/cuda/runners/CudaBaseRunner.h
        src/backend/cuda/runners/CudaCnRunner.h
        src/backend/cuda/wrappers/CudaDevice.h
        src/backend/cuda/wrappers/CudaLib.h
       )

    set(SOURCES_BACKEND_CUDA
        src/backend/cuda/CudaBackend.cpp
        src/backend/cuda/CudaConfig.cpp
        src/backend/cuda/CudaLaunchData.cpp
        src/backend/cuda/CudaThread.cpp
        src/backend/cuda/CudaThreads.cpp
        src/backend/cuda/CudaWorker.cpp
        src/backend/cuda/runners/CudaBaseRunner.cpp
        src/backend/cuda/runners/CudaCnRunner.cpp
        src/backend/cuda/wrappers/CudaDevice.cpp
        src/backend/cuda/wrappers/CudaLib.cpp
       )

   if (WITH_NVML AND NOT APPLE)
       add_definitions(/DXMRIG_FEATURE_NVML)

       list(APPEND HEADERS_BACKEND_CUDA
           src/backend/cuda/wrappers/nvml_lite.h
           src/backend/cuda/wrappers/NvmlHealth.h
           src/backend/cuda/wrappers/NvmlLib.h
           )

       list(APPEND SOURCES_BACKEND_CUDA src/backend/cuda/wrappers/NvmlLib.cpp)
   else()
       remove_definitions(/DXMRIG_FEATURE_NVML)
   endif()

   if (WITH_RANDOMX)
       list(APPEND HEADERS_BACKEND_CUDA src/backend/cuda/runners/CudaRxRunner.h)
       list(APPEND SOURCES_BACKEND_CUDA src/backend/cuda/runners/CudaRxRunner.cpp)
   endif()

   if (WITH_ASTROBWT)
       list(APPEND HEADERS_BACKEND_CUDA src/backend/cuda/runners/CudaAstroBWTRunner.h)
       list(APPEND SOURCES_BACKEND_CUDA src/backend/cuda/runners/CudaAstroBWTRunner.cpp)
   endif()
else()
    remove_definitions(/DXMRIG_FEATURE_CUDA)
    remove_definitions(/DXMRIG_FEATURE_NVML)

    set(HEADERS_BACKEND_CUDA "")
    set(SOURCES_BACKEND_CUDA "")
endif()
