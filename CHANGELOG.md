# Changelog

## [0.6.2](https://github.com/criotive/esphome-sdk/compare/v0.6.1...v0.6.2) (2026-10-23)


### Bug Fixes

* **serial-api:** harden the line protocol for third-party clients ([7019812](https://github.com/criotive/esphome-sdk/commit/701981244e94447d5e103d15f819083979971dfe))


### Documentation

* **serial-api:** rewrite the protocol reference for client developers ([4de6606](https://github.com/criotive/esphome-sdk/commit/4de6606d456e14240b2f2279309050ccd2d518de))

## [0.6.1](https://github.com/criotive/esphome-sdk/compare/v0.6.0...v0.6.1) (2026-10-23)


### Bug Fixes

* **serial-api:** build with ESPHome 2026.10 ([73dc701](https://github.com/criotive/esphome-sdk/commit/73dc70163a4963321ebbf821b000d1582ce6ac59))

## [0.6.0](https://github.com/criotive/esphome-sdk/compare/v0.5.1...v0.6.0) (2026-10-23)


### ⚠ BREAKING CHANGES

* **serial-api:** modules/serial_api.yaml adds two required substitutions (serial_api_tx_pin, serial_api_rx_pin) with no default. modules/ is one shared namespace, so check-contract-diff.sh classifies these as breaking. No deployed config breaks: nothing imports the opt-in module yet.

### Features

* **serial_api:** read path, entity enumeration and exposure rule ([71ce99e](https://github.com/criotive/esphome-sdk/commit/71ce99e39d3b7020e28a7b67de33764ccb21dc36))
* **serial_api:** write path (SET) and per-type value adapters ([8644193](https://github.com/criotive/esphome-sdk/commit/86441937238734255b3e0f8dbc2c0ab532540f37))
* **serial-api:** module, required pins and the pin-conflict validation gate ([e4a3d27](https://github.com/criotive/esphome-sdk/commit/e4a3d2733a88c79be6232a9c43460dab3855fdb2))


### Documentation

* **serial-api:** protocol reference, wiring and exposure contract ([e8b83f8](https://github.com/criotive/esphome-sdk/commit/e8b83f849873d3d6bbf69ff56d5dc0e0fb0351a6))

## [0.5.1](https://github.com/criotive/esphome-sdk/compare/v0.5.0...v0.5.1) (2026-10-21)


### Features

* **hardware:** name the HDS expander channels' TCA8418 pins ([c47d9cd](https://github.com/criotive/esphome-sdk/commit/c47d9cd8266ee1b6386bdd172bb7bc966a3bde58))

## [0.5.0](https://github.com/criotive/esphome-sdk/compare/v0.4.1...v0.5.0) (2026-10-18)


### ⚠ BREAKING CHANGES

* **ota:** the SDK requires ESPHome 2026.8.0 or later; core.yaml sets min_version, so an older ESPHome refuses the config. An OTA url must be served with a certificate the ESP-IDF CA bundle trusts.
* **ota:** controls.yaml no longer publishes system_restart_safe_mode.
* **ciotcfg:** the ciotcfg row is type 0x40, subtype 0x00. A device flashed with a 0.4.x image (data, subtype 0x40) does not find its record and keeps MQTT off until it is reflashed.
* **sdk:** a config whose esp32 framework is arduino no longer validates; set `esp32: framework: type: esp-idf`.
* **sdk:** modules/ha_api.yaml and its api_reboot_timeout substitution are gone. A config that imports the module must drop it.
* **core:** device_name is now at most 24 characters, and the hostname, mDNS name and discovery node id carry the MAC suffix.
* **improv:** a provisioned device no longer starts BLE, not even after a Wi-Fi outage. Re-provision it over the serial cable or after a factory reset.

### Features

* **sdk:** build with ESP-IDF only ([4403a35](https://github.com/criotive/esphome-sdk/commit/4403a354fda70287678a995915032005b39b21be))
* **sdk:** remove the Home Assistant native API module ([0e62116](https://github.com/criotive/esphome-sdk/commit/0e62116be2845730a26fd997132cf2203fb3e709))


### Bug Fixes

* **ciotcfg:** fail closed on a record that cannot be allocated, and check the partition row ([af32249](https://github.com/criotive/esphome-sdk/commit/af322497ba6092891367f9cace0d2055e612a5e1))
* **ciotcfg:** give the partition a type of its own, 0x40 ([d63bcc8](https://github.com/criotive/esphome-sdk/commit/d63bcc834737448c5d9c86583434a8b4131d261e))
* **core:** append the MAC to the device name ([b68e0d5](https://github.com/criotive/esphome-sdk/commit/b68e0d57e1c91f633ee0fbce5ceba127df282990))
* **improv:** run the BLE stack only while a device has no Wi-Fi credentials ([60918b3](https://github.com/criotive/esphome-sdk/commit/60918b395079d4a1bed758a5a65b6a42b60969fa))
* **ota:** confirm an update only from the broker, and verify the download's TLS ([0a7e719](https://github.com/criotive/esphome-sdk/commit/0a7e719e74ec916251a2060a4eaf5388c1c5ccad))
* **ota:** fall back to the previous confirmed image on a boot loop ([336b37f](https://github.com/criotive/esphome-sdk/commit/336b37fb7838cb9c2d42819c1a16c6b44dfe61c4))
* **ota:** report a malformed ota command instead of ignoring it ([b540cd5](https://github.com/criotive/esphome-sdk/commit/b540cd5da5e1de2b96a234f6afc347850a9af69a))
* **sdk:** fetch modules and components from criotive/esphome-sdk ([f77b115](https://github.com/criotive/esphome-sdk/commit/f77b11595c684f34eabc016ede4b4503afa21485))
* **sleep:** check again before sleeping, after the second that publishes Stay Awake ([a3fcde7](https://github.com/criotive/esphome-sdk/commit/a3fcde764cd0b40da710c39b5367eb1f6af64d36))


### Documentation

* pin the README example to v0.5.0 and point the changelog at this repository ([10b624e](https://github.com/criotive/esphome-sdk/commit/10b624efa172584492e5aaacb6f4bff1942b7e94))

## [0.4.1](https://github.com/criotive/esphome-sdk/compare/v0.4.0...v0.4.1) (2026-10-17)


### Features

* **api:** Home Assistant's native API, keyed by its first client and never rebooting for lack of one ([c63cda1](https://github.com/criotive/esphome-sdk/commit/c63cda162f503bd7270457c50877fe864a8b6e84))
* **sleep:** a deep-sleep cycle that never sleeps through an unconfirmed update, with a Stay Awake switch ([3c6a365](https://github.com/criotive/esphome-sdk/commit/3c6a36586356377169e62ceed27b3c8cfc6a1011))


### Bug Fixes

* **sdk:** run the SDK's boot actions from sdk_boot, which a device's on_boot cannot replace ([72413ab](https://github.com/criotive/esphome-sdk/commit/72413ab7715989b50f4f5a5cf02dbd597d791a8d))


### Documentation

* **readme:** the native API and deep-sleep modules ([a8d35c1](https://github.com/criotive/esphome-sdk/commit/a8d35c14bbdc96a2805edb5ce19c4faa21b85e6d))
* **readme:** why the SDK's boot actions run from sdk_boot ([4faa1e7](https://github.com/criotive/esphome-sdk/commit/4faa1e7755b244c7bd7f9630ab7478353e8c3118))

## [0.4.0](https://github.com/criotive/esphome-sdk/compare/v0.3.4...v0.4.0) (2026-10-04)


### ⚠ BREAKING CHANGES

* **logger:** the logger_hardware_uart substitution is removed. A device that needs a specific console sets logger: hardware_uart: in its own config.
* **ota:** the device no longer sends HTTP Basic auth on the ota download, so the platform must accept the ticket from the url. The legacy OTA HTTPS prod text entity, perform_ota_update and the ota_http_server / ota_http_server_test substitutions are removed; ota_password is optional and, left empty, removes the native OTA port.
* **wifi:** wifi.yaml no longer raises a fallback access point or takes wifi_ap_password. A device that wants the AP imports modules/wifi_ap.yaml, which requires wifi_ap_password. A device with no station slot must import improv_serial.yaml, improv_ble.yaml or wifi_ap.yaml.
* **mqtt:** a consuming config must import modules/ciotcfg.yaml and the device must be flashed with a ciotcfg record. Remove criotive_env, mqtt_username, mqtt_password and mqtt_client_id from its substitutions. A device already in the field does not gain the partition over the air - the partition table is written only by the factory image, over serial.

### Features

* **ciotcfg:** read the per-device MQTT identity from a flash partition ([dc9144c](https://github.com/criotive/esphome-sdk/commit/dc9144c0cbfb78fdf6e3b6e1ef4ae64fe7339c62))
* **improv:** provision over BLE with esp32_improv ([0b4dcdd](https://github.com/criotive/esphome-sdk/commit/0b4dcdd182a56e8fdd7bac7665bb4c20ebdd0c27))
* **improv:** provision over the serial cable with improv_serial ([8c58f02](https://github.com/criotive/esphome-sdk/commit/8c58f02b6e0ed9709e2d3ff6857558309165b57b))
* **mqtt:** take the broker, credentials and CA from the ciotcfg partition ([0dbe39a](https://github.com/criotive/esphome-sdk/commit/0dbe39ae21e9209ba95fdd503b783096757bf073))
* **ota:** fetch the command url without credentials and keep it out of the logs ([719c939](https://github.com/criotive/esphome-sdk/commit/719c9395b496430eb8738d1a99438ac5bb84d108))
* **ota:** flash from an mqtt command carrying a ticketed url ([be359e5](https://github.com/criotive/esphome-sdk/commit/be359e52cbfc89de1d29ca619d3718cd785dd6c9))
* **wifi:** move the fallback access point into an opt-in wifi_ap module ([7e2353f](https://github.com/criotive/esphome-sdk/commit/7e2353f30f0a9a24f6c0f3c502ee6123e9ee6000))


### Bug Fixes

* **logger:** use each variant's own console and let improv_serial open it ([46c8b0b](https://github.com/criotive/esphome-sdk/commit/46c8b0b379a5599b615e0afafe3dd028801c0568))
* **sdk:** fetch every module's components from the repository that publishes its tags ([8c37402](https://github.com/criotive/esphome-sdk/commit/8c37402e2bae91275c7d069847bb6f71c0e9568f))


### Documentation

* **readme:** the recommended main.yaml, what each module publishes, and the OTA route ([2158633](https://github.com/criotive/esphome-sdk/commit/2158633dc569d8977b6b57da8228dd7ed8a500ed))
* **readme:** the two Improv routes next to the captive portal ([e546a5d](https://github.com/criotive/esphome-sdk/commit/e546a5de46556316deb7ed15cd52d8f295c9dc05))

## [0.3.4](https://github.com/criotive/esphome-sdk/compare/v0.3.3...v0.3.4) (2026-08-25)


### Features

* **linear-motor:** drive an H-bridge linear actuator as a switch ([de3c847](https://github.com/criotive/esphome-sdk/commit/de3c847d4cf04a199137b1371297ff2fde118389))


### Bug Fixes

* **ota:** give the OTA HTTP client a 15s timeout instead of ESPHome's 4.5s ([78576ae](https://github.com/criotive/esphome-sdk/commit/78576aecff8722a17be8de8a5fd3831cbbfa6509))

## [0.3.3](https://github.com/criotive/esphome-sdk/compare/v0.3.2...v0.3.3) (2026-08-19)


### Bug Fixes

* **hardware:** give the task watchdog 60s so OTA can finalize ([04470f5](https://github.com/criotive/esphome-sdk/commit/04470f56ccc6ca5471b442e4be3245c81c86d94b))

## [0.3.2](https://github.com/criotive/esphome-sdk/compare/v0.3.1...v0.3.2) (2026-08-18)


### Bug Fixes

* **location:** handle platform location requests ([a678a9d](https://github.com/criotive/esphome-sdk/commit/a678a9dda2737efbe030a9387900ae91ab5220d9))

## [0.3.1](https://github.com/criotive/esphome-sdk/compare/v0.3.0...v0.3.1) (2026-08-18)


### Bug Fixes

* **mqtt:** publish from the worker thread instead of the main loop ([0f34205](https://github.com/criotive/esphome-sdk/commit/0f342059304346ed945f5b2ee55a04a7b16f618d))
* **tca8418:** stop each pin invalidating its own cache entry on read ([938120d](https://github.com/criotive/esphome-sdk/commit/938120d86c8ef37440f462a2952a74e1ba91ad2a))


### Performance

* **tca8418:** read the expander at 20Hz instead of every loop pass ([4e3b418](https://github.com/criotive/esphome-sdk/commit/4e3b4189450d33f85228ea54c340fa231eda163a))

## [0.3.0](https://github.com/c-iot-systems/esphome-sdk/compare/v0.2.0...v0.3.0) (2026-08-17)


### ⚠ BREAKING CHANGES

* **controls:** the four operator-facing controls publish under new MQTT object_ids. A consumer that matches them by name must read the `system_` prefix; devices must be reflashed for the new names to appear.

### Features

* **controls:** prefix the operator-facing device controls with system_ ([af62e28](https://github.com/c-iot-systems/esphome-sdk/commit/af62e2820a7034b213a57741038fad6bf0bda0b5))

## [0.2.0](https://github.com/c-iot-systems/esphome-sdk/compare/v0.1.2...v0.2.0) (2026-08-14)


### ⚠ BREAKING CHANGES

* **mqtt:** `mqtt_broker`, `mqtt_port` and `mqtt_ca_certificate` are no longer caller-supplied and are replaced by a required `criotive_env` (`prod` or `hmg`). A consuming config drops the three substitutions and adds `criotive_env`. Credentials are unchanged: `mqtt_username` and `mqtt_password` stay required with no defaults.

### Features

* **mqtt:** pin the broker per environment with criotive_env and ship both CAs ([24c7df8](https://github.com/c-iot-systems/esphome-sdk/commit/24c7df8a8ce0bab852f1ed3c6137847e5d81cadc))


### Documentation

* **mqtt:** state the internal substitutions plainly instead of claiming a mismatch is unrepresentable ([616dffc](https://github.com/c-iot-systems/esphome-sdk/commit/616dffcd4135d6067b8f41ba5dacef6762e7114c))

## [0.1.2](https://github.com/c-iot-systems/esphome-sdk/compare/v0.1.1...v0.1.2) (2026-08-14)


### Features

* **ack_button:** a button that acknowledges its press on the state topic ([4ab0611](https://github.com/c-iot-systems/esphome-sdk/commit/4ab06111af79f96554b480d08e74e3997bea8b8c))
* **controls,location:** move the SDK's own buttons onto ack_button and gate it ([3a750ea](https://github.com/c-iot-systems/esphome-sdk/commit/3a750eac1300068f19494be565fa254de53892e5))

## [0.1.1](https://github.com/c-iot-systems/esphome-sdk/compare/v0.1.0...v0.1.1) (2026-08-14)


### Features

* **hardware:** publish the Power connector's CAN pins as can_tx / can_rx ([a924536](https://github.com/c-iot-systems/esphome-sdk/commit/a9245362f55ca1f1a3887f2102676e3a852e39cb))
* **phone:** bound the accumulated input with max_length ([0aa4828](https://github.com/c-iot-systems/esphome-sdk/commit/0aa48289659fa8da6387bfac1e9c2b11baf45f61))
* **phone:** runtime passwords, submit/clear actions, on_no_match, max_length — plus can_tx/can_rx ([1a781bc](https://github.com/c-iot-systems/esphome-sdk/commit/1a781bcd38a7953ff7bf40f7ae714293fbf2d9fd))
* **phone:** templatable passwords, submit/clear actions, and on_no_match ([6071b86](https://github.com/c-iot-systems/esphome-sdk/commit/6071b863801932a64aceac50e20db8d64ec331fb))


### Bug Fixes

* **phone:** dispatch callbacks before clearing and resolve passwords up front ([34bb500](https://github.com/c-iot-systems/esphome-sdk/commit/34bb5000738a82b47afa3a68abc65463682a893a))
* **phone:** let sequence_timeout: 0s disable the timeout instead of firing it always ([be7e7e1](https://github.com/c-iot-systems/esphome-sdk/commit/be7e7e11c3530b0a287e0ea051390e7f23977c15))
* **phone:** override Action::play correctly and snapshot the submitted input ([9356282](https://github.com/c-iot-systems/esphome-sdk/commit/93562824644ea1a0bfcc262dbacda91b091b3050))

## 0.1.0 (2026-08-14)


### ⚠ BREAKING CHANGES

* **hardware:** hardware/hds_v2_0.yaml and its 104 slot substitutions are gone. Nothing ever depended on them: no version of this SDK has been released, and the board was only ever a concept.

### Features

* **ci:** add check-offline-survival gate for reboot_timeout defaults ([bb00dcf](https://github.com/c-iot-systems/esphome-sdk/commit/bb00dcf4549c0fd2158a7a283c2f1df7d7c7be4b))
* **ci:** compile validate fixtures on main, while a release is still only a proposal ([7276fd4](https://github.com/c-iot-systems/esphome-sdk/commit/7276fd4ce8e2c287ca5cb97d60f346d900bf7dd4))
* **ci:** compute versions with release-please and a pre-1.0 re-pin mapping ([30d4b0a](https://github.com/c-iot-systems/esphome-sdk/commit/30d4b0aceb3386003dfc26bbadf3ad70a324172a))
* **ci:** gate breaking substitution-contract changes on a breaking commit marker ([769a2a2](https://github.com/c-iot-systems/esphome-sdk/commit/769a2a2991d02f1ee45eed4600a33f91aedc1b78))
* **hardware:** add esp-idf board variants and opt-in SW1 unit off GPIO1 ([94d061f](https://github.com/c-iot-systems/esphome-sdk/commit/94d061fc1f6c6fa5a725b8840bce8a224ccb2dfe))
* **hardware:** add static slot pin tables for hds_v1_0/v1_1/v2_0 with slot-name fixtures ([897a023](https://github.com/c-iot-systems/esphome-sdk/commit/897a02303da4ee36689aca46b6ff148793f9d0c8))
* **mqtt:** add settable mqtt_client_id defaulting to device_name ([0d297ce](https://github.com/c-iot-systems/esphome-sdk/commit/0d297ce0260d58258b7445cac3c10c4fc62e7339))
* **sdk:** core.yaml, wifi.yaml, substitution contract and negative harness ([126b463](https://github.com/c-iot-systems/esphome-sdk/commit/126b46390eefc56702e40e33a10982d2c73c1712))
* **wifi:** three optional station slots and a provisioning-only mode ([ceb5427](https://github.com/c-iot-systems/esphome-sdk/commit/ceb542735d9bf573dc791810088b50e0867f2f52))


### Bug Fixes

* **ci:** close review gaps — fixture-only guard, flow/anchor detection, sdk_ref presence ([70b248f](https://github.com/c-iot-systems/esphome-sdk/commit/70b248f094a183af864ef5f5b21c57e309049a4e))
* **ci:** compile every fixture before failing, so one run reports every break ([6ce07ce](https://github.com/c-iot-systems/esphome-sdk/commit/6ce07ce0a5083f6ef926ad8468ad2bce59fbc36e))
* **ci:** detect list-item/quoted/spaced on_* keys and shorthand/vacuous SDK refs ([e920a03](https://github.com/c-iot-systems/esphome-sdk/commit/e920a03931f4963b85e73b30007caaf55cf9f378))
* **ci:** gate the release on the contract since the last tag, not only on the PR pass ([79664e0](https://github.com/c-iot-systems/esphome-sdk/commit/79664e0047ae75acc3586a67da0f450b93ebbf40))
* **ci:** ignore commented-out reboot_timeout in presence check ([36ffd81](https://github.com/c-iot-systems/esphome-sdk/commit/36ffd811026c34c67ba6c14cde7d79cbfe45288d))
* **ci:** make the release job need the compile so a tag cannot outrun it ([f288b0b](https://github.com/c-iot-systems/esphome-sdk/commit/f288b0b4a90978f4a595d6e2c3cfa5d7cd14579c))
* **ci:** per-component quote-aware reboot_timeout presence check ([a7e4cad](https://github.com/c-iot-systems/esphome-sdk/commit/a7e4cad98ff45e6d7165f0301bd91ec478c267b9))
* **ci:** publish only from the run that verified the release commit, and run the negative harness pre-release ([01575c6](https://github.com/c-iot-systems/esphome-sdk/commit/01575c6e2921b51585b00ab60b596c18f47c5826))
* **ci:** reject fractional non-zero timeouts and missing reboot_timeout keys ([a86215d](https://github.com/c-iot-systems/esphome-sdk/commit/a86215d6c9ea4d263be5e6701e67b6d8d395f31b))
* **ci:** report a deleted module as one path finding, not one per substitution ([ef0da48](https://github.com/c-iot-systems/esphome-sdk/commit/ef0da48183ea9fc1deb68d5eedbba77a4da14f31))
* **ci:** require reboot_timeout as a direct component child (block-scalar safe) ([201bdff](https://github.com/c-iot-systems/esphome-sdk/commit/201bdff15e6bee28d05d3101f4b1856fdeb6ea62))
* **ci:** scope the contract per module and fail closed on malformed guards and unresolvable refs ([1b9d6c4](https://github.com/c-iot-systems/esphome-sdk/commit/1b9d6c4be870380f0131f75f9517ea13ec7898d5))
* **ci:** treat module paths as contract and guard the release with every invariant gate ([4bf4e8a](https://github.com/c-iot-systems/esphome-sdk/commit/4bf4e8ab00f7cda1a2bd60fc310488d70c369aa5))
* **core:** default logger_level to NONE for production parity ([d74e994](https://github.com/c-iot-systems/esphome-sdk/commit/d74e9941f2725b63fb41309ba135f76f2a13723b))
* **core:** do not reintroduce infinite_* substitutions per master correction ([ba4e87c](https://github.com/c-iot-systems/esphome-sdk/commit/ba4e87c8ac8e4bfb94ac953928e3e707613caad2))
* **diagnostics:** poll on a real interval, not `never` ([f18f030](https://github.com/c-iot-systems/esphome-sdk/commit/f18f030dd00fec06f35aab9fc0cb715fcf650238))
* **google_location:** AUTO_LOAD json so the geolocation request body compiles ([08ab1eb](https://github.com/c-iot-systems/esphome-sdk/commit/08ab1ebf3a36d6d8a12c419ff8297784eccfa51e))
* **hardware:** make the SW1 flag settable from a string and stop diagnostics polling at 1 kHz ([bfc0d25](https://github.com/c-iot-systems/esphome-sdk/commit/bfc0d259058f618cefc2111dd049b5c29597a743))
* **hardware:** remove hds_v2_0 — the v2.0 board never existed ([2695623](https://github.com/c-iot-systems/esphome-sdk/commit/26956235e77d0835b5b6fa97d743d2e10d7d205a))
* **hardware:** restore hds_v1_1_sw1 opt-in; composition, not baud rate, gates SW1 ([c5e1119](https://github.com/c-iot-systems/esphome-sdk/commit/c5e11190459579607b3473520ccb1fa5aac4a65a))
* **logger:** gate serial console on logger_baud_rate so SW1 rejoins hds_v1_1 ([9db0858](https://github.com/c-iot-systems/esphome-sdk/commit/9db08582d9b12232b52f8ec9062d7f7e5128b860))
* **mqtt:** make mqtt_port a required substitution with no default ([6002378](https://github.com/c-iot-systems/esphome-sdk/commit/60023788617b938771e7c0c15a437ecb0c64260e))
* **ota:** defer safe_mode confirmation past the rollback watchdog so bad OTAs still roll back ([822696e](https://github.com/c-iot-systems/esphome-sdk/commit/822696ea3d1566f0803e68f6c3815b31785f5bb8))
* **ota:** gate enable_ota_rollback on ${ota_rollback} so rollback support is esp-idf-only ([ff062fc](https://github.com/c-iot-systems/esphome-sdk/commit/ff062fcc85aa9cb1113cf39eb568f64aa1b22eeb))
* **ota:** pin enable_ota_rollback and collapse _idf boards into ${framework_variant} (default esp-idf) ([a3d1755](https://github.com/c-iot-systems/esphome-sdk/commit/a3d17557a1a21eaffc1d899a3091337b42cedac8))
* **repo:** restore SDK-3 through SDK-6 changes (RECOVERY) ([9d72e00](https://github.com/c-iot-systems/esphome-sdk/commit/9d72e00523d59886c007c3e6e316d852e45311fb))
* **sdk:** enforce sdk_ref, schema-safe AP SSID, escaped AP password ([e6a0c5c](https://github.com/c-iot-systems/esphome-sdk/commit/e6a0c5c87fedd2efcee460d0baee6fa1afe48092))
* **wifi,core:** default reboot timeouts to 0s and restore infinite_* substitutions ([18b8a0c](https://github.com/c-iot-systems/esphome-sdk/commit/18b8a0c7dd7215723534780bc0f53030f6cd999b))


### Performance

* **ci:** compile each validate fixture in its own job, ~52 min serial to ~5 min ([ad07d59](https://github.com/c-iot-systems/esphome-sdk/commit/ad07d5925bbb798f4d4bb3890fb80507e9b7063e))


### Refactors

* **ci:** factor SDK-ref materialization into a self-testing gate script ([e0e2f16](https://github.com/c-iot-systems/esphome-sdk/commit/e0e2f16686cfefc831d31db62e0bc94937972581))
* **hardware:** gate hds_v1_1 SW1 behind a compile-time flag, one file per board ([14e6187](https://github.com/c-iot-systems/esphome-sdk/commit/14e618754329ea492b9caa6e7607c8edb276bf19))


### Documentation

* **hardware:** state SW1/serial-console exclusivity on hds_v1_0 as a hardware constraint ([ceba105](https://github.com/c-iot-systems/esphome-sdk/commit/ceba105c9ad56a24d5317ace5ba5c28f3a1faa4f))
* **sdk:** adopt strict Conventional Commits with the Jira key in the subject tail ([0094a57](https://github.com/c-iot-systems/esphome-sdk/commit/0094a57cf404de662f0cbd2fce0af8a026b1804a))
* **sdk:** document the release process, the bump mapping and the gate index ([7dd8fdb](https://github.com/c-iot-systems/esphome-sdk/commit/7dd8fdb16604975fd457777b704c7d7d702640a0))
* **sdk:** scrub internal references from the public repo and document slot/esp-idf/SW1 ([48bcef3](https://github.com/c-iot-systems/esphome-sdk/commit/48bcef3db7bcc6fb3ab64ba93b3a1b46f1152b11))
* **sdk:** scrub residual predecessor narration and CA-injection disclosure ([47cb8a1](https://github.com/c-iot-systems/esphome-sdk/commit/47cb8a1c6d5d5f128232dfb419f086aac79a0209))
* **sdk:** trim oversized YAML comments ([7ee7be8](https://github.com/c-iot-systems/esphome-sdk/commit/7ee7be8236506a31fb4c241a58e35d6e12532238))
* **tests:** restore fixture intent comments and pin-validation rationale ([0568aeb](https://github.com/c-iot-systems/esphome-sdk/commit/0568aeba4d690c647c34758fa4e4a791e9289a9a))


### Chores

* **release:** publish the first tag as v0.1.0 ([45e0160](https://github.com/c-iot-systems/esphome-sdk/commit/45e0160afa1b8ccafeda04fad34206f78463542e))
