
add_library(python3 SHARED IMPORTED)
set_property(TARGET python3 PROPERTY IMPORTED_LOCATION ${PYTHON_LIB})

set (cpython python3.6m)


add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/rai.wrap.cpp
    COMMAND python3 -m cython --cplus ${CMAKE_CURRENT_SOURCE_DIR}/rai.pyx -o ${CMAKE_CURRENT_SOURCE_DIR}/rai.wrap.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/rai.pyx
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/pyobject.wrap.cpp
    COMMAND python3 -m cython --cplus ${CMAKE_CURRENT_SOURCE_DIR}/pyobject.pyx -o ${CMAKE_CURRENT_SOURCE_DIR}/pyobject.wrap.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pyobject.pyx
)


add_library( ${cpython}
             SHARED
    main.cpp
    pyobject.cpp
    pyobject.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pyobject.wrap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rai.wrap.cpp
)

if (APPLE)
    target_link_libraries( ${cpython} PUBLIC  intl dl util python3)
else()
    target_link_libraries( ${cpython} PUBLIC dl util python3)
endif()



target_include_directories( ${cpython}
                            PUBLIC ${PYTHON_INC}
                            PUBLIC ${CMAKE_SOURCE_DIR}
                            )

INSTALL( TARGETS
   ${cpython}
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
