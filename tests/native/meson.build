add_languages('objcpp', native: true)
error_ownership = executable('metal_error_ownership', 'error_ownership.mm',
  dependencies: [winemetal_dep],
  link_args: ['-framework', 'Foundation', '-framework', 'Metal'],
)
test('metal-error-ownership', error_ownership,
  env: ['NSZombieEnabled=YES'], timeout: 30,
)

shader_interface_relaxations = executable('shader_interface_relaxations',
  'shader_interface_relaxations.cpp',
  dependencies: [airconv_dep_darwin, dxbc_parser_native_dep],
  include_directories: [include_directories('.')],
)
test('shader-interface-relaxations', shader_interface_relaxations, timeout: 60)
