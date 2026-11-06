# Warnings as errors for first-party C. C++-only flags stay on the C++ targets.
function(ascend_forge_set_c_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target_name} PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      -Wconversion
      -Wshadow
      -Wcast-align
      -Wunused
    )
  endif()
endfunction()
