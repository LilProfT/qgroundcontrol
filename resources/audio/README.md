An audio (wave file) can be set as replacement for upstream's Text-To-Speak feature.

Here comes steps to do it (scripting is hard and slow because `qgroundcontrol.qrc` is a big XML) :

1. Place the **wave file** (`.wav`) here.  
   Ex:
   ```
   cp path/to/communication_lost.wav .
   ```
2. To map a **line of text** to the **wave file**, add a key-value pair to `audioMap.json`.  
   Ex:
   ```
   "communication lost": "communication_lost.wav",
   ```
3. Add the **wave file** as a **resource** to `qgroundcontrol.qrc`.
   Ex:
   ```
   <qresource prefix="/audio">
        <file alias="communication_lost.wav">resources/audio/communcation_lost.wav</file>
   </qresource>
   ```
4. Update custom qrc:  
   ```
   cd custom
   python2 updateqrc.py
   ```
5. Rebuild project as usual.
