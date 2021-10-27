# Passive Melody Buzzer
Bevor es mit der Deutschen Beschreibung weitergeht, erst mal eine Kurzzusammenfassung für unsere Englischsprachigen Freunde.

## Summary
Sorry, English readme is still in the making. (I would appreciate if some native speaker that knows a bit about scores could volunteer :wink:).
In a nutshell, this library allows to play melodies using a passive buzzer connected to a digital output pin. 
Aim was to allow playback in non-blocking mode and have a simple yet powerful way to write down a tune in ASCII.
As a very basic example the String **"CDEFGABc"** will play a C-Major-scale.
Nonblocking playback is achieved by using a timer interrupt to generate the sound. That allows for a high precision in pitch and sound duration. However, this also gives a portability issue: currently it is Espressif ESP32 only.

Tested with Arduino and PlatformIO. 
Example "BuzzerSerial" is in Arduino and allows to test the functionality by playing melodies entered on the Serial monitor input. Source code is commented, so you will get the basic API.

At the bottom of the sketch file is a number of examples for explaining the notation "language", starting easy with some scale examples but also include more sophisticated examples of melodies. 
If you are able to read scores and are somewhat familar with the notation language, you should be able to transcribe something like the snippets included ("Raiders March" or "Axel F" for instance) within 5 minutes.
Go try it, and let me know if you came up with a beautiful melody!

## Zusammenfassung
Mit dieser Library ist es möglich, Melodien über eine am Controller angeschlossenen Passiven Buzzer zu spielen.
Zwei wesentliche Entwicklungsziele gab es:
- Nichtblockierendes Abspielen soll möglich sein.
- Melodien sollen zur Laufzeit über eine möglichst einfache aber trotzdem leistungsfähige "Notensprache" in ASCII beschrieben werden können. 

Das hat geklappt, allerdings bedeutet das, dass man wenigstens grundlegendes Verständnis von Noten haben muss, um erfolgreich eigene Melodien zu schreiben (ein paar mehr oder weniger bekannte Melodien werden allerdings im folgenden beschrieben und können z. B. im Repository enthaltenen Arduino-Sketch *SerialBuzzer.ino* über die Serielle Schnittstelle ausprobiert werden).
Die Sprache ist natürlich gewöhnungsbedürftig zu lesen, ist aber halbwegs intuitiv in der Anwendung beim Transkribieren. Auch die komplizierteren Beispiele unten (wie "Thais Meditation") sind nach etwas Eingewöhnung in 5 Minuten geschrieben.
Dadurch, dass die Melodie als String spezifiziert werden kann, kann die Schnittstelle leicht in verschiedene Anwendungen integriert werden. Im Beispiel weiter unten ist die Eingabe per serieller Schnittstelle möglich. Das läßt sich leicht adaptieren auf andere Schnittstellen, wie BT-Serial oder MQTT oder oder oder...

Das nichtblockierende Playback wird erreicht, indem die Tongenerierung über einen Timer-Interrupt erfolgt. Das bewirkt eine hohe Genauigkeit in der Tonhöhe und Dauer, kommt aber auf Kosten der Portabilität: momentan ist die Umsetzung auf ESP32 beschränkt.

## Beschreibung des Beispiels *BuzzerSerial.ino*
Anhand dieses Abschnitts soll die wesentliche API zur Anwendung der Bibliothek erläutert werden.
Der Sketch findet sich im Verzeichnis **/examples** dieses Repositories.

Die Bibliothek muss wie gewohnt in das Arduino-Verzeichnis **/libraries** kopiert werden. 
Dann sollte sich *BuzzerSerial* auch über das Arduino-Menü **File/Examples/PassiveMelodyBuffer/BuzzerSerial** laden lassen (im Zweifelsfall sind in diesem Untermenü viele Beispiele aus anderen Bibliotheken zu finden, einfach mal suchen).

Im Beispielsketch ist nur eine Einstellung vorzunehmen, das ist der Pin, an den der Buzzer angeschlossen ist:

```
#define BUZZER_PIN 21        
```

Hier die 21 ändern, falls der Buzzer nicht an diesen Pin angeschlossen ist (die 21 passt für einen AZ-Touch mod).

Für das Aufspielen aus dem **Tools**-Menü das passende Board auswählen (es sollte für jedes beliebige ESP32-Modul compilieren).

Wenn alles korrekt funktioniert, sollten nach dem Aufspielen (bei jedem Reset) drei kurz aufeinanderfolgende Klicks und eine C-Dur-Tonleiter zu hören sein.

Schauen wir mal rein:

```
#include <Arduino.h>
#include <PassiveMelodyBuzzer.h>
/*
 * At the bottom of the sketch are a few examples of melodies to play (by pasting them to the serial monitor input).
 * At the time of writing, this library can only be used for ESP32.
 * A passive buzzer is required (active buzzers can play only one specific frequency).
 * The volume of the playback can not be changed by software setting.
 */

#define BUZZER_PIN 21                     // IO Pin the buzzer is connected to (AZ-Touch mod has it connected to gpio 21)

PassiveMelodyBuzzer buzzer(BUZZER_PIN, true);   // Create buzzer object. Second (optional) parameter specifies, if buzzer is HIGH active.
                                                // Defaults to "true". If you do not hear three clicks but the C major scale, try "false".
```
Durch diesen Abschnitt wird:
- die Bibliothek eingebunden (`#include <PassiveMelodyBuffer.h>`)
- der Pin als Konstante definiert (`#define BUZZER_PIN 21`) 
- das Buzzer-Objekt erstellt (`PassiveMelodyBuzzer buzzer(BUZZER_PIN, true);`). Der erste Parameter ist die definierte Pin-Nummer. Der zweite Parameter ist optional (wird als *true* angenommen, falls weggelassen) und legt fest, ob der Buzzer *HIGH*- oder *LOW*-aktiv ist. Pragmatischer Ansatz: wenn sowohl die angesprochenen Klicks als auch die Tonleiter zu hören sind, ist der Parameter OK. Falls die Klicks fehlen und nur die Tonleiter zu Anfang kommt, sollte der Parameter inverviert werden (*false* statt *true*). Falls gar nichts kommt, ist es Zeit, den Aufbau :electric_plug: zu prüfen...

Weiter mit *setup()*:

```
void setup() {
  Serial.begin(115200);                   
  for(int i = 0; i < 3; i++)              // For 3 times 
  {
    buzzer.click();                           // generate "click" sound
    while (buzzer.busy()) ;                   // wait for the click sound to finish
    delay(300);                               // wait another 300 msec
  }
  buzzer.playMelody("C D E F G A B c");   // Start playing C major scale. Does not block here...
}
```

Die relevanten Anweisungen hier sind:
- `buzzer.click();`: dadurch wird ein "Klick"-Geräusch erzeugt. Die Funktion wartet nicht, bis der "Klick" vollständig abgearbeitet ist, sondern kehrt sofort zurück.
- `while (buzzer.busy()) ;`: Wartet, bis der "Klick"-Sound beendet ist. `buzzer.busy())` liefert *true*, solange der letzte sounderzeugende Befehl (hier `buzzer.click()`) noch nicht vollständig abgearbeitet ist.
- `buzzer.playMelody("C D E F G A B c");` startet schließlich eine "Melodie", in dem Fall eine C-Dur-Tonleiter. Wie bei `buzzer.click()` startet dieser Aufruf die Melodie, kehrt aber unmittelbar zurück und wartet nicht, bis die Melodie fertig abgespielt ist.

Mehr Details zu den Tönen und wie man daraus "richtige" Melodien macht kommt später. Jetzt nur soviel:
- es gibt 14 Grundtöne, jeweils dargestellt durch einen Groß- oder Kleinbuchstaben, von tief zu hoch sind das
  **C D E F G A B c d e f g a b**.
- es werden die im Englischen üblichen Namen verwendet. Daher entspricht **Bb** in dieser Notation dem im deutschen
  Gebrauch üblichen Notennamen **h**.
- der Ton **a** ist auf 440Hz festgelegt, der Rest leitet sich daraus aus der gebräuchlichen chromatischen Stimmung ab.

Jetzt noch *loop()*
```
void loop() {
  buzzer.loop();                          // Keep on playing 
  String readLine;
  if (serialReadLine(readLine))           // A line from Serial available?
  {
    if (buzzer.busy())                       // if the buzzer is still playing the "old" melody, stop playback
      buzzer.stop();                         // Note that this is for demonstration only, starting a new melody with the line
                                             // below would cancel playback anyways...
    buzzer.playMelody(readLine.c_str(), 1);  // Feed it to the buzzer, to treat it as Melody
                                             // Second parameter will force some debug output on Serial
  }
}
```

Die "wichtigste" Zeile hier ist der Aufruf `buzzer.loop()`, der die Melodie weiterführt. Falls dieser Aufruf fehlen würde, würde der erste Ton der Melodie nicht beendet werden. Dieser Aufruf muss so oft wie möglich zyklisch erfolgen, ansonsten kann es sein, dass einige/alle Töne der Melodie ungewollt länger erklingen.
`buzzer.loop();` ist übrigens wirkungsgleich mit `buzzer.busy();`. Liest sich nur bequemer, ansonsten könnte auch in der Zeile `buzzer.busy();` stehen.

Die Bedingung `if (serialReadLine(readLine))` ist dann erfüllt, falls im Seriellen Monitor eine Zeile eingegeben wurde. Idealerweise ist das eine gültige Melodie, diese wird dann durch den Aufruf `buzzer.playMelody(readLine, 1);` gestartet. (Der zweite Parameter **1** ist optional und sorgt nur für etwas Ausgaben auf dem seriellen Monitor).

Vorher wird noch durch die Abfrage `if (buzzer.busy())` überprüft, ob aktuell noch eine Melodie (von der vorherigen Eingabe) abgespielt wird. Diese würde dann mit der Anweisung `buzzer.stop();` beendet. 
Diese Abfrage ist hier nur zu Demozwecken enthalten und eigentlich überflüssig: jeder Aufruf von `buzzer.playMelody()` würde eine eventuell noch laufende Melodie abbrechen, bevor die neue Melodie startet.

## Notation von Melodien
### Grundsätzlich
Jede Melodie besteht aus einer Reihe von Tönen (und Pausen als Spezialton). Töne können unterschiedlich lang sein und verschiedene Tonhöhen haben. (Eigentlich auch Variationen in der Lautstärke, dass geht aber konzeptbedingt mit den passiven Buzzern nicht).

Momentan sind nur die Grundtöne (von **C..g**) bekannt, die auch immer in einer festen Länge (500msec) ertönen.

Dies läßt sich ändern. Dabei können einzelne Parameter (z. B. die Länge) individuell für jede einzelne Note als auch für die gesamte Melodiefolge geändert werden (so dass z. B. jeder Ton eine andere vordefinierte Grundlänge als 500msec hat). 

Im folgenden werden beide Varianten beschrieben.

Die Erläuterungen lassen sich am besten nachvollziehen, indem sie (nach Laden des Beispiels **Seriam angeschlossenen seriellen Monitor nachvollzogen werden. Einige der nun folgenden Beispielsequenzen sind auch direkt im Sketch mit (am Ende als Kommentar) enthalten.
Falls mal was schief geht, und irritierende Klänge kommen, kann die Ausgabe der Töne durch Eingabe einer Leerzeile am seriellen Monitor gestoppt werden.

Grundsätzlich ist der Parser für die Notation robust ausgelegt: wenn Zeichen auftauchen, die es laut Notationsvorschrift nicht geben dürfte (den Notennamen 'Z' z. B.) werden diese Zeichen ignoriert (übersprungen).

Zwischen Notenname können beliebig viele Leerzeichen stehen, das erhöht die Lesbarkeit beim Niederschreiben, sollte aber vermieden werden, wenn die Melodie z. B. im Programmspeicher abgelegt werden soll. Musikalisch haben diese Leerzeichen keine Relevanz. Beispiel: die beiden folgenden Zeilen führen zu einer identischen Tonausgabe:

```
C          D E FGA      Bc
CDEFGABc
```

### Modifikationen einzelner Töne
Zunächst fällt auf, dass die Töne ziemlich direkt ineinander übergehen (quasi legato). Falls man möchte, dass die Noten abgesetzt erklingen, kann an die Note ein **,** oder ein **.** angehängt werden. Dass sorgt dafür, dass die entsprechende Noten ein wenig (mit Komma) oder stark (durch Punkt) verkürzt wird. Zwischen Note und **,** bzw. **.** darf kein Leerzeichen eingefügt werden!

Durch die folgende Zeile wird drei mal die C-Dur-Tonleiter gespielt, zunächst ohne, dann mit geringer und dann mit starker Kürzung der einzelnen Noten. Der Unterschied sollte hörbar (und auf dem seriellen Monitor auch sichtbar) sein.
```
C D E F G A B c      C, D, E, F, G, A, B, c,      C. D. E. F. G. A. B. c.
```

Wichtig zum Verständnis ist, dass dabei der Schlag (Beat) nicht geändert wird. Jede Note startet weiterhin 500ms nach der vorhergehenden. Die eingefügte Pause wird von der klingenden Länge abgezogen.


Richtige Pausen sind in der Musik wichtig, daher gibt es sie hier auch als "Note" mit dem Namen **r** oder **R** (von Englisch **rest**). Der Unterschied zwischen Groß- und Kleinschreibung liegt hier nicht in der Tonhöhe, sondern in der Länge: **r** ist genau so lang wie jede Note auch (500 ms), **R** ist doppelt so lang (1 sec).

Hier nun die schon bekannte Tonleiter mit (verschieden langen) Pausen:
```
C r D R E r F R F r A R B r c
```

Nun können wir uns an die Notenlängen wagen. Bisher hat jede Note die exakt gleiche Länge von 500 msec. Diese kann verändert werden, wenn hinter die Note ein Verlängerungs- oder Verkürzungsfaktor angegeben wird:
- eine Verlängerung wird eingeleitet durch ein **\*** direkt (ohne Leerzeichen) hinter dem Notennamen wiederum direkt gefolgt von einer positiven Ganzzahl (>0, ohne Vorzeichen). Die zugehörige Note wird auf die gültige Standardlänge (bisher 500msec) multipliziert mit der angegebene Ganzzahl verlängert.
- eine Verkürzung wird eingeleitet durch ein **/** direkt (ohne Leerzeichen) hinter dem Notennamen wiederum direkt gefolgt von einer positiven Ganzzahl (>0, ohne Vorzeichen). Die zugehörige Note wird auf die bisher gültige Standardlänge (500ms) dividiert durch um die angegebene Ganzzahl verkürzt.
- beides kann kombiniert werden: **C\*3/4**, wodurch die Note auf 375ms (500 * 3 / 4) der ursprünglichen Länge verkürzt wird.
- bei einer kombinierten Angabe ist es egal, ob Verlängerung oder Verkürzung zuerst angegeben werden. **C/4\*3** ist identisch zum vorherigen Beispiel. Aus musikalischem Verständnis heraus ist diese Variante ggf. einprägsamer. Wenn man sich Standardschlag von 120 bpm (Schlägen pro Minute, entspricht einer Notenlänge von 500ms) z. B. als Viertel-Note vorstellt, wird daraus zunächst durch **/4** eine 16-tel Note, deren Länge multipliziert mit 3 einer punktierten Achtel entspricht.

Hier mal eine C-Dur-Tonleiter mit unterschiedlichen Notenlängen.
```
C/2 D/2 E/2 F G*2/3 A*2/3 B*2/3 c*2
```
Wenn man den Grundschlag wie eben als Viertel annimmt, sind damit die ersten 3 Töne (halbiert) Achtel, gefolgt von einer Viertel-Note (ohne Veränderung), gefolgt von drei Triolen-Vierteln (**\*2** ergibt eine halbe, die durch **/3** wiederum auf ein drittel davon verkürzt wird) gefolgt von einer halben Note als Abschlußton (**\*2**).

Nach der Tonlänge kümmern wir uns nun um die Tonhöhe. Zunächst mit Blick auf den darstellbaren Tonumfang. Bisher kennen wir die die 14 Grundtöne (in Reihenfolge der Tonhöhe) **C D E...** bis **...e f g**. Damit kann man schon was anfangen, aber nicht alles machen. 
Dazu läßt sich der Tonumfang durch Okavierung erweitern. Oktaviert (erhöht) werden können alle Noten mit Kleinbuchstaben, indem hinter die Note die Ziffer **1** (für eine Oktave höher) bis **4** (4 Oktaven höher) angehängt wird (wie üblich kein Leerzeichen zwischen Notennamen und der Ziffer!). Damit ist der volle Tonumfang nun (anhören auf eigene Gefahr, das zeigt physikalische Grenzen der Buzzer auf):
```
CDEFGAB cdefgab c1 d1e1f1g1a1b1 c2d2e2f2g2a2b2 c3d3e3f3g3a3b3 c4d4e4f4g4a4b4
```

Damit gibt es nun drei (optionale) Suffixe, die Einfluss auf den Ton haben:
- Oktavierung (bei Noten mit Kleinbuchstaben) durch angehängte Ziffern **1..4**
- Verkürzung und/oder Verlängerung der Note durch mit **\*** und/oder **/** angehängte Verkürzungs-/Verlängerungsfaktoren
- Verkürzung der Note durch angehängte **.** oder **,**

Jede Note kann keinen, einen, zwei oder alle drei dieser Suffixe haben. Falls mehr als einer verwendet wird, muss die eben angegebene Reihenfolge eingehalten werden! Grundsätzlich darf sich in dem gesamten Konstrukt aus Notennamen und angehängten Suffixen nie ein Leerzeichen befinden, das muss immer alles kompakt zusammen geschrieben werden. Im folgenden nun mal wieder unsere schon legendäre C-Dur-Tonleiter, dieses Mal zur Abwechslung nach oben oktaviert (und mit unterschiedlichen Tonlängen, um die korrekte Notationsreihenfolge noch mal zu verdeutlichen).

```
c1/2, d1/2, e1/2, f1/2, g1/2, a1/2, b1/2, c2/2*5
```

Pausen können ebenfalls durch Faktoren verlängert oder verkürzt werden. In diesem Fall geht die Verlängerung und Verkürzung unabhängig von Groß oder Kleinschreibung (**R** vs. **r**) immer von identischer Grundlänge aus (bei Standard 120 bpm also 500ms). Siehe folgendes Beispiel (Oktavierungen von Pausen und Verkürzungen durch angehängte **,** oder **.** werden toleriert aber ignoriert, haben also keinerlei musikalischen Effekt):

```
c1 d1 e1 f1 g1 r a1 R*4/2. b1 r1*2/4, c2*2
```
Damit endlich mal was anderes kommt als die C-Dur-Tonleiter, ein letztes Element zur Modifikation der Tonhöhe: Vorzeichen:
- Vorzeichen müssen direkt vor dem Notennamen angegeben werden, ohne Leerzeichen oder sonstiges dazwischen.
- ein **#** erhöht die folgende Note um einen Halbton (**#f** beispielsweise klingt als *fis*)
- ein **~** erniedrigt die folgende Note um einen Halbton (**~e** beispielsweise klingt als *es*)
- für jeden der Grundtöne sind beide Vorzeichen definiert, wobei es zu enharmonischen Verwechslungen kommen kann, so ist z. B.  **~a** (as) identisch mit **#g** (gis) oder **#e** (e-is) identisch mit **f**.

Damit jetzt eine G-Dur-Tonleiter (mit **fis**), nach einer Pause gefolgt von einer F-Dur-Tonleiter
```
g a b c1 d1 e1 #f1 g1*2 R f g a ~b c1 d1 e1 f1*2
```

Damit sind alle Möglichkeiten, eine einzelne Note in Tonlänge und Tonhöhe zu verändern, besprochen. Es folgt die Beschreibung der Möglichkeiten, die Parameter Geschwindigkeit und Tonhöhe einer (Teil-) Melodie insgesamt zu ändern.

### "Permanente" Modifikation von Tonhöhe und Geschwindigkeit
Das erste, was dringend zu ändern wäre, ist die Festlegung auf den Grundschlag von 120 Schlägen pro Minute. Das ist ganz einfach gemacht, indem die Zeichensequenz **!=** direkt gefolgt von einer positiven (Ganzzahl) größer 0 und kleiner/gleich 6000 angegeben wird. Diese Ganzzahl (wie immer ohne Leerzeichen zwischen **!=** und Zahl) definiert die ab jetzt für alle folgenden Noten der Melodie geltende Schlagzahl. Standard ist 120 Noten pro Minute (bpm), 240 ist also doppelt so schnell, wie man an folgendem Beispiel hört, wo die gleiche recht bekannte Sequenz nur mit anderer Geschwindigkeit wiederholt wird.

```
cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4, r !=240 cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4,

```

Prinzipiell kann die Geschwindigkeit damit beliebig oft verändert werden. Diese absolute Setzung empfiehlt sich jedoch wenn möglich nur einmalig am Anfang zu machen, danach sollten ebenfalls mögliche relative Geschwindigkeitsänderungen genutzt werden. Diese werden analog wie für die einzelnen Noten angegeben spezifiziert:
- **!\*** direkt gefolgt von einer Ganzzahl > 1 verlängert alle folgenden Noten um den angegebenen Faktor. **!\*2** verdoppelt also die Länge (und halbiert somit effektiv den Grundschlag) für alle folgenden Noten
- **!/** direkt gefolgt von einer Ganzzahl > 1 verkürzt die Länge aller folgenden Noten um den angegebenen Faktor **!/2** halbiert also die Länge (und verdoppelt somit effektiv den Grundschlag) für alle folgenden Noten
- der Grundschlag ist dabei der durch die letzte **!=**-Sequenz gesetzte Schlag (bzw. der Standard von 120 bpm, falls keine individuelle Einstellung erfolgte).
- wie bei den individuellen Verkürzungen/Verlängerungen für einzelne Noten vorher, können auch hier beide Varianten zur Darstellung nicht-ganzzahliger Faktoren verwendet werden. **!/4*3** verkürzt alle nachfolgden Noten auf 75% der ursprünglichen Länge (und vergrößert somit effektiv den Grundschlag auf 4 Drittel des bisherigen Wertes)
- Für die einzelnen Noten gilt der so eingestellte Grundschlag, individuelle Verkürzungen/Verlängerungen der Noten bleiben natürlich möglich, und beziehen sich dabei auf den aktuell gültigen Grundschlag.
- Die Änderung des Grundschlags gilt immer bis zur nächsten Änderung des Grundschlags, maximal bis zum Ende der Melodie. (Jede Melodie startet immer mit dem Grundschlag von 120 bpm)
- Relative Änderungen des Grundschlags können zurückgenommen werden durch ein einzelnes **!**. Dadurch wird der Grundschlag wieder auf den durch die letzte **!=**-Sequenz gesetzten Schlag (bzw. den Standard von 120 bpm, falls keine individuelle Einstellung erfolgte) gesetzt


In den folgenden zwei Zeilen ist die erste Zeile eine Kopie des letzten Beispiels. Die zweite Zeile ist musikalisch identisch, äquivalent zum eben gehörten, allerdings mit relativer Geschwindigkeitsänderung (**!/2**)

Die dritte Zeile ist identisch mit der vorhergehenden, allerdings wurde hier schon zu Beginn eine etwas höhere Grundgeschwindigkeit (140 bpm) eingestellt. Der zweite Teil wird dann weiter mit doppelter Geschwindigkeit, resultierend also 280 bpm abgespielt.

Wer hart im Nehmen ist, kann auch die vierte Zeile spielen. Das ist wie zuvor, nur wird die Sequenz ein drittes Mal gespielt, wobei für die dritte Wiederholung der Schlag durch das **!** wieder auf 140 bpm zurückgesetzt wird.

```
cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4, r !=240 cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4,
cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4, r !/2 cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4,
!=140 cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4, r !/2 cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4,
!=140 cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4, r !/2 cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4, r ! cdef g*2,g*2, a.a.a.a. g*4, a.a.a.a. g*4,
```

Neben der "globalen" Änderung der Tondauer kann auch die Tonhöhe (durch Oktavierung) auf globaler Ebene verschoben werden.
- ein **+1** bis **+4** erhöht alle nachfolgenden Töne um jeweils 1 bis 4 Oktaven bezogen auf den ursprünglichen Notenwert.
- ein **-1** bis **-4** verringert die Höhe aller nachfolgenden Töne jeweils um 1 bis 4 Oktaven.
- durch **+** oder **-** ohne nachfolgende Ziffer wird eine eventuelle Oktavierung wieder aufgehoben (ebenso durch **+0** oder **-0**)
- das ist zusätzlich zu den individuellen Oktavierungen der einzelnen Töne. **+1 c2** ergibt also klingend **c4**
- Aus technischen Gründen ist Addition (Erhöhung) dabei begrenzt auf 4 Oktaven über dem Grundton: **+1 c4 +4 c2 +4 c3** ergibt also nicht **c5 c6 c7** sondern tatsächlich **c4 c4 c4**. 

Mit Hilfe des Einsatzes der Oktavierung lässt sich oft das Notenbild kompakter schreiben, da die individuelle Oktavierung von Tönen wegfällt. Folgende zwei Zeilen spielen jeweils eine identische C-Dur-Tonleiter über 2 Oktaven.

```
!=240 c1d1e1f1g1a1b1c2d2e2f2g2a2b2c3
!=240 +2CDEFGABcdefgabc1
```
Durch **-4** kommt man 4 Oktaven unter den bisher tiefsten Ton. In Bereiche, wo es eher Geräusch als Ton ist:
```
-4 CDEFGABcdefgabc1
```



Jetzt haben wir uns aber was zur Entspannung verdient. Die folgenden Beispiele sind mit den bisher beschriebenen Mitteln vollständig darstellbar (die senkrechten Striche ggf. gefolgt von Leerzeichen sind nur zur Lesbarkeit eingefügt und sollen Taktstriche verdeutlichen. Vom Parser werden sie, ebenso wie die Leerzeichen, als ungültig ignoriert und haben keine musikalische Bedeutung). 

Zuerst was klassisches: Der Anfang von Thais Meditation von Jules Massenet
```
 !=88 | !/2 #f*5 d !/3 A d #f | ! b*2 #c1 d1 |  !/2 d*3 e !/5 #fg#fe#f !/2 a   A  |6 ! B*3, !/2 #cd, | #fe ! g*2, !/2 #de, | #fg, a, b, ! b B, | #c*2 d !/4 ed#cd | ! e*2 f*2 | #f*3, 
```
80er Jahre Italo-Pop: Das Intro aus "Vamos a la playa" von Righeira
```
 !=600ar+1arar+0ar!*2ar!a1r!*2ar+1!erdrcrdr+0ar!*2ar!ar+1arar+0ar!*2ar!a1r!*2ar+1!erdrcr!*3e.!r!*3c.!r+!*3ar
```

Und noch mal 80er Jahre Filmmusik/Pop: "Axel F" von Harold Faltermeyer
```
 !=130 f. !/4 ~a*3.f*2,f !/2 ~b,f,~e, | ! f. !/4  c1*3.f*2,f !/2 ~d1,c1,~a | fc1f1 !/4 f~e*2.~e, !/2 c, g, f*4,
```

Oder ganz "klassische Filmmusik": Der "Raiders March" von John Williams aus Indiana Jones 
```
 !=125!/4e*3,f!g,c1*2,!/4d*3,e!f*3.!/4g*3,a!b,f1*2.!/4a*3,b!c1d1e1!/4e*3,f!gc1*2!/4d1*3,e1!f1*3,!/4g*3,ge1*4d1*3,ge1*4d1*3,ge1*4d1*3,g!+1e,d,c
```


### Von der Melodie zum Sound
Für die klassische Hausautomatisierungsanwendung braucht man auch mal Alarmsounds o. ä., die man sich mit folgenden Tricks/Ergänzungen zusammenbauen kann:

- die Geschwindigkeit kann auf bis zu 6000bpm erhöht werden, das ergibt schon bei Tonleitern nette Effekte:
```
!=3000+1CDEFGABc*4Rc*4BAGFEDCr
```

Dadurch verkürzt sich natürlich die Gesamtdauer. Um einen Sireneneffekt zu erzielen, müsste diese Sequenz mehrfach wiederholt werden. Um das wiederholte Aneinanderkopieren zu vermeiden (und wieder Speicherplatz zu sparen) gibt es eine Notationsmöglichkeit, eine Sequenz mehrfach zu wiederholen. Dazu muss **am Beginn** dieser Sequenz (führende Leerzeichen sind OK) ein Doppelpunkt unmittelbar (d. h. ohne Leerzeichen) gefolgt von einer Ganzzahl > 0 folgen.
Es wird immer die komplette Melodiefolge wiederholt.

Das folgende Beispiel bringt so die Tonfolge von Oben, nur noch schneller und 10 mal wiederholt (durch **r\*8** mit kurzer Pause zwischen den Wiederholungen).
```
:10 !=6000+1CDEFGABc*4Rc*4BAGFEDC r*8
```

Schon kleine Änderungen können ganz anders klingende Effekte erzeugen, hier z. B. wieder wie eben, aber die absteigende Tonleiter ist dies mal nicht dabei:
```
:10 !=6000+1CDEFGABc*4R
```

Töne können auch direkt durch Angabe ihrer Frequenz erzeugt werden. Dazu muss (anstatt eines Noten-Namens) das **@**-Zeichen direkt gefolgt von der Frequenz angegeben werden. Um Fließpunktzahlen zu vermeiden und trotzdem eine ausreichende Genauigkeit zu erzielen, wird die Frequenz als deci-Hz (10tel Hz) angegeben. **@4400** entspricht damit dem Kammerton **a** mit 440 Hz.

So erzeugte Töne können nicht individuell oktaviert oder mit Vorzeichen versehen werden. Sie können jedoch auf die übliche Weise in der Länge beinflusst werden:
```
a @4400 @4400. @4400/4,@4400/4,@4400/4,@4400/4,
```

Das erzeugt insgesamt sieben mal den Ton **a** allerdings mit variablen Längen.

Für die Tonerzeugung wird das vielleicht weniger gebraucht werden, allerdings lassen sich so andere Effekte darstellen: 
```
=1 @5 
```
Dadurch werden für eine Minute Klicks im Sekundenabstand erzeugt. Das könnte z. B. für einen "akustischen Countdown" stehen, wobei mit `buzzer.busy()` überprüft werden kann, ob der Countdown noch läuft (`buzzer.busy()` liefert dann **true**, ansonsten eben **false**, falls der Countdown abgelaufen ist).

So, das wars. Viel Spass beim Ausprobieren.