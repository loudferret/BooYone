
// https://github.com/mixxxdj/mixxx/wiki/Midi-Scripting
// eslint-disable-next-line no-var


const groupLeft  = "[Channel1]";
const groupRight = "[Channel2]";

var ARDU =  {};

ARDU.init = function(id, debugging) {
    print("Arduino Leonardo MIDI Controller INIT");
}

ARDU.shutdown = function() {
    print("Arduino Leonardo MIDI Controller SHUTDOWN");
}

ARDU.beatloopTwoBeats = function (channel, control, value, status, group) {
    print("ARDU.beatloopTwoBeats(" + status + ")");
    engine.setParameter(group, "beatloop_size", 2);
}

ARDU.beatloopFourBeats = function (channel, control, value, status, group) {
    print("ARDU.beatloopFourBeats(" + status + ")");
    engine.setParameter(group, "beatloop_size", 4);
}

ARDU.beatloopEightBeats = function (channel, control, value, status, group) {
    print("ARDU.beatloopEightBeats(" + status + ")");
    engine.setParameter(group, "beatloop_size", 8);
}

ARDU.beatloopSixteenBeats = function (channel, control, value, status, group) {
    print("ARDU.beatloopSixteenBeats(" + status + ")");
    engine.setParameter(group, "beatloop_size", 16);
}

ARDU.beatloopThirtytwoBeats = function (channel, control, value, status, group) {
    print("ARDU.beatloopThirtytwoBeats(" + status + ")");
    engine.setParameter(group, "beatloop_size", 32);
}

