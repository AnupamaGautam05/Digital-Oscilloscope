# Digital-Oscilloscope
<pre>
The DSO screen has total size of 128 x 64 pixels, which is divided into two sections:
 * 
 * [ I ] DISPLAY WAVEFORM => Area where the input waveform is to be displayed (may it be Sine, Square, etc.)
 * 							 The center line is a reference line about which the waveform should be symmetric.
 * 							 The height of this area should accomodate the waveform with max. Vpp of 5V.
 * 							 The width of this area should accomodate the waveform with:
 * 							* min. frequency of 250 Hz (0.25 kHz)
 * 							* max. frequency of 2000 Hz (2 kHz)
 * 
 * [ II ] DISPLAY WAVEFORM INFO => Area where the information about the waveform is to be displayed.
 * 								   The peak-to-peak voltage (Vpp), frequency in kHz, ms/div, V/div.
 * 
 * 
    ____________________________________________________________
 * |(0,0)                                   |                   |
 * |                                        |                   |
 * |                                        |                   |
 * |          DISPLAY WAVEFORM              |                   |
 * |                                        |                   |
 * |                                        |     DISPLAY       |
 * |                                        |                   |
 * |________________________________________|                   |
 * |                                        |     WAVEFORM      |
 * |                                        |                   |
 * |                                        |                   |
 * |          DISPLAY WAVEFORM              |       INFO        |
 * |                                        |                   |
 * |                                        |                   |
 * |                                        |                   |
 * |_________________________________(95,63)|(96,63)____(127,63)|
   </pre>
