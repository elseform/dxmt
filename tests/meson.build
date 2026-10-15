if dxmt_crossbuild
  subdir('dx11')
else
  subdir('native')
endif

# The converter in Wine is a native x86_64 Unix library. Exercise the same
# detector against that library configuration, not a Windows LLVM substitute.
if cpu_family in ['x86_64', 'aarch64'] or not dxmt_crossbuild
  shader_interface_relaxations = executable('shader_interface_relaxations',
    'native/shader_interface_relaxations.cpp',
    native: dxmt_crossbuild,
    dependencies: [airconv_dep_darwin, dxbc_parser_native_dep],
    include_directories: [dxmt_native_include_path, llvm_include_path_darwin,
                          include_directories('../src/airconv')],
  )
  test('shader-interface-relaxations', shader_interface_relaxations, timeout: 60)
endif
