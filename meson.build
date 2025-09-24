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
    'src/catalog/catalog.cpp',
    'src/parser/parser.cpp'
)

libcore = static_library('core',
    sources: core_sources,
    include_directories: inc
)

# CLI module meson.build
cli_sources = files('src/cli/main.cpp')
cli_exe = executable('bo-sql',
    sources: cli_sources,
    include_directories: inc,
    link_with: libcore,
    dependencies: [dependency('fmt')]
)

# tests dir has its own meson.build
subdir('tests')