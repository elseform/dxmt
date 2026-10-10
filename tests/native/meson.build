add_languages('objcpp', native: true)
error_ownership = executable('metal_error_ownership', 'error_ownership.mm',
  dependencies: [winemetal_dep],
  link_args: ['-framework', 'Foundation', '-framework', 'Metal'],
)
test('metal-error-ownership', error_ownership,
  env: ['NSZombieEnabled=YES'], timeout: 30,
)
