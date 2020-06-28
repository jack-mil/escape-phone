# Arduino Rotary Phone Counter

<p align=center>
    This project is part of an escape room at Race 2 Escape. <br/>
    Check them out at <em align=center><a href="https://www.race2escape.net/">https://www.race2escape.net/</a></em>
</p>

Using an Arduino Nano (ATmega328) to detect digits dialed on an old rotary phone. Players in the escape room must dial the correct numeric code to unlock the next puzzle.

The existing phone speaker is used to playback recorded dial tone and operator messages.
These recordings are read from an external SD card module and played with the SimpleSDAudio library by Lutz Lisseck. *See references*

> Dialing *`352-6041`* will write digital pin 5 HIGH, to be used on a relay or another part of the puzzle.

The cmd line tool `sox` can be used to convert the audio files (in .wav format), as per SimpleSDAudio recomendations. See their wiki for more info.

#### References
> *The SimpleSDAudio library for Arduino by Lutz Lisseck can be downloaded from [his wiki](https://www.hackerspace-ffm.de/wiki/index.php?title=SimpleSDAudio)*

> *Thanks to Alastair Aitchison and his [YouTube video](https://www.youtube.com/watch?v=siwq1FxvRrw) for helping to clean up the code*
