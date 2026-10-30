# Kollektivtransport (Entur)

En rute som viser når neste buss, trikk, tog, T-bane eller ferge går fra en holdeplass du følger med på, hvor som
helst i Norge. Den leser en sensor du allerede har fra Home Assistants egen
[Entur-integrasjon](https://www.home-assistant.io/integrations/entur_public_transport); denne pluginen spør aldri
Entur selv, og krever derfor ingen konto, nøkkel eller ekstra tillatelse.

Ruten viser linjen og hvor den går, hvor mange minutter til den går (som telles ned på skjermens egen klokke mellom
to oppdateringer), og avgangen etter den under. En forsinkelse på ett minutt eller mer vises som «+2» bak hvor den
går.

## Slik setter du det opp

### 1. Legg til Entur-integrasjonen i Home Assistant, hvis du ikke allerede har den

Hopp over dette hvis du allerede har `sensor.entur_...`-enheter.

1. Finn ID-en til holdeplassen. Åpne [stoppested.entur.org](https://stoppested.entur.org), søk opp holdeplassen din,
   og kopier ID-en den viser (`NSR:StopPlace:548` for hele holdeplassen, `NSR:Quay:70500` for én enkelt plattform av
   den, hvis du bare vil ha siden av veien du går fra).
2. Legg den til i Home Assistants `configuration.yaml`:
   ```yaml
   sensor:
     - platform: entur_public_transport
       stop_ids:
         - NSR:StopPlace:548
       name: Entur
   ```
   List opp hver holdeplass du vil ha en rute for under `stop_ids`, én per linje; hver av dem blir sin egen sensor.
   Vil du bare ha enkelte linjer fra en stor holdeplass? Integrasjonens egen innstilling `line_whitelist` gjør det (se
   [dokumentasjonen](https://www.home-assistant.io/integrations/entur_public_transport) for alle innstillingene).
3. Start Home Assistant på nytt. Én sensor dukker opp per holdeplass, navngitt etter den (`sensor.entur_<holdeplass>`).

### 2. Legg til pluginen og en rute

1. Legg til denne pluginen på en skjerm i Tessera (Plugins, eller skjermens Plugins-fane). Skjermen bygges én gang.
2. I Layout, plasser ruten **Neste avgang** fra bibliotekets Plugins-gruppe.
3. I inspektøren velger du sensoren for holdeplassen du vil ha, under feltet for enhet. Tessera viser alle sensorer
   som ser ut som en Entur-avgangssensor, så dine egne er lette å finne selv på et stort anlegg.
4. Legg til én rute til per holdeplass eller plattform du vil følge med på; hver velger sin egen sensor.
5. Sett **Gangtid til holdeplassen** til minuttene du trenger for å komme deg dit; en avgang du ikke når lenger, blir
   hoppet over til fordel for den etter. Slå av **Vis forsinkelser** hvis du heller ikke vil se dem.

## Godt å vite

- Ruten viser bare det sensoren allerede har; den spør aldri Entur selv. Den følger Home Assistants egne
  oppdateringer av sensoren, omtrent hvert 45. sekund, og teller minuttene videre ned på skjermens egen klokke i
  mellomtiden, så den kan ligge litt bak like etter at noe har endret seg.
- Bare den neste avgangen og den etter den har en linje og et mål ruten kan vise; avganger lenger fram som
  integrasjonens innstilling `number_of_departures` legger til, kommer som ren tekst ruten ikke leser ennå.
- En avgang du ikke når lenger (gjennom **Gangtid til holdeplassen**) blir hoppet over; når ingen er igjen, sier
  ruten det.
- Virker på alle typer skjermer, også CYD.

## Hvordan det virker

- `tessera-plugin.yaml`: ruten, som hører til en `sensor`-enhet (Home Assistants egen Entur-sensor), og attributtene
  den leser fra den.
- `components/entur_departures/`: ESPHome-komponenten som tegner ruten og teller ned.
- `translations/`: ordene på skjermen (`screen`) og i appen (`app`); engelsk og norsk (bokmål).
