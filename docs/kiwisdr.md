# KiwiSDR (network HF receiver)

A KiwiSDR is a software defined receiver that covers 0 to 30 MHz and is reached over the internet or a local network. Many owners make theirs public
(list: <https://kiwisdr.com/public/>). OnAir uses one as a radio: **Network radios > Type KiwiSDR**, the receiver's address and port (default 8073),
the password if the owner set one, **Connect**. **Find KiwiSDR...** downloads the public list when you click it (never at startup) and adds a receiver
with one click. The password is kept in OnAir's settings (the same file as the rest of the list) and is not shown in a name, a log line or a command.

## What you get

The Kiwi's "IQ" mode: 12 kHz of spectrum (the passband is 3, 5 or 5.9 kHz either side of the tuned frequency) around the frequency you set, with the
Kiwi's own AGC or a manual gain (the gain slider, 0 to 120 dB in the Kiwi's scale). The S-meter and an ADC overload mark show in the Network radios
window while it streams. The receiver also sends a GPS time stamp in every IQ frame (when the Kiwi has a GPS lock); OnAir reads it
(`kiwiStatus()`, `kiwiGpsToUnix()`) but no clock in the app uses it yet.

OnAir's receivers want at least 250 kHz of sample rate, so the 12 kHz stream is interpolated by 21 (252 kHz) inside the source (36-tap-per-phase
windowed sinc, image rejection more than 40 dB). The modes that put their channel off the radio's centre (HF digital, CW, PSK, JS8, ...) get the
Kiwi tuned to the dial and the offset applied by a mixer, so the receivers see the usual layout. The general receiver's 250 kHz offset needs more room:
that mode runs the stream at 84 times (1.008 MHz). So the sample rate is not a setting: the Kiwi runs at one rate.

## Which modes can work

By the code (not run against a live Kiwi): modes with a minimum rate up to 252 kHz and a channel of a few kHz: HF digital (FT8, FT4, WSPR, JT65/JT9, Q65,
FST4, MSK144 off the HF bands, RTTY, SSTV, PSK, Olivia and the other keyboard modes, FreeDV, HamDRM), CW, PSK, JS8, HFDL, ALE, the general receiver (AM, SSB, CW, NFM
in a window of 12 kHz) and the AM broadcast entry. A Kiwi's 12 kHz window cannot hold: HF beacons across a band (they need megahertz), wide modes, and
anything above 30 MHz. DRM: the Kiwi has its own DRM extension; OnAir does not use it and its DRM mode wants a wider capture, so DRM from a Kiwi is not
offered.

## The spectrum and waterfall

The spectrum and waterfall come from the **Kiwi's own waterfall** (the setting "Kiwi's own waterfall", on by default), as on the Kiwi's web page: the
Kiwi computes 1024 levels over the part of its 0 to 30 MHz you look at, so the picture is sharp at every zoom, from the whole HF band down to about
2 kHz. Zoom out (wheel, the Zoom slider, the zoom keys) past the 12 kHz of IQ and keep going to the whole band; pan anywhere in it; a click outside the
IQ retunes the Kiwi there, as with any radio. OnAir asks the Kiwi for the zoom that covers the view (span 30 MHz / 2^zoom, zoom 0 to 14) and its
centre, at most ten times a second, and only when the view moved by a bin or more. A new zoom starts the waterfall's rows again; a pan moves them along.

The levels are the Kiwi's dBm (the owner's calibration, `wf_cal`, applied); with a dBm offset set for the radio (Spectrum menu) they show as the Kiwi
reports them. The meters, the decoders and the automatic bandwidth still use the 12 kHz of IQ. The waterfall is a second connection to the Kiwi
(`/kiwi/<the IQ connection's time>/W/F`), the way the Kiwi's page does it, so it uses the same listener slot. The Kiwi ties the two together: when
the waterfall connection closes, it ends the IQ session too. So OnAir opens the waterfall with the IQ connection and closes it only with it (turning
the setting on or off reconnects the radio). A Kiwi has fewer waterfalls than receivers (often 2 or 3 for 8, shared): one that sends no waterfall
lines, or stops, leaves the spectrum and waterfall on the 12 kHz of IQ (after 10 seconds without a line) until the lines come again. A shared
waterfall sends only a few lines a second and pauses for a few seconds after a new zoom. The Network radios window says which ("the Kiwi's
waterfall", or why not).

## Public receivers are somebody's hobby

- One connection per receiver. OnAir opens one stream and never a second one to the same receiver. The test when you press Connect opens the connection and
  gives the slot back at once.
- A busy receiver ("too busy"), a refusal (wrong password, one connection per address), a receiver that is down, and the time limit many public Kiwis put on
  a session (often about 30 minutes) are shown in plain words. A connection that fails the first time is not repeated.
- After a loss OnAir waits before it connects again: 5 s, then 10, 20 ... up to 2 minutes, and at least a minute after "busy", a time limit, a kick or a
  receiver that is down. The wait is reset after a connection that held for 30 seconds.
- A keepalive goes every second (what the Kiwi's own page does), and the user name `OnAir` is sent so the owner sees what is listening.
- Leave when you are done. If you listen a lot to one receiver, think of the owner: many accept a thank-you note.

## What is verified and what is assumed

Read in kiwiclient's sources (protocol facts only; no code was copied): the message tags, the MSG layout, the SND frame layout (flags, sequence, S-meter,
GPS block, big-endian 16-bit IQ), the commands (`SET auth`, `SET mod=... low_cut high_cut freq`, `SET agc`, `SET compression=0`, `SET AR OK`, `SET keepalive`,
`SET ident_user`), the error names (`too_busy`, `badp` 1 to 7, `down`, `redirect`), the W/F frame layout, and the sample_rate message.

The waterfall (W/F) was checked against a live KiwiSDR (v1.902, a public receiver) and read in the KiwiSDR sources (`rx/rx_waterfall.cpp`,
`web/openwebrx/openwebrx.js`; protocol facts only): `MSG wf_setup` (with `zoom_max`, `wf_cal`) before `SET zoom=Z cf=<kHz>`, `SET maxdb/mindb`,
`SET wf_speed`, `SET wf_comp=0`, `SET interp=13`; the answer `MSG zoom=Z start=<x_bin>`; each line's x_bin (start Hz = x_bin * bandwidth / (1024 * 2^14)),
the zoom in the low 16 bits of the flags (compression is bit 16), 1024 levels of 255 + dBm, plus `wf_cal`; the first line at a new zoom is a bright
row and is dropped; and that closing the W/F connection ends the paired IQ session.

Assumed (marked in `core/src/source/kiwisdr_proto.h`): the path prefix `/kiwi/<time>/SND`; `p=#` for no password; that in IQ mode the tuned frequency is the
centre of the 12 kHz; I first, Q second (the setting "Swap I and Q" mirrors it if a Kiwi disagrees); the names of the time-limit and kick messages
(`inactivity_timeout`, `kick_by_admin`); the list's field names and address (below). On a real KiwiSDR (v1.902, a public receiver, from Windows 11) only
the connection, the IQ stream and the waterfall were tried, in the Shortwave mode; the IQ demodulation itself, the time limit and the busy and password
answers were not. The rest is tested against a fake server in `tests/source/test_kiwisdr.cpp` (handshake, commands, keepalive, IQ with sequence gaps, short / odd / compressed / oversize frames, busy, bad password,
down, time limit, reconnect pauses, fuzzed parsers, the list parser, the interpolator, the waterfall's connection, view commands, band and levels).

## The public list

The Find KiwiSDR window reads a JSON array of receivers (fields name, url, loc, antenna, sdr_hw, gps, users, users_max, snr, bands, offline), also when it
sits in a JavaScript file. The default address is the file the public map uses, `http://rx.linkfanel.net/kiwisdr_com.js`; kiwisdr.com itself could not be
reached when this was written, so the format is from memory of that file, not checked. If the listing moves or changes, the window's parser skips what it
cannot read and shows an error instead of crashing; the address is in `app/ui/toolbar_rig.cpp` (`kKiwiListUrl`).

## Licences

kiwiclient (jks-prv): the repository page shows no licence file; only protocol facts were taken. KiwiSDR (jks-prv): a mix of GPL v2.1/3 and MIT per directory and
file (its `_LICENSE` files). OnAir contains no code of either; both are credited in THIRD_PARTY.md and the in-app Credits.
