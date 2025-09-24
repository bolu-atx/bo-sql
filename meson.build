project('bo-sql', 'cpp',
   version : '0.1',
   default_options : ['cpp_std=c++20']
)

# Include directories
inc = include_directories('include')

# Core library
core_sources = files(
    'src/storage/dictionary.cpp',
    'src/storage/table.cpp',
    'src/storage/csv_loader.cpp',
    'src/catalog/catalog.cpp'
)

libcore = static_library('core',
    sources: core_sources,
    include_directories: inc
)

subdir('src/parser')
subdir('src/cli')
subdir('tests')