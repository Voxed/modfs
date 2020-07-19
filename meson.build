project('ModFS', 'cpp', default_options : ['cpp_std=c++17'])

includes = include_directories('include')

sources = [
    'source/main.cc',
    'source/filefinder.cc',
    'source/overwritemanager.cc'
]

dependencies = [
    dependency('fuse3')
]

executable('ModFS', sources, include_directories : includes, dependencies : dependencies)