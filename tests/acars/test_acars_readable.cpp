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

    // real Inmarsat Aero messages of one night (the user's database): two scrambled blocks that have enough letters, and real ones that
    // must stay (a crew notice, a turbulence report, a ZFW load message, a loadsheet)
    struct Real { const char* reg; const char* label; const char* text; bool readable; };
    const Real real[] = {
    {"N45JE", "41", "MSG0209c8N4841cL0j-G!Hc\r\nLqm\"SMa`gLRaMxtg0^FSMagjpSFagUxFg+NB G\r\ne|Lgr>aSFUSFS>MSgGUUSMaS&gWp^$vag!S$^x>@\r\nHvSgFS$^x>gwxqgRFSgS>aSF^>$gvR@gmSS>g^&S>a^U^S&gmwgGjLjgR@gRg2x@@^mpSgRUUSMaS&gFS$^x>gUxFgepxmRpg+R_^$Ra^x>RpgLRaSpp^aSgLw@aSt@gxFge|", false},
    {"D-AIMM", "3L", "573924E3AR732MFree Text DABYF LH 572,0002,69[d@k<[Ol$bb|t|j4Rwhl\?lQ\"xzwlldH}jH!=w\r\n~wkac|ralRuFRbwr\\twH)H\\$yO{cb=qy.Kin==hR]l^z`\r\nu$hugOQ\?geH[h^%jHjw=tOt|=^wnkKt|^!wsljR'j`Q\?g~\r\nl\\'\?Hzde$eObqy&oq'Kc|od\"^ta.K`zhqqzt\"\r\ndxgw'\?~\\\".H>%ww|oO&c|K\"", false},
    {"9V-SWG", "31", ".SKYSWSQ 140109 CPYXXXX        \r\nSQ0026\r\nFROM OCC\r\nHI CAPT AND CREW\r\nUS FAA HAS ANNOUNCED A NATIONWIDE GROUND STOP\r\nDUE TO COMMUNICATION ISSUES.\r\nHOWEVER, SQ FLIGHTS ARE STILL OPERATING NORMALLY.\r\nDFC", true},
    {"HL8046", "C1", ".DDLIRXA 140107\r\nAGM\r\nAN HL8046/MA 005A\r\n-  WSI FPG INTERSECTION\r\nTYPE: TURB CAT\r\nID: 44246\r\nINTERSECTION TIME: 01:37:08Z\r\nVALID: 00:00:00Z-03:00:00Z\r\nSEVERITY: OCNL MOD\r\nENTRY/EXIT: 4329N8054E / 4344N9009E\r\nALT: FL320-FL400\r\nMVT:\r\nDISC: TE", true},
    {"G-ZBKI", "13", ":PAD12.LHRLDBA.ZFW\r\nAZF/GZBKI/BA0134\r\n- PAX/191 INCLUDING 002 INF\r\n- CGO/016163\r\n- ZFW/169880\r\n- CRW/03/09\r\n- FWT/062191\r\n- OWT/192889\r\n- TTL/038644\r\n- TOW/231533\r\n- DEP/0210\r\n- SEC/BOM-LHR\r\nFLT STATUS:CLOSED\r\nLCO:SIMON STACHEL\r\nSI ISSUED O", true},
    {"HZ-AS52", "30", "ANTRY CODE  E  \r\nCHECKED BAGGAGE PIECES\r\nJED 3/TI/24/TD/35 4/Q/8/J/9/Y/23\r\nPREPARED BY JAN/PODESVA 420  221101434\r\nSERVICE WEIGHT ADJUSTMENT WEIGHT/INDEX\r\nDEDUCTIONS\r\nNIL\r\nALHAZMI\r\nBW  42655\r\nCREW EFFECT  506 / 1.4 -\r\nJED C       0 M       ", true},
    };
    for (const Real& r : real) CHECK(acarsReadable(r.label, r.text) == r.readable, "%s label %s: %s", r.reg, r.label, r.readable ? "real text dropped" : "scrambled block let through");

    // a readable header over one scrambled line: the whole text has few odd symbols, the line many
    CHECK(!acarsReadable("3L", "FREE TEXT FROM THE DISPATCH OFFICE ABOUT THE LATE ARRIVAL OF THE CREW BUS AT THE GATE TODAY\r\n"
                               "PLEASE ADVISE THE CAPTAIN AND THE PURSER ABOUT THE NEW TIMES FOR THE RETURN FLIGHT\r\n[d@k<[Ol$bb|t|j4Rwhl"),
          "one scrambled line under a long readable header");
    CHECK(!acarsReadable("10", "PLEASE CHECK gLRaMxtg SMagjpSFag GUUSMaS NOW"), "words with capitals inside");
    CHECK(acarsReadable("10", "Hi Capt And Crew, Welcome Back To Singapore"), "mixed case at the start of words stays");
    CHECK(acarsReadable("RA", "QUTYOAONH~1DIS01140152\r\nWX INFO\r\nAUTO UPLINK FOR THE ARRIVAL AT TOKYO HANEDA AIRPORT"), "one ~ of an uplink header stays");

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
