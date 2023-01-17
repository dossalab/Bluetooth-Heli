project('ble_copter', 'c',
  version : '0.1',
  default_options : [ 'optimization=s' ])

subdir('nrf-ble-template/targets/nrf52832')
subdir('nrf-ble-template')

executable('ble_copter',
           'main.c',
           'controls.c',
           'motors.c',
           'power.c',
           'timer.c',
           link_with : nrf_sdk,
           include_directories : nrf_sdk_inc)
