catch2_dep = dependency('catch2-with-main')

tests_sources = files('test_main.cpp')
tests_exe = executable('tests',
    sources: tests_sources,
    include_directories: inc,
    link_with: [storage_lib],
    dependencies: [catch2_dep]
)

test('unit tests', tests_exe)