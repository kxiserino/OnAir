// "Readable text only" (dect2/acars/acars_readable.h): the kinds of text a person reads pass (free text, weather, operations notes), the
// machine data of ACARS, VDL2, HFDL and Inmarsat Aero does not (labels, FMS and datalink plumbing, encrypted blocks, short or symbol-heavy
// text). Examples are of the kind seen on a night of 10500 bit/s Inmarsat Aero (about 735 messages).
#include "dect2/acars/acars_readable.h"
#include "../common/test_util.h"
#include <cstdio>

using namespace dect2;

int main() {
    // pass: operations free text, Spanish included, and "<sp>" as a space
    CHECK(acarsReadable("10", "CREW REPORTED TURBULENCE AT FL350 NEAR WAYPOINT ABC"), "crew report");
    CHECK(acarsReadable("RA", "QUCDGOWAF~7DISPATCH MSG\r\nAF866,\r\n \r\nMERCI :)\r\n \r\nBON VOL A VOUS AUSSI\r\nDSP / NADIA S."), "dispatch note (one ~ and a smiley)");
    CHECK(acarsReadable("A9", "RECIBIDO ETOPS OK BUEN VUELO"), "Spanish ops text");
    CHECK(acarsReadable("10", "PLEASE<sp>CALL<sp>OPS<sp>ON<sp>ARRIVAL"), "<sp> as spaces");
    CHECK(acarsReadable("RA", "  WX REQUEST RECEIVED STANDBY FOR LATEST WEATHER  "), "leading and trailing spaces");
    CHECK(acarsReadable("3T", "HEAVY RAIN AND THUNDERSTORMS EXPECTED AFTER SUNSET"), "weather text");
    CHECK(acarsReadableText("REQUEST FUEL FIGURES FOR THE RETURN LEG"), "text alone");

    // labels that carry no text for a person, whatever the text
    for (const char* lb : {"_d", "_\x7f", "", "A6", "B6", "AA", "BA", "MA", "15", "16", "Q0", "SQ", "1G"})
        CHECK(!acarsReadable(lb, "CREW REPORTED TURBULENCE AT FL350"), "label %s is machine data", lb);
    CHECK(!acarsMachineLabel("10") && !acarsMachineLabel("RA") && !acarsMachineLabel("H1"), "ordinary labels are not machine labels");

    // FMS and datalink plumbing
    CHECK(!acarsReadable("H1", "- #MDREQUEST ROUTE DATA FOR FLIGHT PLAN"), "- #MD start");
    CHECK(!acarsReadable("H1", "- #DFB REQUEST DATA FROM THE GROUND"), "- #DF start");
    CHECK(!acarsReadable("H1", "- #EI ENGINE INDICATIONS REPORT FOLLOWS"), "- #EI start");
    CHECK(!acarsReadable("H1", "#M1BPOS REPORT FROM THE AIRCRAFT NOW"), "#M1 start");
    CHECK(!acarsReadable("H1", "/HDQDLXA.TEXT MESSAGE FROM THE AIRLINE"), "/ start");
    CHECK(!acarsReadable("H1", "FINNAIR DATA - #DF REQUEST FOR THE ROUTE"), "- #DF inside");
    CHECK(!acarsReadable("H1", "DATA LINK - #MD REQUEST FOR THE ROUTE"), "- #MD inside");

    // too short, too few letters, too few words; encrypted-looking blocks
    CHECK(!acarsReadable("10", "OK THANKS"), "shorter than 12");
    CHECK(!acarsReadable("10", "N51234W00512 350 0412 M47 270/45"), "numbers and symbols");
    CHECK(!acarsReadable("10", "T.0&HDeF/k2l9Qx!zP0#a8VbN"), "encrypted look");
    CHECK(!acarsReadable("10", "OK GO 12345678 99 AB"), "fewer than 3 words of 3 letters");
    CHECK(!acarsReadableText(""), "empty");
    CHECK(!acarsReadable("41", "MSG0209c8N4841cL0j-G!Hc\r\nLqm\"SMa`gLRaMxtg0^FSMagjpSFagUxFg+NB G\r\ne|Lgr>aSFUSFS>MSgGUUSMaS&gWp^$vag!S$^x>@"),
          "a scrambled block with enough letters: too many odd symbols");

    // HFDL / VDL2 summary lines
    std::string lb, tx;
    CHECK(acarsFromSummary("ACARS SA (Media advisory) SU1187: 0EH174253VH", lb, tx) && lb == "SA" && tx == "0EH174253VH", "summary parsed: %s / %s", lb.c_str(), tx.c_str());
    CHECK(acarsFromSummary("ACARS Q0 (Link test) EI0595", lb, tx) && lb == "Q0" && tx.empty(), "a summary without text");
    CHECK(acarsFromSummary("ACARS 10 BA123: A: B", lb, tx) && lb == "10" && tx == "A: B", "no label name; a colon in the text stays");
    CHECK(!acarsFromSummary("X.25 data, LCN 3071, P(S) 1 P(R) 2: compressed CLNP header", lb, tx) && !acarsFromSummary("ACARS: not a valid block", lb, tx),
          "not an ACARS frame");

    // stored messages (msgdb: type, extra lines, text)
    CHECK(msgReadable("msg.aero", "registration=EC-MXV\nlabel=A9\nlabel_text=ATIS\n", "RECIBIDO ETOPS OK BUEN VUELO"), "an Aero row");
    CHECK(!msgReadable("msg.aero", "registration=EC-MXV\nlabel=B6\n", "CREW REPORTED TURBULENCE AT FL350"), "an Aero ADS-C row");
    CHECK(!msgReadable("msg.aero", "registration=EC-MXV\n", "CREW REPORTED TURBULENCE AT FL350"), "an Aero row without a label (not ACARS)");
    CHECK(msgReadable("acars.message", "tail=G-ABCD\nlabel=10\ntext=x\n", "CREW REPORTED TURBULENCE AT FL350"), "an ACARS row");
    CHECK(msgReadable("hfdl.message", "kind=Unnumbered acked data\n", "ACARS 10 (Free text) BA123: CREW REPORTED TURBULENCE AT FL350"), "an HFDL free-text row");
    CHECK(!msgReadable("hfdl.message", "kind=Logon request (DLS)\n", "Logon request (DLS) from 151FEC"), "an HFDL logon");
    CHECK(!msgReadable("hfdl.message", "", "ACARS SA (Media advisory) SU1187: 0EH174253VH"), "an HFDL media advisory");
    CHECK(!msgReadable("vdl2.message", "proto=X.25\n", "X.25 data, LCN 3071, P(S) 4 P(R) 4: compressed CLNP header"), "a VDL2 X.25 row");
    CHECK(!msgReadable("vdl2.message", "proto=ACARS\n", "ACARS _d (Command or response) LS006H"), "a VDL2 general response");
    CHECK(msgReadable("vdl2.message", "proto=ACARS\n", "ACARS 10 (Free text) BA123: CREW REPORTED TURBULENCE AT FL350"), "a VDL2 free-text row");
    CHECK(!msgReadable("pager.message", "label=10\n", "FIRE ALARM AT THE MAIN STATION TODAY"), "another mode never");
    CHECK(msgReadable("msg.aero", "xlabel=B6\nlabel=10\n", "CREW REPORTED TURBULENCE AT FL350"), "the label is the key 'label', not one ending in it");

    return testSummary("acars_readable");
}
