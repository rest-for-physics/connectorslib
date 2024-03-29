set_library_version(LibraryVersion)
add_definitions(-DLIBRARY_VERSION="${LibraryVersion}")

set(excludes)

set(deps detector geant4 track raw)
if (NOT ${RESTLIB_RAW} MATCHES "ON")
    list(REMOVE_ITEM deps raw)
    set(excludes ${excludes} TRestDetectorSignalToRawSignalProcess
                 TRestRawReadoutAnalysisProcess TRestRawToDetectorSignalProcess)
endif ()
if (NOT ${RESTLIB_TRACK} MATCHES "ON")
    list(REMOVE_ITEM deps track)
    set(excludes
        ${excludes} TRestDetectorHitsToTrackProcess
        TRestDetectorHitsToTrackFastProcess TRestTrackToDetectorHitsProcess)
endif ()
if (NOT ${RESTLIB_GEANT4} MATCHES "ON")
    list(REMOVE_ITEM deps geant4)
    set(excludes ${excludes} TRestGeant4ToDetectorHitsProcess)
endif ()

list(LENGTH deps ndeps)
if (NOT ${ndeps} MATCHES 1)
    compilelib(deps)
else ()
    message(
        WARNING
            "Library RESTConnectors requires at least one active library (${deps})"
    )
endif ()

add_library_test()
