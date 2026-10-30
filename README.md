# Public transport (Entur)

A tile that shows when the next bus, tram, train, metro or ferry leaves a stop you watch, anywhere in Norway. It
reads a sensor you already have from Home Assistant's own
[Entur integration](https://www.home-assistant.io/integrations/entur_public_transport); this plugin asks nothing of
Entur itself, so it needs no account, no key and no extra permission.

The tile shows the line and where it goes, how many minutes until it leaves (ticking down on the screen's own clock
between two updates), and the departure after that below it. A delay of a minute or more shows as "+2" behind where
it goes.

## Set up

### 1. Add the Entur integration to Home Assistant, if you have not already

Skip this if you already have `sensor.entur_...` entities.

1. Find the stop's id. Open [stoppested.entur.org](https://stoppested.entur.org), search your stop by name, and copy
   the id it shows (`NSR:StopPlace:548` for the whole stop, `NSR:Quay:70500` for one platform of it, if you want only
   the side of the street you leave from).
2. Add it to Home Assistant's `configuration.yaml`:
   ```yaml
   sensor:
     - platform: entur_public_transport
       stop_ids:
         - NSR:StopPlace:548
       name: Entur
   ```
   List every stop you want a tile for under `stop_ids`, each on its own line; every one becomes its own sensor.
   Want only certain lines from a busy stop? The integration's own `line_whitelist` option does that (see its
   [documentation](https://www.home-assistant.io/integrations/entur_public_transport) for the full set of options).
3. Restart Home Assistant. One sensor appears per stop, named after it (`sensor.entur_<stop name>`).

### 2. Add the plugin and a tile

1. Add this plugin to a screen in Tessera (Plugins, or the Plugins tab of the screen). The screen builds once.
2. In Layout, place the **Next departure** tile from the library's Plugins group.
3. In the inspector, choose the sensor for the stop you want under its entity field. Tessera offers every sensor that
   looks like an Entur departure sensor, so yours are easy to pick out even on a large installation.
4. Add one more tile per stop or platform you want to watch; each picks its own sensor.
5. Set **Walk to the stop** to the minutes you need to get there; a departure you can no longer make is skipped in
   favour of the one after it. Turn **Show delays** off if you would rather not see them.

## Good to know

- The tile only draws what the sensor already has; it never asks Entur itself. It follows Home Assistant's own
  updates of the sensor, about every 45 seconds, and counts the minutes down further on the screen's own clock in
  between, so it may be briefly behind right after something changes.
- Only the next departure and the one after it have a line and a destination the tile can show; departures further
  out that the integration's `number_of_departures` option adds come as plain text it does not read yet.
- A departure you can no longer make (through **Walk to the stop**) is skipped; when none are left, the tile says so.
- Works on every board, the CYD included.

## How it works

- `tessera-plugin.yaml`: the tile, which belongs to a `sensor` entity (Home Assistant's own Entur sensor), and the
  attributes it reads from it.
- `components/entur_departures/`: the ESPHome component that draws the tile and counts down.
- `translations/`: the words on the screen (`screen`) and in the app (`app`); English and Norwegian (Bokmål).
