# pc8801

![pc8801](imgur.com image replace me!)

*A short description of the keyboard/project*

* Keyboard Maintainer: [Arei1126](https://github.com/Arei1126)
* Hardware Supported: *The PCBs, controllers supported*
* Hardware Availability: *Links to where you can find this hardware*

Make example for this keyboard (after setting up your build environment):

    make pc8801:default

Flashing example for this keyboard:

    make pc8801:default:flash

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

Enter the bootloader in 3 ways:

* **Bootmagic reset**: Hold down the key at (0,0) in the matrix (usually the top left key or Escape) and p*/lug in the keyboard
* **Physical reset button**: Briefly press the button on the back of the P/*CB - some may have pads you must short instead

Ψ Created a new keyboard called pc8801.
Ψ Build Command: qmk compile -kb pc8801 -km default.
Ψ Project Location: /home/yuuri/qmk_firmware/keyboards/pc8801,
Ψ {}Now update the config files to match the hardware!{}
