catch2_dep = dependency('catch2-with-main')

test_exe = executable('tests',
  'test_main.cpp',  # Assuming a test file exists
  dependencies : [catch2_dep],
  include_directories : inc
)

test('unit tests', test_exe)