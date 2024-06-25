
set (TestToRun
  opcodes.cxx
  instructions.cxx
  readfile.cxx
)

foreach(source TestToRun)
  set_source_files_properties(${source} PROPERTIES
      SKIP_LINTING ON
  )

endforeach()



create_test_sourcelist (Tests SuperNovaTests.cxx ${TestToRun})

# add the executable 
add_executable (SuperNovaTests ${Tests})
target_link_libraries(SuperNovaTests PUBLIC supernova)
# Add all the ADD_TEST for each test

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/smaller.spn
               ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/invalid_magic.spn
               ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.spn
               ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)



add_test(NAME Rinstr COMMAND SuperNovaTests instructions rinst)
add_test(NAME Sinstr COMMAND SuperNovaTests instructions sinst)
add_test(NAME Linstr COMMAND SuperNovaTests instructions linst)
add_test(NAME opcodes COMMAND SuperNovaTests opcodes)
add_test(NAME readfile COMMAND SuperNovaTests readfile)