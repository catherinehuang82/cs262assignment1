cmake_minimum_required(VERSION 3.5.1)

project(${DL_ARGS_PROJ}-download NONE)

include(ExternalProject)
ExternalProject_Add(${DL_ARGS_PROJ}-download
                    ${DL_ARGS_UNPARSED_ARGUMENTS}
                    SOURCE_DIR          "${DL_ARGS_SOURCE_DIR}"
                    BINARY_DIR          "${DL_ARGS_BINARY_DIR}"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
)