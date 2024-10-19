# Somfy Cover Group component for ESPHome

This is a custom component for ESPHome that allows you to control a group of Somfy covers with a single ESPHome device.

This work is based on [ruedli/SomfyMQTT](https://github.com/ruedli/SomfyMQTT) using an ESP8266 and CC1101 for RF transmission.

I followed the wiring scheme from [LSatan/SmartRC-CC1101-Driver-Lib](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib):

![Connecting CC1101 to ESP8266](img/Esp8266_CC1101.png)

I had to change [ruedli/SomfyMQTT](https://github.com/ruedli/SomfyMQTT)'s code to use pin 5 instead of 2 with this wiring for some reason (_I_don't_know_what_I'm_doing.gif_).

Based on all this work, I created this custom component for ESPHome that can be used with a configuration file like this:

```yaml
esphome:
  name: somfy_remote_control

esp8266:
  board: esp01_1m

...

external_components:
  - source:
      type: local
      path: my_components

somfy_cover_group:
  covers:
      - id: "office_big_roller_blind"
        name: "Office Big Roller Blind"
        remote_code: 0x123455
      - id: "office_small_roller_blind"
        name: "Office Small Roller Blind"
        remote_code: 0x123456
```

Just change the source to this repository if that's what you wanna use, or copy/clone this to your external components folder and tweak the `path` accordingly.

Then add as many `covers` as you want to the `somfy_cover_group` configuration, each with a unique `id`, `name` and `remote_code`. The `remote_code` identifies the remote control. It's like having multiple physical remote controllers in the same device.

Then, in Home Assistant you will have, for each cover you added:
- 🪟 The cover controls (to open ⬆️, close ⬇️ and stop ⏹️).
- 🔘 One switch (`PROG`) to synchronize the remote control with the Somfy cover. You need to set the Somfy cover in "pairing mode", and then use the `PROG` switch from Home Assistant in order to pair them together.
- 🔘 One switch (`FAVORITE`) mapped to the `My` button of the Somfy control, which essentially moves the cover to a "favorite" position that you have configured beforehand.

I used the 3D model from [ruedli/SomfyMQTT](https://github.com/ruedli/SomfyMQTT) to create a case for the ESP8266 and CC1101. I copied them to this repo for completion, but all the design is `ruedli`'s. You can find it in the `case` folder.

![Pic Case 1](img/Pic%20Case%201.JPG)
![Pic Case 2](img/Pic%20Case%202.JPG)
![Pic Case 3](img/Pic%20Case%203.JPG)

## Acknowledgements

- [LSatan/SmartRC-CC1101-Driver-Lib](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib) for all the work to connect ESP8266 and CC1101.
- [ruedli/SomfyMQTT](https://github.com/ruedli/SomfyMQTT) for the idea, the 3D model and sharing all this work.
- [Legion2/Somfy_Remote_Lib](https://github.com/Legion2/Somfy_Remote_Lib) for all the work to encapsulate the Somfy protocol.
