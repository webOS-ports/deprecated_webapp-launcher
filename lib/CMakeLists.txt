set(SOURCES_LIB
    baseextension.cpp
    scriptexecutor.cpp
    baseextension.h
    scriptexecutor.h)

add_library(webapp-plugin SHARED ${SOURCES_LIB})
qt5_use_modules(webapp-plugin Core)

install(FILES baseextension.h scriptexecutor.h DESTINATION include/webapp-plugin)

webos_build_library(NAME libwebapp-plugin TARGET webapp-plugin NOHEADERS)
