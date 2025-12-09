cmake_minimum_required(VERSION 3.20) # 3.20 for CMAKE_<LANG>_BYTE_ORDER
project(actf
  VERSION 0.0.1
  DESCRIPTION "A cute acute CTF reader"
  LANGUAGES C)

option(BUILD_BIN   "Build test applications." ON)
option(BUILD_TESTS "Build unit tests." ON)
option(BUILD_DOC   "Build documentation." ON)

include(GNUInstallDirs)

# Setup pkg-config
SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix ${CMAKE_INSTALL_PREFIX})
SET(libdir ${CMAKE_INSTALL_FULL_LIBDIR})
SET(includedir ${CMAKE_INSTALL_FULL_INCLUDEDIR})
SET(VERSION ${PROJECT_VERSION})
SET(REQUIRES json-c)
configure_file(${PROJECT_SOURCE_DIR}/actf.pc.in ${PROJECT_BINARY_DIR}/actf.pc @ONLY)

set(C_STANDARD 11)
set(C_STANDARD_REQUIRED True)
add_definitions("-D_POSIX_C_SOURCE=200809L")

if(CMAKE_C_BYTE_ORDER STREQUAL "BIG_ENDIAN")
  set(WORDS_BIGENDIAN ON)
endif()

configure_file(${PROJECT_SOURCE_DIR}/cmake/config.h.in ${PROJECT_BINARY_DIR}/config.h)

set(ACTF_HDR_PRIVATE
  ${PROJECT_BINARY_DIR}/config.h

  ${PROJECT_SOURCE_DIR}/breader.h
  ${PROJECT_SOURCE_DIR}/crust/arena.h
  ${PROJECT_SOURCE_DIR}/crust/common.h
  ${PROJECT_SOURCE_DIR}/crust/vec.h
  ${PROJECT_SOURCE_DIR}/crust/rb_tree.h
  ${PROJECT_SOURCE_DIR}/crust/sival.h
  ${PROJECT_SOURCE_DIR}/crust/uival.h
  ${PROJECT_SOURCE_DIR}/ctfjson.h
  ${PROJECT_SOURCE_DIR}/error.h
  ${PROJECT_SOURCE_DIR}/event_int.h
  ${PROJECT_SOURCE_DIR}/event_state.h
  ${PROJECT_SOURCE_DIR}/flags_int.h
  ${PROJECT_SOURCE_DIR}/fld_cls_int.h
  ${PROJECT_SOURCE_DIR}/fld_int.h
  ${PROJECT_SOURCE_DIR}/fld_loc_int.h
  ${PROJECT_SOURCE_DIR}/json_utils.h
  ${PROJECT_SOURCE_DIR}/mappings_int.h
  ${PROJECT_SOURCE_DIR}/metadata_int.h
  ${PROJECT_SOURCE_DIR}/pkt_int.h
  ${PROJECT_SOURCE_DIR}/pkt_state.h
  ${PROJECT_SOURCE_DIR}/prio_queue.h
  ${PROJECT_SOURCE_DIR}/rng_int.h
  ${PROJECT_SOURCE_DIR}/str_vec.h
)

set(ACTF_HDR_PUBLIC
  ${PROJECT_SOURCE_DIR}/actf.h
  ${PROJECT_SOURCE_DIR}/decoder.h
  ${PROJECT_SOURCE_DIR}/event.h
  ${PROJECT_SOURCE_DIR}/event_generator.h
  ${PROJECT_SOURCE_DIR}/filter.h
  ${PROJECT_SOURCE_DIR}/flags.h
  ${PROJECT_SOURCE_DIR}/fld.h
  ${PROJECT_SOURCE_DIR}/fld_cls.h
  ${PROJECT_SOURCE_DIR}/fld_loc.h
  ${PROJECT_SOURCE_DIR}/freader.h
  ${PROJECT_SOURCE_DIR}/mappings.h
  ${PROJECT_SOURCE_DIR}/metadata.h
  ${PROJECT_SOURCE_DIR}/muxer.h
  ${PROJECT_SOURCE_DIR}/pkt.h
  ${PROJECT_SOURCE_DIR}/print.h
  ${PROJECT_SOURCE_DIR}/rng.h
  ${PROJECT_SOURCE_DIR}/types.h
)

set(ACTF_SRCS
  ${PROJECT_SOURCE_DIR}/breader.c
  ${PROJECT_SOURCE_DIR}/crust/rb_tree.c
  ${PROJECT_SOURCE_DIR}/ctfjson.c
  ${PROJECT_SOURCE_DIR}/decoder.c
  ${PROJECT_SOURCE_DIR}/error.c
  ${PROJECT_SOURCE_DIR}/event.c
  ${PROJECT_SOURCE_DIR}/event_generator.c
  ${PROJECT_SOURCE_DIR}/filter.c
  ${PROJECT_SOURCE_DIR}/flags.c
  ${PROJECT_SOURCE_DIR}/fld.c
  ${PROJECT_SOURCE_DIR}/fld_cls.c
  ${PROJECT_SOURCE_DIR}/fld_loc.c
  ${PROJECT_SOURCE_DIR}/freader.c
  ${PROJECT_SOURCE_DIR}/json_utils.c
  ${PROJECT_SOURCE_DIR}/mappings.c
  ${PROJECT_SOURCE_DIR}/metadata.c
  ${PROJECT_SOURCE_DIR}/muxer.c
  ${PROJECT_SOURCE_DIR}/pkt.c
  ${PROJECT_SOURCE_DIR}/print.c
  ${PROJECT_SOURCE_DIR}/rng.c
)

add_library(${PROJECT_NAME}
  ${ACTF_SRCS}
  ${ACTF_HDR_PUBLIC}
  ${ACTF_HDR_PRIVATE}
)

find_package(json-c CONFIG NO_CMAKE_FIND_ROOT_PATH)
target_link_libraries(${PROJECT_NAME} PRIVATE json-c::json-c)

target_include_directories(${PROJECT_NAME}
  PUBLIC ${PROJECT_BINARY_DIR}
)

if(BUILD_BIN)
  add_executable(dsr dsr.c)
  target_link_libraries(dsr PRIVATE ${PROJECT_NAME})
  add_executable(${PROJECT_NAME}.out actf.c)
  target_link_libraries(${PROJECT_NAME}.out PRIVATE ${PROJECT_NAME})
  set_property(TARGET ${PROJECT_NAME}.out PROPERTY OUTPUT_NAME ${PROJECT_NAME})
endif()

if(BUILD_TESTS)
  set(ACTF_TEST_SRCS
    ${PROJECT_SOURCE_DIR}/test_breader.c
    ${PROJECT_SOURCE_DIR}/test_ctfjson.c
    ${PROJECT_SOURCE_DIR}/test_decoder.c
    ${PROJECT_SOURCE_DIR}/test_error.c
    ${PROJECT_SOURCE_DIR}/test_filter.c
    ${PROJECT_SOURCE_DIR}/test_freader.c
    ${PROJECT_SOURCE_DIR}/test_fld_cls.c
    ${PROJECT_SOURCE_DIR}/test_metadata.c
    ${PROJECT_SOURCE_DIR}/test_prio_queue.c
    ${PROJECT_SOURCE_DIR}/test_rng.c
    ${PROJECT_SOURCE_DIR}/tests.c
  )
  set(ACTF_TEST_HDR
    ${PROJECT_SOURCE_DIR}/test_breader.h
    ${PROJECT_SOURCE_DIR}/test_ctfjson.h
    ${PROJECT_SOURCE_DIR}/test_decoder.h
    ${PROJECT_SOURCE_DIR}/test_error.h
    ${PROJECT_SOURCE_DIR}/test_filter.h
    ${PROJECT_SOURCE_DIR}/test_freader.h
    ${PROJECT_SOURCE_DIR}/test_fld_cls.h
    ${PROJECT_SOURCE_DIR}/test_metadata.h
    ${PROJECT_SOURCE_DIR}/test_prio_queue.h
    ${PROJECT_SOURCE_DIR}/test_rng.h
  )
  add_executable(tests.out
    ${ACTF_SRCS}
    ${ACTF_TEST_SRCS}
    ${ACTF_TEST_HDR}
    ${ACTF_HDR_PUBLIC}
    ${ACTF_HDR_PRIVATE}
  )
  target_include_directories(tests.out
    PRIVATE ${PROJECT_BINARY_DIR}
    PRIVATE ${PROJECT_BINARY_DIR}/crust
    PRIVATE ${PROJECT_BINARY_DIR}/include
  )
  find_package(cunit CONFIG)
  target_link_libraries(tests.out
    PRIVATE cunit
    PRIVATE json-c::json-c
  )
endif()

if(BUILD_DOC)
  find_package(Doxygen REQUIRED doxygen)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
  set(DOXYGEN_SORT_MEMBER_DOCS NO)
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
  doxygen_add_docs(doc
    ${ACTF_HDR_PUBLIC}
    README.md
  )
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(TARGET ${PROJECT_NAME}.out)
  install(TARGETS ${PROJECT_NAME}.out DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(FILES ${ACTF_HDR_PUBLIC} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/actf)
install(FILES ${PROJECT_BINARY_DIR}/actf.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
