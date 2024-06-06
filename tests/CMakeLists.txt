
set (TestToRun
  opcodes.cxx
  instructions.cxx
)
create_test_sourcelist (Tests SuperNovaTests.cxx ${TestToRun})

# add the executable 
add_executable (SuperNovaTests ${Tests})
target_link_libraries(SuperNovaTests PUBLIC supernova)
# Add all the ADD_TEST for each test

add_test(NAME Rinstr COMMAND SuperNovaTests instructions rinst)
add_test(NAME Sinstr COMMAND SuperNovaTests instructions sinst)
add_test(NAME Linstr COMMAND SuperNovaTests instructions linst)
add_test(NAME opcodes COMMAND SuperNovaTests opcodes)