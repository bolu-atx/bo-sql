# CLI module meson.build

cli_sources = files('main.cpp')

cli_exe = executable('bo-sql',
    sources: cli_sources,
    include_directories: inc,
    link_with: libcore,
    dependencies: [dependency('fmt')]
)