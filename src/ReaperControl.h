#pragma once

#include "ofMain.h"
#include "ofxOsc.h"

class ReaperControl{
    
public:
    
    ReaperControl(){
        resetValue = 30 * 1; // 5 sec hold
        sender.setup("localhost", 8000);
        noteOnState.assign(16, vector<int>());
        for(vector<int> & v : noteOnState){
            v.assign(128, 0);
        }
    };
    
    void update(){
        for(vector<int> & v : noteOnState){
            for(int i=0; i<v.size(); i++){
                int wait = v[i];
                v[i] = MAX(0, wait-1);
            }
        }
    }
    
    void sendNoteOn(int midiCh, int noteNum, int velocity, int duration){

        if(noteOnState[midiCh][noteNum] != 0 ) return;
        noteOnState[midiCh][noteNum] = resetValue;
        
        ofxOscMessage on;
        char c[255];
        sprintf(c, "/vkb_midi/%i/note/%i", midiCh, noteNum);
        on.setAddress(string(c));
        on.addIntArg(velocity);
        sender.sendMessage(on);
        ofLogVerbose("Note ON", string(c));
        
        std::thread noteOffThread( &ReaperControl::sendNoteOff, this, string(c), duration);
        noteOffThread.detach();
    }
    
    void sendNoteOff(string address, int duration){
        if(duration!=0) std::this_thread::sleep_for( chrono::microseconds(duration*1000) );
        ofxOscMessage off;
        off.setAddress(address);
        sender.sendMessage(off);
        
        //ofLogVerbose("Note OFF", address);
    }
    
    void sendFxParam(int track, int fx, int prm, float value){
        ofxOscMessage msg;

        char c[255];
        sprintf(c, "/track/%i/fx/%i/fxparam/%i/value", track, fx, prm);
        msg.setAddress(string(c));
        msg.addFloatArg(value);
        //ofLogVerbose("", string(c));
    }
    
    void sendNoteOffAll(){
        for(int i=0; i<16; i++){
            for(int j=0; j<127; j++){
                char c[255];
                sprintf(c, "/vkb_midi/%i/note/%i", i, j);
                sendNoteOff(string(c), 0);
            }
        }
    }
    
    ofxOscSender sender;
    
    vector<vector<int>> noteOnState;

    int resetValue;
};
