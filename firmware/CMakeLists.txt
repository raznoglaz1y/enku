cmake_minimum_required(VERSION 3.16)

project(enku_host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_library(enku_core STATIC
    src/parsers/txt/txt_parser.cpp
    src/reader/text_paginator.cpp
    src/reader/document_reader_engine.cpp
    src/reader/reader_session.cpp
    src/reader/book_loader.cpp
    src/runtime/reader_runtime.cpp
    src/runtime/storage_startup.cpp
    src/storage/cbor_reader_checkpoint.cpp
    src/storage/cbor_library_service.cpp
    src/storage/book_import_service.cpp
    src/storage/staged_book_import_service.cpp
    src/storage/posix_book_file_store.cpp
    src/storage/posix_state_file_store.cpp
)

target_include_directories(enku_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(enku_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

function(enku_add_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE enku_core)

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /UNDEBUG)
    endif()

    add_test(NAME ${target} COMMAND ${target})
endfunction()

enku_add_test(txt_parser_tests tests/txt_parser_tests.cpp)
enku_add_test(txt_pagination_tests tests/txt_pagination_tests.cpp)
enku_add_test(reader_engine_tests tests/reader_engine_tests.cpp)
enku_add_test(reader_session_tests tests/reader_session_tests.cpp)
enku_add_test(book_loader_tests tests/book_loader_tests.cpp)
enku_add_test(reader_runtime_tests tests/reader_runtime_tests.cpp)
enku_add_test(cbor_reader_checkpoint_tests tests/cbor_reader_checkpoint_tests.cpp)
enku_add_test(cbor_library_service_tests tests/cbor_library_service_tests.cpp)
enku_add_test(book_import_service_tests tests/book_import_service_tests.cpp)
enku_add_test(staged_book_import_tests tests/staged_book_import_tests.cpp)
enku_add_test(storage_startup_tests tests/storage_startup_tests.cpp)
enku_add_test(posix_checkpoint_integration_tests tests/posix_checkpoint_integration_tests.cpp)
