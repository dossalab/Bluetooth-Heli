project('ble_copter', 'c',
  version : '0.1',
  default_options : [ 'optimization=s' ])

subdir('nrf-ble-template/targets/nrf52832')
subdir('nrf-ble-template')

version_info_header = vcs_tag(
            command: ['git', 'describe', '--always', '--dirty', '--tags'],
            input : 'version.h.in',
            output : 'version.h')

executable('ble_copter',
           'main.c',
           'controls.c',
           'motors.c',
           'gyro.c',
           'power.c',
           version_info_header,
           'version.c',
           'error-handlers.c',
           dependencies : nrf_sdk_dep)
