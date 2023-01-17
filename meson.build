project('ble_copter', 'c',
  version : '0.1',
  default_options : [ 'optimization=s' ])

add_global_link_arguments(
  '-L' + meson.current_source_dir(),
  '-Tapp_linker.ld',
  language : 'c'
)

subdir('nrf-ble-template/targets/nrf52832')
subdir('nrf-ble-template')

executable('ble_copter',
           'main.c',
           'controls.c',
           'motors.c',
           'battery.c',
           'timer.c',
           'led.c',
           'extint.c',
           link_with : nrf_sdk,
           include_directories : nrf_sdk_inc)
