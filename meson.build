project('bo-sql', 'cpp',
   version : '0.1',
   default_options : ['cpp_std=c++14']
)

# Include directories
inc = include_directories('include')

# Core library
core_sources = files(
    'storage/dictionary.cpp',
    'storage/table.cpp',
    'catalog/catalog.cpp'
)

libcore = static_library('core',
    sources: core_sources,
    include_directories: inc
)

# Subdirectories
subdir('engine')
subdir('parser')
subdir('planner')
subdir('exec')
subdir('storage')
subdir('catalog')
subdir('cli')
subdir('tests')