catch2_dep = dependency('catch2-with-main')

tests_sources = files(
    'test_main.cpp',
    'test_parser.cpp',
    'test_types.cpp',
    'test_columnar.cpp',
    'test_csv.cpp',
    'test_catalog.cpp',
    'test_logical.cpp'
)
tests_exe = executable('tests',
    sources: tests_sources,
    include_directories: inc,
    link_with: libcore,
    dependencies: [catch2_dep]
)

test('unit tests', tests_exe)