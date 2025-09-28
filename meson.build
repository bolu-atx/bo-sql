project('bo-sql', 'cpp',
    version : '0.1',
    default_options : ['cpp_std=c++20', 'default_library=static']
)

# Include directories
inc = include_directories('include')

# Core library
core_sources = files(
    'src/storage/dictionary.cpp',
    'src/storage/table.cpp',
    'src/storage/csv_loader.cpp',
    'src/catalog/catalog.cpp',
    'src/parser/parser.cpp',
    'src/parser/ast_to_string.cpp',
    'src/logical/logical.cpp',
    'src/logical/planner.cpp',
    'src/exec/physical_planner.cpp',
    'src/exec/execution.cpp'
)

fmt_dep = dependency('nonexistent_fmt', fallback: ['fmt', 'fmt_dep'])
libcore = static_library('core',
    sources: core_sources,
    include_directories: inc,
    dependencies: [fmt_dep]
)

# CLI module meson.build
cli_sources = files('src/cli/main.cpp')
cli_exe = executable('bq',
    sources: cli_sources,
    include_directories: inc,
    link_with: libcore,
    dependencies: [fmt_dep],
    install: true
)

# Smoke tests
test('smoke', find_program('smoke.sh'), workdir: meson.source_root(), depends: [cli_exe])

# tests dir has its own meson.build
subdir('tests')