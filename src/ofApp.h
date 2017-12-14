#pragma once

#include "ofMain.h"
#include "ofRange.h"

class Csound;

class ofApp : public ofBaseApp{
    
public:
    
    static ofApp * get(){
        static ofApp * app;
        if(!app) app = new ofApp();
        return app;
    }
    
    void setup();
    void update();
    void draw();
    void exit();
    void keyPressed(int key);
    
    void loadData(string fileName);
    void saveString(string str, filesystem::path path);
    void setupCsound();
    void setupOrc();
    void setupSco();
    void startCsound();
    
    bool bPrmMode = false;
    int frame = 0;
    int numOcs = -123;
    int sr = 48000;    // sampling rate
    int ksmps = 16;    // control rate
    
    float renderDuration; // sec

    vector<ofVboMesh> mesh;
    vector<ofPolyline> poly;
    vector<ofPolyline> magnitude;
    vector<ofPolyline> rotation;
    
    map<string, ofRange> range;
    map<string, int> paramId;
    
    ofVboMesh points;
    ofVboMesh prmLine;
    ofEasyCam cam;
    
    Csound * csound;

};
