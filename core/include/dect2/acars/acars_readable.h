// "Readable text only" for ACARS-family messages (ACARS, VDL2, HFDL, Inmarsat Aero): most of what a ground station or an aircraft sends is
// machine data (ADS-C, CPDLC, FMS and datalink requests, encrypted blocks, acknowledgements). This keeps what a person wrote or reads: free
// text, weather (METAR / TAF), operations notes. Header only: the message database (dect2_msgdb) uses it as an SQL function.
// Tuned on about 735 real Inmarsat Aero messages (10500 bit/s, one night): about 5 % of them pass.
#pragma once
#include <cstddef>
#include <cstring>
#include <string>

namespace dect2 {

// Labels that never carry text for a person: _d general response ("_\x7f" on the wire), "" non-ACARS user data, A6/B6 ADS-C, AA/BA CPDLC,
// MA encrypted ("T.0&HDeF/..."), 15/16 position requests (REQPOS), Q0 link test, SQ squitter, 1G
inline bool acarsMachineLabel(const std::string& label) {
    static const char* const kMachine[] = {"_d", "_\x7f", "A6", "B6", "AA", "BA", "MA", "15", "16", "Q0", "SQ", "1G"};
    if (label.empty()) return true;
    for (const char* m : kMachine) if (label == m) return true;
    return false;
}

// The text itself: FMS and datalink plumbing out ("- #MD", "- #DF", "- #EI", "#M1", "/..."), then enough letters and words to read.
// "<sp>" (how some ground stations write a space) counts as a space. Bytes of 0x80 and up count as letters (UTF-8 accents: "RECIBIDO").
inline bool acarsReadableText(const std::string& raw) {
    std::string t;
    t.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) {
        if (raw.compare(i, 4, "<sp>") == 0) { t += ' '; i += 3; continue; }
        t += raw[i];
    }
    size_t a = 0, b = t.size();
    while (a < b && (unsigned char)t[a] <= ' ') a++;
    while (b > a && (unsigned char)t[b - 1] <= ' ') b--;
    t = t.substr(a, b - a);
    for (const char* p : {"- #MD", "- #DF", "- #EI", "#M1", "/"}) if (t.compare(0, std::string(p).size(), p) == 0) return false;
    if (t.find("- #DF") != std::string::npos || t.find("- #MD") != std::string::npos) return false;
    if (t.size() < 12) return false;
    size_t letters = 0, words = 0, run = 0, odd = 0;
    auto isLetter = [](unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c >= 0x80; };
    for (unsigned char c : t) {
        if (isLetter(c)) { letters++; run++; continue; }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') letters++;   // whitespace counts with the letters
        if (std::strchr("!\"$&^|<>@{}[]`\\~", (int)c) && c) odd++;
        if (run >= 3) words++;
        run = 0;
    }
    if (run >= 3) words++;
    // symbols that written text hardly has: more than 2 % is a scrambled or binary block that still has enough letters (label 41 "MSG0209c8N...|Lgr>a")
    return letters * 10 >= t.size() * 6 && words >= 3 && odd * 50 <= t.size();
}

inline bool acarsReadable(const std::string& label, const std::string& text) { return !acarsMachineLabel(label) && acarsReadableText(text); }

// An HFDL or VDL2 summary line: "ACARS <label>[ (<label name>)][ <flight>][: <text, at most 80 characters>]" (vdl2_proto.cpp, hfdl_proto.cpp).
// Its label and text; false for any other frame (X.25, logons, link control): those are never text for a person.
inline bool acarsFromSummary(const std::string& s, std::string& label, std::string& text) {
    if (s.compare(0, 6, "ACARS ") != 0 || s.size() < 8) return false;
    label = s.substr(6, 2);
    size_t from = 8;
    if (s.compare(8, 2, " (") == 0) { const size_t close = s.find(')', 10); if (close != std::string::npos) from = close + 1; }
    const size_t colon = s.find(": ", from);
    text = colon == std::string::npos ? std::string() : s.substr(colon + 2);
    return true;
}

// A stored message (msgdb: type, the extra "key=value" lines, the text): an ACARS-family message with readable text. Anything else is not.
// HFDL and VDL2 records keep a summary line: the label and the text are read back out of it (acarsFromSummary).
inline bool msgReadable(const std::string& type, const std::string& extra, const std::string& text) {
    auto field = [&](const char* key) -> std::string {
        const std::string k = std::string(key) + "=";
        for (size_t i = 0; i < extra.size();) {
            size_t e = extra.find('\n', i);
            if (e == std::string::npos) e = extra.size();
            if (extra.compare(i, k.size(), k) == 0) return extra.substr(i + k.size(), e - i - k.size());
            i = e + 1;
        }
        return std::string();
    };
    if (type == "acars.message" || type == "msg.aero") return acarsReadable(field("label"), text);
    if (type == "vdl2.message" || type == "hfdl.message") {
        std::string label, body;
        return acarsFromSummary(text, label, body) && acarsReadable(label, body);
    }
    return false;
}

} // namespace dect2
